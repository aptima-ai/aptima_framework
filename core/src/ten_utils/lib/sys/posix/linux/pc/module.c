//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/lib/module.h"

#include "ten_utils/lib/string.h"

#define _GNU_SOURCE
#include <dlfcn.h>

#include "include_internal/ten_utils/log/log.h"

void *ten_module_load(const ten_string_t *name, int as_local) {
  if (!name || !ten_string_check_integrity(name)) {
    TEN_LOGE("Invalid argument: module name is null.");
    return NULL;
  }

  void *handle = dlopen(ten_string_get_raw_str(name),
                        RTLD_NOW | (as_local ? RTLD_LOCAL : RTLD_GLOBAL));

  if (!handle) {
    const char *err_msg = dlerror();
    TEN_LOGE("Failed to dlopen %s: %s", ten_string_get_raw_str(name),
             err_msg ? err_msg : "Unknown error");
  }

  return handle;
}

int ten_module_close(void *handle) {
  if (!handle) {
    TEN_LOGE("Invalid argument: handle is null.");
    return -1;
  }

  int result = dlclose(handle);
  if (result != 0) {
    const char *err_msg = dlerror();
    TEN_LOGE("Failed to dlclose handle: %s",
             err_msg ? err_msg : "Unknown error");
  }

  return result;
}
