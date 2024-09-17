//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/lib/path.h"

#include <mach-o/dyld.h>
#include <stdlib.h>

#include "ten_utils/macro/check.h"
#include "ten_utils/lib/string.h"

ten_string_t *ten_path_get_executable_path() {
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
