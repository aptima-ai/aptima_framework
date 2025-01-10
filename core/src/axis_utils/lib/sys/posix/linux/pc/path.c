//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/path.h"

#include <stdlib.h>
#include <unistd.h>

axis_string_t *axis_path_get_executable_path(void) {
  char *buf = NULL;
  ssize_t len = 256;
  ssize_t ret = 0;
  axis_string_t *path = NULL;

  for (;;) {
    buf = (char *)malloc(len);
    ret = readlink("/proc/self/exe", buf, len);

    if (ret < 0) {
      free(buf);
      buf = NULL;
      break;
    }

    if (ret < len) {
      buf[ret] = '\0';
      break;
    }

    free(buf);
    len += 256;
  }

  if (!buf) {
    return NULL;
  }

  path = axis_string_create_formatted(buf);
  free(buf);

  axis_string_t *dir = axis_path_get_dirname(path);
  axis_string_destroy(path);

  return dir;
}
