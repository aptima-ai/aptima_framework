//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/addon/addon_autoload.h"

#include "ten_runtime/app/app.h"
#include "ten_utils/container/list.h"
#include "ten_utils/container/list_node_str.h"
#include "ten_utils/lib/error.h"

#if defined(OS_LINUX)
#define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

#include "include_internal/ten_runtime/app/app.h"
#include "include_internal/ten_runtime/global/global.h"
#include "include_internal/ten_runtime/global/signal.h"
#include "include_internal/ten_utils/log/log.h"
#include "ten_utils/lib/module.h"
#include "ten_utils/lib/path.h"
#include "ten_utils/lib/string.h"

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

    if (ten_path_to_system_flavor(file_path) != 0) {
      TEN_LOGE("Failed to convert path to system flavor: %s",
               ten_string_get_raw_str(file_path));
      goto continue_loop;
    }

    if (ten_module_load(file_path, 1) == 0) {
      TEN_LOGE("Failed to load module: %s", ten_string_get_raw_str(file_path));
      goto continue_loop;
    }

    TEN_LOGI("Loaded module: %s", ten_string_get_raw_str(file_path));

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
  ten_string_t self;
  ten_string_init_formatted(&self, "%s", path);

  if (!ten_path_is_dir(&self)) {
    goto done;
  }

  ten_string_append_formatted(&self, "/lib");
  if (ten_path_to_system_flavor(&self) != 0) {
    TEN_LOGE("Failed to convert path to system flavor: %s",
             ten_string_get_raw_str(&self));
    goto done;
  }

  if (!ten_path_exists(ten_string_get_raw_str(&self)) ||
      !ten_path_is_dir(&self)) {
    TEN_LOGD("The dynamic library path(%s) does not exist.",
             ten_string_get_raw_str(&self));
    goto done;
  }

  // Load self first.
  load_all_dynamic_libraries_under_path(ten_string_get_raw_str(&self));

done:
  ten_string_deinit(&self);
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

typedef struct addon_folder_t {
  const char *path;
} addon_folder_t;

void ten_addon_load_from_path(const char *path) {
  TEN_ASSERT(path, "Invalid argument.");
  load_all_dynamic_libraries(path);
}

bool ten_addon_load_all_from_app_base_dir(ten_app_t *app, ten_error_t *err) {
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

  static const addon_folder_t folders[] = {
      {"/ten_packages/extension"},
      {"/ten_packages/extension_group"},
      {"/ten_packages/protocol"},
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
        load_all_dynamic_libraries(ten_string_get_raw_str(&module_path));
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

    ten_addon_load_from_base_dir(ten_string_get_raw_str(ten_package_base_dir));
  }

  return success;
}

#endif
