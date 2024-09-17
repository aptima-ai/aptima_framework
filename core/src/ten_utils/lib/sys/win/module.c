//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "ten_utils/lib/module.h"

#include <Windows.h>

void *ten_module_load(const ten_string_t *name, int as_local) {
  (void)as_local;
  if (!name || ten_string_is_empty(name)) {
    return NULL;
  }

  // LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR: the directory that contains the DLL is
  // temporarily added to the beginning of the list of directories that are
  // searched for the DLL's dependencies. Directories in the standard search
  // path are not searched.
  return (void *)LoadLibraryExA(
      ten_string_get_raw_str(name), NULL,
      LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR);
}

int ten_module_close(void *handle) {
  return FreeLibrary((HMODULE)handle) ? 0 : -1;
}
