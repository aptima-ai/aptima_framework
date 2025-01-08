//
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#include <cstring>
#include <string>

#include "include_internal/axis_runtime/app/metadata.h"
#include "include_internal/axis_runtime/binding/cpp/detail/addon_loader.h"
#include "include_internal/axis_runtime/binding/cpp/detail/addon_manager.h"
#include "include_internal/axis_runtime/binding/python/common.h"
#include "include_internal/axis_runtime/common/base_dir.h"
#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/metadata/manifest.h"
#include "axis_runtime/addon/addon.h"
#include "axis_utils/container/list_str.h"
#include "axis_utils/lib/module.h"
#include "axis_utils/lib/path.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/log/log.h"
#include "axis_utils/macro/check.h"

static void foo() {}

/**
 * This addon is used for those ten app whose "main" function is not written in
 * python. By putting this addon into a ten app, the python runtime can be
 * initialized and other python addons can be loaded and registered to the ten
 * world when the ten app is started.
 *
 * Time sequence:
 *
 * 0) The executable of the ten app (non-python) links with libaxis_runtime.
 *
 * 1) The program of the ten app (non-python) is started, with libaxis_runtime
 *    being loaded, which triggers this addon to be dlopen-ed.
 *
 * 2) libaxis_runtime will call 'axis_addon_register_extension()' synchronously,
 *    then python_addon_loader_addon_t::on_init() will be called from
 * libaxis_runtime.
 *
 * 3) python_addon_loader_addon_t::on_init() will handle things including
 * Py_Initialize, setup sys.path, load all python addons in the app's addon/
 * folder.
 *
 * 4) libaxis_runtime_python will be loaded when any python addon is loaded (due
 *    to the python code: 'import libaxis_runtime_python')
 *
 * 5) After all python addons are registered,
 * python_addon_loader_addon_t::on_init() will release the python GIL so that
 * other python codes can be executed in any other threads after they acquiring
 * the GIL.
 *
 * ================================================
 * What will happen if the app is a python program?
 *
 * If no special handling is done, there will be the following 2 problems:
 *
 * 1) Python prohibits importing the same module again before it has been fully
 *    imported (i.e., circular imports). And if the main program is a python
 *    program, and if the main program loads libaxis_runtime_python (because it
 *    might need some features in it), python addons will be loaded after
 *    libaxis_runtime_python is imported (because libaxis_runtime_python will
 *    loads libaxis_runtime, and libaxis_runtime will loop addon/ folder to
 *    load/dlopen all the _native_ addons in it, and it will load
 *    python_addon_loader, and this python_addon_loader will load all python
 * addons in addon/ folder). And if these loaded Python addons load
 *    libaxis_runtime_python (because they need to use its functionalities),
 *    which will create a circular import.
 *
 * 2. If the main program is a python program and it loaded this addon
 *    _synchronously_ in the python main thread (see above), then if the GIL is
 *    released in python_addon_loader_addon_t::on_init, then no more further
 * python codes can be executed normally in the python main thread.
 *
 * 3. Even though the app is not a python program, if the python
 *    multiprocessing mode is set to 'spawn', then the subprocess will be
 *    executed by a __Python__ interpreter, not the origin native executable.
 *    While if the 'libaxis_runtime_python' module is imported before the target
 *    function is called in subprocess (For example, if the Python module
 *    containing the target function or its parent folder's Python module
 *    imports axis_runtime_python.) (And this situation is similar to the python
 *    main situation), then libaxis_runtime will be loaded again, which will
 *    cause this addon to be loaded. Which results in a circular import similar
 *    to the situation described above.
 *
 * How to avoid any side effects?
 *
 * The main reason is that, theoretically, python main and python_addon_loader
 * should not be used together. However, due to some reasonable or unreasonable
 * reasons mentioned above, python main and python_addon_loader are being used
 * together. Therefore, what we need to do in this situation is to detect this
 * case and then essentially disable python_addon_loader. By checking
 * 'axis_py_is_initialized' on python_addon_loader_addon_t::on_init, we can know
 * whether the python runtime has been initialized. And the calling operation
 * here is thread safe, because if the app is not a python program, the python
 * runtime is not initialized for sure, and if the app is a python program, then
 * the python_addon_loader_addon_t::on_init will be called in the python main
 * thread and the GIL is held, so it is thread safe to call
 * 'axis_py_is_initialized'.
 */

namespace {

class python_addon_loader_t : public ten::addon_loader_t {
 public:
  explicit python_addon_loader_t(const char *name) { (void)name; };

  void on_init() override {
    // Do some initializations.

    int py_initialized = axis_py_is_initialized();
    if (py_initialized != 0) {
      axis_LOGI("Python runtime has been initialized.");
      return;
    }

    py_init_by_self_ = true;

    // We met 'symbols not found' error when loading python modules while the
    // symbols are expected to be found in the python lib. We need to load the
    // python lib first.
    //
    // Refer to
    // https://mail.python.org/pipermail/new-bugs-announce/2008-November/003322.html?from_wecom=1
    load_python_lib();

    axis_py_initialize();

    find_app_base_dir();

    // Before loading the ten python modules (extensions), we have to complete
    // sys.path first.
    complete_sys_path();

    axis_py_run_simple_string(
        "import sys\n"
        "print(sys.path)\n");

    const auto *sys_path = axis_py_get_path();
    axis_LOGI(
        (std::string("python initialized, sys.path: ") + sys_path).c_str());

    axis_py_mem_free((void *)sys_path);

    start_debugpy_server_if_needed();

    if (load_all_on_init) {
      // Traverse `axis_packages/extension` directory and import module.
      load_python_extensions_according_to_app_manifest_dependencies();
    } else {
      axis_LOGI(
          "load_all_on_init is false, skip loading all python "
          "extensions when startup.");
    }

    // The `app_base_dir` is no longer needed afterwards, so it is released.
    axis_string_destroy(app_base_dir);
    app_base_dir = nullptr;

    py_thread_state_ = axis_py_eval_save_thread();
  }

  void on_deinit() override {
    // Do some de-initializations.
    if (py_thread_state_ != nullptr) {
      axis_py_eval_restore_thread(py_thread_state_);
    }

    if (py_init_by_self_) {
      int rc = axis_py_finalize();
      if (rc < 0) {
        axis_LOGF((std::string("Failed to finalize python runtime, rc: ") +
                  std::to_string(rc))
                     .c_str());

        axis_ASSERT(0, "Should not happen.");
      }
    }
  }

  // **Note:** This function, used to dynamically load other addons, may be
  // called from multiple threads. Therefore, it must be thread-safe. Since it
  // calls `axis_py_gil_state_ensure` and `axis_py_gil_state_release`, thread
  // safety is ensured.
  void on_load_addon(axis_ADDON_TYPE addon_type,
                     const char *addon_name) override {
    // Load the specified addon.

    void *axis_py_gil_state = axis_py_gil_state_ensure();

    // Construct the full module name.
    axis_string_t *full_module_name = axis_string_create_formatted(
        "axis_packages.%s.%s", axis_addon_type_to_string(addon_type), addon_name);

    // Import the specified Python module.
    axis_py_import_module(axis_string_get_raw_str(full_module_name));

    axis_string_destroy(full_module_name);

    // Register the addon if necessary.
    register_single_addon(addon_type, addon_name);

    axis_py_gil_state_release(axis_py_gil_state);
  }

 private:
  void *py_thread_state_ = nullptr;
  bool py_init_by_self_ = false;
  bool load_all_on_init = false;
  axis_string_t *app_base_dir = nullptr;

  void find_app_base_dir() {
    axis_string_t *module_path =
        axis_path_get_module_path(reinterpret_cast<const void *>(foo));
    axis_ASSERT(module_path, "Failed to get module path.");

    app_base_dir = axis_find_base_dir(axis_string_get_raw_str(module_path),
                                     axis_STR_APP, nullptr);
    axis_string_destroy(module_path);
  }

  // Setup python system path and make sure following paths are included:
  // <app_root>/axis_packages/system/axis_runtime_python/lib
  // <app_root>/axis_packages/system/axis_runtime_python/interface
  // <app_root>
  //
  // The reason for adding `<app_root>` to `sys.path` is that when using
  // `PyImport_Import` to load Python packages under `axis_packages/`, the module
  // name used will be in the form of `axis_packages.extensions.xxx`. Therefore,
  // `<app_root>` must be in `sys.path` to ensure that `axis_packages` can be
  // located.
  void complete_sys_path() {
    axis_list_t paths;
    axis_list_init(&paths);

    axis_string_t *lib_path = axis_string_create_formatted(
        "%s/axis_packages/system/axis_runtime_python/lib",
        axis_string_get_raw_str(app_base_dir));
    axis_string_t *interface_path = axis_string_create_formatted(
        "%s/axis_packages/system/axis_runtime_python/interface",
        axis_string_get_raw_str(app_base_dir));

    axis_list_push_str_back(&paths, axis_string_get_raw_str(lib_path));
    axis_list_push_str_back(&paths, axis_string_get_raw_str(interface_path));
    axis_list_push_str_back(&paths, axis_string_get_raw_str(app_base_dir));

    axis_string_destroy(lib_path);
    axis_string_destroy(interface_path);

    axis_py_add_paths_to_sys(&paths);

    axis_list_clear(&paths);
  }

  // Get the real path of <app_root>/axis_packages/extension/
  axis_string_t *get_addon_extensions_path() {
    axis_string_t *result = axis_string_clone(app_base_dir);
    axis_string_append_formatted(result, "/axis_packages/extension/");
    return result;
  }

  void load_python_extensions_according_to_app_manifest_dependencies() {
    axis_string_t *addon_extensions_path = get_addon_extensions_path();

    load_all_python_modules(addon_extensions_path);

    register_all_addons();

    axis_string_destroy(addon_extensions_path);
  }

  // Start the debugpy server according to the environment variable and wait for
  // the debugger to connect.
  static void start_debugpy_server_if_needed() {
    const char *enable_python_debug = getenv("axis_ENABLE_PYTHON_DEBUG");
    if (enable_python_debug == nullptr ||
        strcmp(enable_python_debug, "true") != 0) {
      return;
    }

    const char *python_debug_host = getenv("axis_PYTHON_DEBUG_HOST");
    if (python_debug_host == nullptr) {
      python_debug_host = "localhost";
    }

    const char *python_debug_port = getenv("axis_PYTHON_DEBUG_PORT");
    if (python_debug_port == nullptr) {
      python_debug_port = "5678";
    }

    // Make sure the port is valid.
    char *endptr = nullptr;
    int64_t port = std::strtol(python_debug_port, &endptr, 10);
    if (*endptr != '\0' || port <= 0 || port > 65535) {
      axis_LOGE((std::string("Invalid python debug port: ") + python_debug_port)
                   .c_str());
      return;
    }

    axis_string_t *start_debug_server_script = axis_string_create_formatted(
        "import debugpy\n"
        "debugpy.listen(('%s', %d))\n"
        "debugpy.wait_for_client()\n",
        python_debug_host, port);

    axis_py_run_simple_string(axis_string_get_raw_str(start_debug_server_script));

    axis_string_destroy(start_debug_server_script);

    axis_LOGI((std::string("Python debug server started at ") +
              python_debug_host + std::to_string(port))
                 .c_str());
  }

  // Load all python addons by import modules.
  static void load_all_python_modules(axis_string_t *addon_extensions_path) {
    if (addon_extensions_path == nullptr ||
        axis_string_is_empty(addon_extensions_path)) {
      axis_LOGE(
          "Failed to load python modules due to empty addon extension path.");
      return;
    }

    axis_dir_fd_t *dir =
        axis_path_open_dir(axis_string_get_raw_str(addon_extensions_path));
    if (dir == nullptr) {
      axis_LOGE((std::string("Failed to open directory: ") +
                axis_string_get_raw_str(addon_extensions_path) +
                " when loading python modules.")
                   .c_str());
      return;
    }

    axis_path_itor_t *itor = axis_path_get_first(dir);
    while (itor != nullptr) {
      axis_string_t *short_name = axis_path_itor_get_name(itor);
      if (short_name == nullptr) {
        axis_LOGE((std::string("Failed to get short name under path ") +
                  axis_string_get_raw_str(addon_extensions_path) +
                  ", when loading python modules.")
                     .c_str());
        itor = axis_path_get_next(itor);
        continue;
      }

      if (!(axis_string_is_equal_c_str(short_name, ".") ||
            axis_string_is_equal_c_str(short_name, ".."))) {
        // The full module name is "axis_packages.extension.<short_name>"
        axis_string_t *full_module_name = axis_string_create_formatted(
            "axis_packages.extension.%s", axis_string_get_raw_str(short_name));
        axis_py_import_module(axis_string_get_raw_str(full_module_name));
        axis_string_destroy(full_module_name);
      }

      axis_string_destroy(short_name);
      itor = axis_path_get_next(itor);
    }

    if (dir != nullptr) {
      axis_path_close_dir(dir);
    }
  }

  static void register_all_addons() {
    axis_py_run_simple_string(
        "from ten import _AddonManager\n"
        "_AddonManager.register_all_addons(None)\n");
  }

  static void register_single_addon(axis_ADDON_TYPE addon_type,
                                    const char *addon_name) {
    (void)addon_type;

    std::string register_script =
        "from ten import _AddonManager\n"
        "_AddonManager.register_addon('" +
        std::string(addon_name) + "', None)\n";
    axis_py_run_simple_string(register_script.c_str());
  }

  static void load_python_lib() {
    axis_string_t *python_lib_path =
        axis_string_create_formatted("libaxis_runtime_python.so");

    // The libaxis_runtime_python.so must be loaded globally using dlopen, and
    // cannot be a regular shared library dependency. Note that the 2nd
    // parameter must be 0 (as_local = false).
    //
    // Refer to
    // https://mail.python.org/pipermail/new-bugs-announce/2008-November/003322.html
    axis_module_load(python_lib_path, 0);

    axis_string_destroy(python_lib_path);
  }
};

axis_CPP_REGISTER_ADDON_AS_ADDON_LOADER(python_addon_loader,
                                       python_addon_loader_t);

}  // namespace
