//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/path.h"

#include <mach-o/dyld.h>
#include <stdlib.h>

#include "axis_utils/lib/string.h"

axis_string_t *axis_path_get_executable_path(void) {
  char *buf = NULL;
  uint32_t size = 256;
  axis_string_t *path = NULL;

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

  path = axis_string_create_formatted(buf);
  free(buf);

  axis_string_t *dir = axis_path_get_dirname(path);
  axis_string_destroy(path);

  return dir;
}
