//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/addon/addon_autoload.h"

#if defined(OS_LINUX)
#define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

#include "include_internal/ten_runtime/addon/addon.h"
#include "include_internal/ten_runtime/addon/addon_manager.h"
#include "include_internal/ten_runtime/addon/extension/extension.h"
#include "include_internal/ten_runtime/app/app.h"
#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/global/global.h"
#include "include_internal/ten_runtime/global/signal.h"
#include "include_internal/ten_runtime/metadata/manifest.h"
#include "include_internal/ten_utils/log/log.h"
#include "ten_runtime/app/app.h"
#include "ten_utils/container/list.h"
#include "ten_utils/container/list_node_str.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/lib/module.h"
#include "ten_utils/lib/path.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/log/log.h"

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
static void load_all_dynamic_libraries_under_path(ten_app_t *app,
                                                  const char *path) {
  TEN_ASSERT(app && ten_app_check_integrity(app, true), "Invalid argument.");
  TEN_ASSERT(path, "Invalid argument.");

  ten_dir_fd_t *dir = NULL;
  ten_path_itor_t *itor = NULL;
  ten_string_t *file_path = NULL;
  ten_string_t *short_name = NULL;

  if (!path || !strlen(path)) {
    TEN_LOGE("Failed to load dynamic library: invalid parameter.");
    goto done;
  }

  dir = ten_path_open_dir(path);
  if (!dir) {
    TEN_LOGE("Failed to open directory: %s", path);
    goto done;
  }

  itor = ten_path_get_first(dir);
  while (itor) {
    short_name = ten_path_itor_get_name(itor);
    if (!short_name) {
      TEN_LOGE("Failed to get short name under path: %s", path);
      goto continue_loop;
    }

    if (ten_string_is_equal_c_str(short_name, ".")) {
      goto continue_loop;
    }

    if (ten_string_is_equal_c_str(short_name, "..")) {
      goto continue_loop;
    }

    file_path = ten_path_itor_get_full_name(itor);
    if (!file_path || !ten_path_is_shared_library(file_path)) {
      goto continue_loop;
    }

    ten_addon_host_t *addon_host = ten_addon_store_find(
        ten_extension_get_global_store(), ten_string_get_raw_str(short_name));
    if (addon_host) {
      // Do _not_ register again.
      goto continue_loop;
    }

    if (ten_path_to_system_flavor(file_path) != 0) {
      TEN_LOGE("Failed to convert path to system flavor: %s",
               ten_string_get_raw_str(file_path));
      goto continue_loop;
    }

    void *module_handle = ten_module_load(file_path, 1);
    if (!module_handle) {
      TEN_LOGE("Failed to load module: %s", ten_string_get_raw_str(file_path));
      goto continue_loop;
    }

    TEN_LOGD("Loaded module: %s", ten_string_get_raw_str(file_path));

  continue_loop:
    if (file_path) {
      ten_string_destroy(file_path);
      file_path = NULL;
    }

    if (short_name) {
      ten_string_destroy(short_name);
      short_name = NULL;
    }

    itor = ten_path_get_next(itor);
  }

done:
  if (dir) {
    ten_path_close_dir(dir);
  }
}

static void ten_addon_load_from_base_dir(ten_app_t *app, const char *path) {
  ten_string_t lib_dir;
  ten_string_init_formatted(&lib_dir, "%s", path);

  if (!ten_path_is_dir(&lib_dir)) {
    goto done;
  }

  ten_string_append_formatted(&lib_dir, "/lib");
  if (ten_path_to_system_flavor(&lib_dir) != 0) {
    TEN_LOGE("Failed to convert path to system flavor: %s",
             ten_string_get_raw_str(&lib_dir));
    goto done;
  }

  if (!ten_path_exists(ten_string_get_raw_str(&lib_dir)) ||
      !ten_path_is_dir(&lib_dir)) {
    TEN_LOGD("The dynamic library path(%s) does not exist.",
             ten_string_get_raw_str(&lib_dir));
    goto done;
  }

  // Load self first.
  load_all_dynamic_libraries_under_path(app, ten_string_get_raw_str(&lib_dir));

done:
  ten_string_deinit(&lib_dir);
}

static void load_all_dynamic_libraries(ten_app_t *app, const char *path,
                                       ten_list_t *dependencies) {
  ten_string_t *cur = NULL;
  ten_string_t *short_name = NULL;
  ten_string_t *self = NULL;
  ten_string_t *sub = NULL;
  ten_dir_fd_t *dir = NULL;
  ten_path_itor_t *itor = NULL;

  if (!path || !strlen(path)) {
    goto done;
  }

  dir = ten_path_open_dir(path);
  if (!dir) {
    TEN_LOGE("Failed to open directory: %s", path);
    goto done;
  }

  itor = ten_path_get_first(dir);
  while (itor) {
    short_name = ten_path_itor_get_name(itor);
    if (!short_name) {
      TEN_LOGE("Failed to get short name under path: %s", path);
      goto continue_loop;
    }

    if (ten_path_is_special_dir(short_name)) {
      goto continue_loop;
    }

    // Check if short_name is in dependencies list.
    bool should_load = false;
    if (dependencies) {
      // Iterate over dependencies and check if short_name matches any.
      ten_list_foreach (dependencies, dep_iter) {
        ten_string_t *dep_name = ten_str_listnode_get(dep_iter.node);
        if (ten_string_is_equal(short_name, dep_name)) {
          should_load = true;
          break;
        }
      }
    } else {
      // If dependencies list is NULL, we load all addons.
      should_load = true;
    }

    if (!should_load) {
      TEN_LOGI("Skipping addon '%s' as it's not in dependencies.",
               ten_string_get_raw_str(short_name));
      goto continue_loop;
    }

    cur = ten_path_itor_get_full_name(itor);
    if (!cur) {
      TEN_LOGE("Failed to get full name under path: %s", path);
      goto continue_loop;
    }

    ten_addon_load_from_base_dir(app, ten_string_get_raw_str(cur));

    ten_string_destroy(cur);
    cur = NULL;

  continue_loop:
    if (cur) {
      ten_string_destroy(cur);
      cur = NULL;
    }

    if (short_name) {
      ten_string_destroy(short_name);
      short_name = NULL;
    }

    if (self) {
      ten_string_destroy(self);
      self = NULL;
    }

    if (sub) {
      ten_string_destroy(sub);
      sub = NULL;
    }

    itor = ten_path_get_next(itor);
  }

done:
  if (dir) {
    ten_path_close_dir(dir);
  }
}

bool ten_addon_load_all_from_app_base_dir(
    ten_app_t *app, ten_list_t *extension_dependencies,
    ten_list_t *extension_group_dependencies, ten_list_t *protocol_dependencies,
    ten_error_t *err) {
  TEN_ASSERT(app && ten_app_check_integrity(app, true), "Invalid argument.");

  bool success = true;
  ten_string_t *app_lib_path = NULL;

#if defined(_WIN32)
  // Append the 'app/lib' to the search path first, the module in extensions and
  // protocols may depend on the libraries in 'app/lib'.
  app_lib_path = ten_string_clone(&app->base_dir);
  if (!app_lib_path) {
    TEN_LOGE("Failed to clone string: %s",
             ten_string_get_raw_str(&app->base_dir));

    success = false;
    goto done;
  }

  ten_string_append_formatted(app_lib_path, "/lib");
  if (ten_path_to_system_flavor(app_lib_path) != 0) {
    TEN_LOGE("Failed to convert path to system flavor: %s",
             ten_string_get_raw_str(app_lib_path));

    success = false;
    goto done;
  }

  AddDllDirectory(ten_string_get_raw_str(app_lib_path));
#endif

  struct {
    const char *path;
    TEN_ADDON_TYPE addon_type;
    ten_list_t *dependencies;
  } folders[] = {
      {
          "/ten_packages/extension",
          TEN_ADDON_TYPE_EXTENSION,
          extension_dependencies,
      },
      {
          "/ten_packages/extension_group",
          TEN_ADDON_TYPE_EXTENSION_GROUP,
          extension_group_dependencies,
      },
      {
          "/ten_packages/protocol",
          TEN_ADDON_TYPE_PROTOCOL,
          protocol_dependencies,
      },
  };

  for (int i = 0; i < sizeof(folders) / sizeof(folders[0]); i++) {
    ten_string_t module_path;
    ten_string_init(&module_path);
    ten_string_copy(&module_path, &app->base_dir);

    do {
      ten_string_append_formatted(&module_path, folders[i].path);

      if (ten_path_to_system_flavor(&module_path) != 0) {
        TEN_LOGE("Failed to convert path to system flavor: %s",
                 ten_string_get_raw_str(&module_path));

        success = false;
        break;
      }

      // The modules (e.g., extensions/protocols) do not exist if only the TEN
      // app has been installed.
      if (ten_path_exists(ten_string_get_raw_str(&module_path))) {
        load_all_dynamic_libraries(app, ten_string_get_raw_str(&module_path),
                                   folders[i].dependencies);
      }
    } while (0);

    ten_string_deinit(&module_path);

    if (!success) {
      goto done;
    }
  }

done:
  if (app_lib_path) {
    ten_string_destroy(app_lib_path);
  }

  return success;
}

bool ten_addon_load_all_from_ten_package_base_dirs(ten_app_t *app,
                                                   ten_error_t *err) {
  TEN_ASSERT(app && ten_app_check_integrity(app, true), "Invalid argument.");

  bool success = true;

  ten_list_foreach (&app->ten_package_base_dirs, iter) {
    ten_string_t *ten_package_base_dir = ten_str_listnode_get(iter.node);
    TEN_ASSERT(ten_package_base_dir &&
                   ten_string_check_integrity(ten_package_base_dir),
               "Should not happen.");

    TEN_LOGI("Load dynamic libraries under path: %s",
             ten_string_get_raw_str(ten_package_base_dir));

    TEN_ADDON_TYPE addon_type = TEN_ADDON_TYPE_INVALID;
    ten_string_t addon_name;
    ten_string_init(&addon_name);

    // Construct `manifest.json` path.
    ten_string_t manifest_json_file_path;
    ten_string_init_from_string(&manifest_json_file_path, ten_package_base_dir);
    ten_path_join_c_str(&manifest_json_file_path, TEN_STR_MANIFEST_JSON);

    // Get type and name from manifest file.
    ten_manifest_get_type_and_name(
        ten_string_get_raw_str(&manifest_json_file_path), &addon_type,
        &addon_name, NULL);

    ten_string_deinit(&manifest_json_file_path);

    ten_addon_load_from_base_dir(app,
                                 ten_string_get_raw_str(ten_package_base_dir));

    ten_string_deinit(&addon_name);
  }

  return success;
}

#endif
