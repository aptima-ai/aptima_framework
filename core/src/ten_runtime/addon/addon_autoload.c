//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/addon/addon_autoload.h"

#include "ten_runtime/common/errno.h"

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
#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/global/global.h"
#include "include_internal/ten_runtime/global/signal.h"
#include "include_internal/ten_runtime/metadata/manifest.h"
#include "include_internal/ten_utils/log/log.h"
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

static void load_all_dynamic_libraries_under_path(const char *path) {
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

static void ten_addon_load_from_base_dir(const char *path) {
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
  load_all_dynamic_libraries_under_path(ten_string_get_raw_str(&lib_dir));

done:
  ten_string_deinit(&lib_dir);
}

static void load_all_dynamic_libraries(const char *path) {
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

    cur = ten_path_itor_get_full_name(itor);
    if (!cur) {
      TEN_LOGE("Failed to get full name under path: %s", path);
      goto continue_loop;
    }

    ten_addon_load_from_base_dir(ten_string_get_raw_str(cur));

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

bool ten_addon_load_all_from_app_base_dir(const char *app_base_dir,
                                          ten_error_t *err) {
  TEN_ASSERT(app_base_dir, "Invalid argument.");

  bool success = true;

  // The extension is loaded dynamically and will only be loaded when it's
  // needed. Both the protocol and addon_loader are preloaded, so when the
  // app starts, it will scan these two folders and load all the addons inside
  // them.
  struct {
    const char *path;
  } folders[] = {
      {"/ten_packages/protocol"},
      {"/ten_packages/addon_loader"},
  };

  for (int i = 0; i < sizeof(folders) / sizeof(folders[0]); i++) {
    ten_string_t module_path;
    ten_string_init_from_c_str(&module_path, app_base_dir,
                               strlen(app_base_dir));

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
        load_all_dynamic_libraries(ten_string_get_raw_str(&module_path));
      }
    } while (0);

    ten_string_deinit(&module_path);

    if (!success) {
      goto done;
    }
  }

done:
  return success;
}

bool ten_addon_load_all_from_ten_package_base_dirs(
    ten_list_t *ten_package_base_dirs, ten_error_t *err) {
  TEN_ASSERT(ten_package_base_dirs, "Invalid argument.");

  bool success = true;

  ten_list_foreach (ten_package_base_dirs, iter) {
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

    ten_addon_load_from_base_dir(ten_string_get_raw_str(ten_package_base_dir));

    ten_string_deinit(&addon_name);
  }

  return success;
}

bool ten_addon_load_specific_addon_from_app_base_dir(const char *app_base_dir,
                                                     TEN_ADDON_TYPE addon_type,
                                                     const char *addon_name,
                                                     ten_error_t *err) {
  TEN_ASSERT(app_base_dir, "Invalid argument.");
  TEN_ASSERT(addon_name, "Addon name cannot be NULL.");

  bool success = true;
  ten_string_t addon_lib_folder_path;

  // Construct the path to the specific addon lib/ folder.
  ten_string_init_from_c_str(&addon_lib_folder_path, app_base_dir,
                             strlen(app_base_dir));

  ten_string_append_formatted(&addon_lib_folder_path, "/ten_packages/%s/%s/lib",
                              ten_addon_type_to_string(addon_type), addon_name);
  if (ten_path_to_system_flavor(&addon_lib_folder_path) != 0) {
    TEN_LOGE("Failed to convert path to system flavor: %s",
             ten_string_get_raw_str(&addon_lib_folder_path));
    success = false;
    goto done;
  }

  if (!ten_path_exists(ten_string_get_raw_str(&addon_lib_folder_path)) ||
      !ten_path_is_dir(&addon_lib_folder_path)) {
    TEN_LOGE("Addon lib/ folder does not exist or is not a directory: %s",
             ten_string_get_raw_str(&addon_lib_folder_path));
    success = false;
    goto done;
  }

  // Load the library from the 'lib/' directory.
  load_all_dynamic_libraries_under_path(
      ten_string_get_raw_str(&addon_lib_folder_path));

done:
  ten_string_deinit(&addon_lib_folder_path);

  if (!success && err) {
    ten_error_set(err, TEN_ERRNO_GENERIC,
                  "Failed to load specific addon: %s:%s",
                  ten_addon_type_to_string(addon_type), addon_name);
  }

  return success;
}

bool ten_addon_register_specific_addon(TEN_ADDON_TYPE addon_type,
                                       const char *addon_name,
                                       ten_addon_register_ctx_t *register_ctx,
                                       ten_error_t *err) {
  TEN_ASSERT(addon_name, "Invalid argument.");

  ten_addon_manager_t *manager = ten_addon_manager_get_instance();

  bool success = ten_addon_manager_register_specific_addon(
      manager, addon_type, addon_name, (void *)register_ctx);

  if (!success && err) {
    ten_error_set(err, TEN_ERRNO_GENERIC,
                  "Failed to register specific addon: %s:%s",
                  ten_addon_type_to_string(addon_type), addon_name);
  }

  return success;
}

#endif
