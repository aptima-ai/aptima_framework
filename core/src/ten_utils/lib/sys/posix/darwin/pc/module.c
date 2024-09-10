//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/lib/module.h"

#include <dlfcn.h>

void *ten_module_load(const ten_string_t *name, int as_local) {
  return dlopen(name->buf, RTLD_NOW | (as_local ? RTLD_LOCAL : RTLD_GLOBAL));
}

int ten_module_close(void *handle) { return dlclose(handle); }