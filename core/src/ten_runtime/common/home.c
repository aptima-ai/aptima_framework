//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/common/home.h"

#include <stdlib.h>

#include "include_internal/ten_runtime/app/base_dir.h"
#include "ten_utils/lib/path.h"
#include "ten_utils/lib/string.h"

ten_string_t *ten_get_app_base_dir(void) {
  ten_string_t *app_base_dir = NULL;

  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  const char *ten_app_base_dir_env_var = getenv("TEN_APP_BASE_DIR");
  if (ten_app_base_dir_env_var && strlen(ten_app_base_dir_env_var) > 0) {
    app_base_dir = ten_string_create_formatted("%s", ten_app_base_dir_env_var);
  } else {
    // The `ten_path_get_module_path()` function returns the base directory of
    // `libten_runtime.so`.
    //
    // Note that we can not use `ten_path_get_executable_path()` here, as the
    // actually executable in some language is not the TEN app. Ex: the
    // `ten_path_get_executable_path()` returns `/usr/bin` if we start a python
    // APP with `python3 bin/main.py`. In this case, the `/usr/bin` is the
    // location of `python3`.
    ten_string_t *path = ten_path_get_module_path((void *)ten_get_app_base_dir);
    if (!path) {
      TEN_LOGE(
          "Could not get app base dir from module path, using TEN_APP_BASE_DIR "
          "instead.");
      return NULL;
    }

    ten_app_find_base_dir(path, &app_base_dir);
    ten_string_destroy(path);
  }

  if (!app_base_dir) {
    TEN_LOGE(
        "Could not get app home from module path, using TEN_APP_BASE_DIR "
        "instead.");
    return NULL;
  }

  ten_path_to_system_flavor(app_base_dir);
  return app_base_dir;
}
