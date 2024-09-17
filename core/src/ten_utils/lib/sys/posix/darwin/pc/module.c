//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "ten_utils/lib/module.h"

#include <dlfcn.h>

void *ten_module_load(const ten_string_t *name, int as_local) {
  return dlopen(name->buf, RTLD_NOW | (as_local ? RTLD_LOCAL : RTLD_GLOBAL));
}

int ten_module_close(void *handle) { return dlclose(handle); }