//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/common/base_dir.h"

#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/common/constant_str.h"
#include "axis_utils/lib/file.h"
#include "axis_utils/lib/path.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"

axis_string_t *axis_find_app_base_dir(void) {
  axis_string_t *app_base_dir = NULL;

  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  const char *axis_app_base_dir_env_var = getenv("axis_APP_BASE_DIR");
  if (axis_app_base_dir_env_var && strlen(axis_app_base_dir_env_var) > 0) {
    app_base_dir = axis_string_create_formatted("%s", axis_app_base_dir_env_var);
    axis_path_to_system_flavor(app_base_dir);
  } else {
    // The following `axis_path_get_module_path()` function returns the base
    // directory of `libaxis_runtime.so`.
    //
    // Note that we can not use `axis_path_get_executable_path()` here, as the
    // actually executable in some language is not the TEN app. Ex: the
    // `axis_path_get_executable_path()` returns `/usr/bin` if we start a python
    // APP with `python3 bin/main.py`. In this case, the `/usr/bin` is the
    // location of `python3`.
    axis_string_t *module_path =
        axis_path_get_module_path((void *)axis_find_app_base_dir);
    if (!module_path) {
      axis_LOGE(
          "Could not get app base dir from module path, using axis_APP_BASE_DIR "
          "instead.");
      return NULL;
    }

    app_base_dir = axis_find_base_dir(axis_string_get_raw_str(module_path),
                                     axis_STR_APP, NULL);
    axis_string_destroy(module_path);
  }

  if (!app_base_dir || axis_string_is_empty(app_base_dir)) {
    axis_LOGW(
        "Could not get app home from module path, using axis_APP_BASE_DIR "
        "instead.");
    return NULL;
  }

  return app_base_dir;
}

void axis_app_find_and_set_base_dir(axis_app_t *self) {
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Should not happen.");

  axis_string_t *app_base_dir = axis_find_app_base_dir();
  if (!app_base_dir) {
    axis_LOGD("Failed to determine app base directory.");
    return;
  }

  axis_string_copy(&self->base_dir, app_base_dir);
  axis_string_destroy(app_base_dir);
}

const char *axis_app_get_base_dir(axis_app_t *self) {
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: This function might be called from other threads, ex: the
  // extension thread. And the `base_dir` is only set when starting the app, so
  // it's thread safe to read after app starts.
  axis_ASSERT(self && axis_app_check_integrity(self, false), "Invalid argument.");

  return axis_string_get_raw_str(&self->base_dir);
}
