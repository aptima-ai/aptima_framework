//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "ten_utils/lib/path.h"

#include <mach-o/dyld.h>
#include <stdlib.h>

#include "ten_utils/lib/string.h"

ten_string_t *ten_path_get_executable_path(void) {
  char *buf = NULL;
  uint32_t size = 256;
  ten_string_t *path = NULL;

  buf = (char *)malloc(size);
  if (buf == NULL) {
    return NULL;
  }

  if (_NSGetExecutablePath(buf, &size) < 0 && size > 256) {
    free(buf);
    buf = (char *)malloc(size);
    if (buf == NULL) {
      return NULL;
    }

    if (_NSGetExecutablePath(buf, &size) < 0) {
      free(buf);
      return NULL;
    }
  }

  path = ten_string_create_formatted(buf);
  free(buf);

  ten_string_t *dir = ten_path_get_dirname(path);
  ten_string_destroy(path);

  return dir;
}
