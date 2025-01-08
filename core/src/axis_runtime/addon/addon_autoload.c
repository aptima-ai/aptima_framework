//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/addon/addon_autoload.h"

#if defined(OS_LINUX)
#define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

#include "include_internal/axis_runtime/addon/addon.h"
#include "include_internal/axis_runtime/addon/addon_manager.h"
#include "include_internal/axis_runtime/addon_loader/addon_loader.h"
#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/global/global.h"
#include "include_internal/axis_runtime/global/signal.h"
#include "include_internal/axis_runtime/metadata/manifest.h"
#include "include_internal/axis_utils/log/log.h"
#include "axis_runtime/addon/addon.h"
#include "axis_runtime/common/errno.h"
#include "axis_utils/container/list.h"
#include "axis_utils/container/list_node_str.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/module.h"
#include "axis_utils/lib/path.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/log/log.h"

/**
 * @brief
 * - For PC platforms, we can load all extensions dynamically using
 *   dlopen/LoadLibrary.
 *
 * - For iOS, it is not allowed to load addon libraries in runtime, so "load
 *   dylib" will be done by application, and that application will mark the
 *   library dependency in _compile_time_.
 *
 * - For Android, the only "safe" way to load addon libraries is by using
 *   System.load("xxx") function from Java.
 *
 * Neither Android nor iOS support unload library.
 */
#if defined(OS_MACOS) || defined(OS_LINUX) || defined(OS_WINDOWS)

static bool load_all_dynamic_libraries_under_path(const char *path) {
  axis_ASSERT(path, "Invalid argument.");

  bool load_at_least_one = false;

  axis_dir_fd_t *dir = NULL;
  axis_path_itor_t *itor = NULL;
  axis_string_t *file_path = NULL;
  axis_string_t *short_name = NULL;

  if (!path || !strlen(path)) {
    axis_LOGE("Failed to load dynamic library: invalid parameter.");
    goto done;
  }

  dir = axis_path_open_dir(path);
  if (!dir) {
    axis_LOGE("Failed to open directory: %s", path);
    goto done;
  }

  itor = axis_path_get_first(dir);
  while (itor) {
    short_name = axis_path_itor_get_name(itor);
    if (!short_name) {
      axis_LOGE("Failed to get short name under path: %s", path);
      goto continue_loop;
    }

    if (axis_string_is_equal_c_str(short_name, ".")) {
      goto continue_loop;
    }

    if (axis_string_is_equal_c_str(short_name, "..")) {
      goto continue_loop;
    }

    file_path = axis_path_itor_get_full_name(itor);
    if (!file_path || !axis_path_is_shared_library(file_path)) {
      goto continue_loop;
    }

    if (axis_path_to_system_flavor(file_path) != 0) {
      axis_LOGE("Failed to convert path to system flavor: %s",
               axis_string_get_raw_str(file_path));
      goto continue_loop;
    }

    void *module_handle = axis_module_load(file_path, 1);
    if (!module_handle) {
      axis_LOGE("Failed to load module: %s", axis_string_get_raw_str(file_path));
      goto continue_loop;
    }

    load_at_least_one = true;

    axis_LOGD("Loaded module: %s", axis_string_get_raw_str(file_path));

  continue_loop:
    if (file_path) {
      axis_string_destroy(file_path);
      file_path = NULL;
    }

    if (short_name) {
      axis_string_destroy(short_name);
      short_name = NULL;
    }

    itor = axis_path_get_next(itor);
  }

done:
  if (dir) {
    axis_path_close_dir(dir);
  }

  return load_at_least_one;
}

static void axis_addon_load_from_base_dir(const char *path) {
  axis_string_t lib_dir;
  axis_string_init_formatted(&lib_dir, "%s", path);

  if (!axis_path_is_dir(&lib_dir)) {
    goto done;
  }

  axis_string_append_formatted(&lib_dir, "/lib");
  if (axis_path_to_system_flavor(&lib_dir) != 0) {
    axis_LOGE("Failed to convert path to system flavor: %s",
             axis_string_get_raw_str(&lib_dir));
    goto done;
  }

  if (!axis_path_exists(axis_string_get_raw_str(&lib_dir)) ||
      !axis_path_is_dir(&lib_dir)) {
    axis_LOGD("The dynamic library path(%s) does not exist.",
             axis_string_get_raw_str(&lib_dir));
    goto done;
  }

  // Load self first.
  load_all_dynamic_libraries_under_path(axis_string_get_raw_str(&lib_dir));

done:
  axis_string_deinit(&lib_dir);
}

static void load_all_dynamic_libraries(const char *path) {
  axis_string_t *cur = NULL;
  axis_string_t *short_name = NULL;
  axis_string_t *self = NULL;
  axis_string_t *sub = NULL;
  axis_dir_fd_t *dir = NULL;
  axis_path_itor_t *itor = NULL;

  if (!path || !strlen(path)) {
    goto done;
  }

  dir = axis_path_open_dir(path);
  if (!dir) {
    axis_LOGE("Failed to open directory: %s", path);
    goto done;
  }

  itor = axis_path_get_first(dir);
  while (itor) {
    short_name = axis_path_itor_get_name(itor);
    if (!short_name) {
      axis_LOGE("Failed to get short name under path: %s", path);
      goto continue_loop;
    }

    if (axis_path_is_special_dir(short_name)) {
      goto continue_loop;
    }

    cur = axis_path_itor_get_full_name(itor);
    if (!cur) {
      axis_LOGE("Failed to get full name under path: %s", path);
      goto continue_loop;
    }

    axis_addon_load_from_base_dir(axis_string_get_raw_str(cur));

    axis_string_destroy(cur);
    cur = NULL;

  continue_loop:
    if (cur) {
      axis_string_destroy(cur);
      cur = NULL;
    }

    if (short_name) {
      axis_string_destroy(short_name);
      short_name = NULL;
    }

    if (self) {
      axis_string_destroy(self);
      self = NULL;
    }

    if (sub) {
      axis_string_destroy(sub);
      sub = NULL;
    }

    itor = axis_path_get_next(itor);
  }

done:
  if (dir) {
    axis_path_close_dir(dir);
  }
}

bool axis_addon_load_all_from_app_base_dir(const char *app_base_dir,
                                          axis_error_t *err) {
  axis_ASSERT(app_base_dir, "Invalid argument.");

  bool success = true;

  // The extension is loaded dynamically and will only be loaded when it's
  // needed. Both the protocol and addon_loader are preloaded, so when the
  // app starts, it will scan these two folders and load all the addons inside
  // them.
  struct {
    const char *path;
  } folders[] = {
      {"/axis_packages/protocol"},
      {"/axis_packages/addon_loader"},
  };

  for (int i = 0; i < sizeof(folders) / sizeof(folders[0]); i++) {
    axis_string_t module_path;
    axis_string_init_from_c_str(&module_path, app_base_dir,
                               strlen(app_base_dir));

    do {
      axis_string_append_formatted(&module_path, folders[i].path);

      if (axis_path_to_system_flavor(&module_path) != 0) {
        axis_LOGE("Failed to convert path to system flavor: %s",
                 axis_string_get_raw_str(&module_path));

        success = false;
        break;
      }

      // The modules (e.g., extensions/protocols) do not exist if only the TEN
      // app has been installed.
      if (axis_path_exists(axis_string_get_raw_str(&module_path))) {
        load_all_dynamic_libraries(axis_string_get_raw_str(&module_path));
      }
    } while (0);

    axis_string_deinit(&module_path);

    if (!success) {
      goto done;
    }
  }

done:
  return success;
}

bool axis_addon_load_all_from_axis_package_base_dirs(
    axis_list_t *axis_package_base_dirs, axis_error_t *err) {
  axis_ASSERT(axis_package_base_dirs, "Invalid argument.");

  bool success = true;

  axis_list_foreach (axis_package_base_dirs, iter) {
    axis_string_t *axis_package_base_dir = axis_str_listnode_get(iter.node);
    axis_ASSERT(axis_package_base_dir &&
                   axis_string_check_integrity(axis_package_base_dir),
               "Should not happen.");

    axis_LOGI("Load dynamic libraries under path: %s",
             axis_string_get_raw_str(axis_package_base_dir));

    axis_ADDON_TYPE addon_type = axis_ADDON_TYPE_INVALID;
    axis_string_t addon_name;
    axis_string_init(&addon_name);

    // Construct `manifest.json` path.
    axis_string_t manifest_json_file_path;
    axis_string_init_from_string(&manifest_json_file_path, axis_package_base_dir);
    axis_path_join_c_str(&manifest_json_file_path, axis_STR_MANIFEST_JSON);

    // Get type and name from manifest file.
    axis_manifest_get_type_and_name(
        axis_string_get_raw_str(&manifest_json_file_path), &addon_type,
        &addon_name, NULL);

    axis_string_deinit(&manifest_json_file_path);

    axis_addon_load_from_base_dir(axis_string_get_raw_str(axis_package_base_dir));

    axis_string_deinit(&addon_name);
  }

  return success;
}

static bool axis_addon_load_specific_addon_using_native_addon_loader(
    const char *app_base_dir, axis_ADDON_TYPE addon_type, const char *addon_name,
    axis_error_t *err) {
  axis_ASSERT(app_base_dir, "Invalid argument.");
  axis_ASSERT(addon_name, "Addon name cannot be NULL.");

  bool success = true;
  axis_string_t addon_lib_folder_path;

  // Construct the path to the specific addon lib/ folder.
  axis_string_init_from_c_str(&addon_lib_folder_path, app_base_dir,
                             strlen(app_base_dir));

  axis_string_append_formatted(&addon_lib_folder_path, "/axis_packages/%s/%s/lib",
                              axis_addon_type_to_string(addon_type), addon_name);
  if (axis_path_to_system_flavor(&addon_lib_folder_path) != 0) {
    axis_LOGE("Failed to convert path to system flavor: %s",
             axis_string_get_raw_str(&addon_lib_folder_path));
    success = false;
    goto done;
  }

  if (!axis_path_exists(axis_string_get_raw_str(&addon_lib_folder_path)) ||
      !axis_path_is_dir(&addon_lib_folder_path)) {
    axis_LOGI("Addon lib/ folder does not exist or is not a directory: %s",
             axis_string_get_raw_str(&addon_lib_folder_path));
    success = false;
    goto done;
  }

  // Load the library from the 'lib/' directory.
  success = load_all_dynamic_libraries_under_path(
      axis_string_get_raw_str(&addon_lib_folder_path));

done:
  axis_string_deinit(&addon_lib_folder_path);

  if (!success && err) {
    axis_error_set(err, axis_ERRNO_GENERIC,
                  "Failed to load specific addon: %s:%s",
                  axis_addon_type_to_string(addon_type), addon_name);
  }

  return success;
}

static bool axis_addon_register_specific_addon(
    axis_ADDON_TYPE addon_type, const char *addon_name,
    axis_addon_register_ctx_t *register_ctx, axis_error_t *err) {
  axis_ASSERT(addon_name, "Invalid argument.");

  axis_addon_manager_t *manager = axis_addon_manager_get_instance();

  bool success = axis_addon_manager_register_specific_addon(
      manager, addon_type, addon_name, (void *)register_ctx);

  if (!success && err) {
    axis_error_set(err, axis_ERRNO_GENERIC,
                  "Failed to register specific addon: %s:%s",
                  axis_addon_type_to_string(addon_type), addon_name);
  }

  return success;
}

bool axis_addon_try_load_specific_addon_using_native_addon_loader(
    const char *app_base_dir, axis_ADDON_TYPE addon_type,
    const char *addon_name) {
  if (axis_addon_load_specific_addon_using_native_addon_loader(
          app_base_dir, addon_type, addon_name, NULL)) {
    if (!axis_addon_register_specific_addon(addon_type, addon_name, NULL,
                                           NULL)) {
      return false;
    }
  }

  return true;
}

bool axis_addon_try_load_specific_addon_using_all_addon_loaders(
    axis_ADDON_TYPE addon_type, const char *addon_name) {
  axis_list_t *addon_loaders = axis_addon_loader_get_all();
  axis_ASSERT(addon_loaders, "Should not happen.");

  axis_list_foreach (addon_loaders, iter) {
    axis_addon_loader_t *addon_loader = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(addon_loader, "Should not happen.");

    if (addon_loader) {
      axis_addon_loader_load_addon(addon_loader, addon_type, addon_name);
    }
  }

  return true;
}

#endif
