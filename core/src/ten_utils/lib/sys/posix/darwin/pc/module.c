//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "ten_utils/lib/module.h"

#include <dlfcn.h>

#include "ten_utils/lib/string.h"
#include "ten_utils/log/log.h"

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

void *ten_module_get_symbol(void *handle, const char *symbol_name) {
  if (!handle) {
    TEN_LOGE("Invalid argument: handle is null.");
    return NULL;
  }

  if (!symbol_name) {
    TEN_LOGE("Invalid argument: symbol name is null or corrupted.");
    return NULL;
  }

  // Clear previous errors.
  dlerror();

  void *symbol = dlsym(handle, symbol_name);

  const char *error = dlerror();
  if (error != NULL) {
    // Enable the code below if debugging is needed.
#if 0
    TEN_LOGD("Failed to find symbol %s: %s", symbol_name, error);
#endif
    return NULL;
  }

  return symbol;
}
