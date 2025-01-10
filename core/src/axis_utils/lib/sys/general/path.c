//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/path.h"

#include <ctype.h>
#include <stdlib.h>

#include "axis_utils/lib/file.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"

#if defined(_WIN32)
#include <Windows.h>
#include <fileapi.h>
#endif

int axis_path_is_special_dir(const axis_string_t *path) {
  axis_ASSERT(path, "Invalid argument.");

  if (!path || axis_string_is_empty(path)) {
    return -1;
  }

  axis_string_t *filename = axis_path_get_filename(path);
  if (!filename) {
    return -1;
  }

  int ret = 0;
  if ((axis_string_len(filename) == 1 &&
       axis_string_is_equal_c_str(filename, ".")) ||
      (axis_string_len(filename) == 2 &&
       axis_string_is_equal_c_str(filename, ".."))) {
    ret = 1;
  }

  axis_string_destroy(filename);

  return ret;
}

axis_string_t *axis_path_get_dirname(const axis_string_t *path) {
  axis_ASSERT(path, "Invalid argument.");

  if (!path || axis_string_is_empty(path)) {
    return NULL;
  }

  const char *raw_path = axis_string_get_raw_str(path);
  char *last_slash = strrchr(raw_path, '/');
#if defined(_WIN32)
  if (!last_slash) {
    last_slash = strrchr(raw_path, '\\');
  }
#endif

  if (!last_slash) {
    // There is no directory part.
    return NULL;
  }

  if (last_slash == raw_path) {
    // It is the root directory.
    return axis_string_create_formatted("/");
  }

  size_t dir_len = last_slash - raw_path;
  return axis_string_create_formatted("%.*s", dir_len, raw_path);
}

axis_string_t *axis_path_get_last_part(const axis_string_t *path) {
  axis_ASSERT(path, "Invalid argument.");

  if (!path || axis_string_is_empty(path)) {
    return NULL;
  }

  const char *raw_path = axis_string_get_raw_str(path);
  char *last_slash = strrchr(raw_path, '/');
#if defined(_WIN32)
  if (!last_slash) {
    last_slash = strrchr(raw_path, '\\');
  }
#endif

  axis_string_t *last_part = NULL;
  if (last_slash && *(last_slash + 1) == '\0') {
    axis_string_t temp;
    axis_string_init_formatted(&temp, "%.*s", axis_string_len(path) - 1,
                              axis_string_get_raw_str(path));
    last_part = axis_path_get_last_part(&temp);
    axis_string_deinit(&temp);
  } else {
    last_part = last_slash ? axis_string_create_formatted(last_slash + 1)
                           : axis_string_clone(path);
  }

  return last_part;
}

axis_string_t *axis_path_get_extension(const axis_string_t *path) {
  axis_ASSERT(path, "Invalid argument.");

  if (!path || axis_string_is_empty(path)) {
    return NULL;
  }

  const char *raw_path = axis_string_get_raw_str(path);
  const char *last_dot = strrchr(raw_path, '.');

  // If no '.' is found or '.' is the first character in the path, return NULL.
  if (!last_dot || last_dot == raw_path) {
    return NULL;
  }

  // Check if there are characters after the '.'. If not, return an empty
  // string.
  if (*(last_dot + 1) == '\0') {
    // Return an empty string as the extension.
    return axis_string_create();
  }

  // Return the extension, which is the substring after '.'
  return axis_string_create_formatted(last_dot + 1);
}

axis_string_t *axis_path_get_filename(const axis_string_t *path) {
  axis_ASSERT(path, "Invalid argument.");

  if (!path || axis_string_is_empty(path)) {
    return NULL;
  }

  char *last_slash = strrchr(path->buf, '/');
#if defined(_WIN32)
  if (!last_slash) {
    last_slash = strrchr(path->buf, '\\');
  }
#endif

  if (last_slash && *(last_slash + 1) == '\0') {
    // The path represents a folder, not a file.
    axis_string_t *file = axis_string_create();
    axis_ASSERT(file, "Failed to allocate memory.");
    return file;
  }

  axis_string_t *filename =
      axis_string_create_formatted(last_slash ? last_slash + 1 : path->buf);
  axis_ASSERT(filename, "Failed to allocate memory.");

  return filename;
}

static int __stricmp(const char *s1, const char *s2) {
  while (*s1 && *s2 && tolower(*s1) == tolower(*s2)) {
    s1++;
    s2++;
  }

  // If the loop ends, either s1 or s2 has reached the end or the characters
  // differ.
  return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

int axis_path_is_shared_library(const axis_string_t *path) {
  axis_ASSERT(path, "Invalid argument.");

  if (!path || axis_string_is_empty(path)) {
    return 0;
  }

  const char *extensions[] = {".so", ".dll", ".dylib"};
  size_t ext_lengths[] = {3, 4, 6};
  size_t len = axis_string_len((axis_string_t *)path);

  for (int i = 0; i < 3; i++) {
    if (len > ext_lengths[i]) {
      const char *ext = path->buf + len - ext_lengths[i];
      if (__stricmp(ext, extensions[i]) == 0) {
        return 1;
      }
    }
  }

  return 0;
}

int axis_path_join_c_str(axis_string_t *self, const char *other) {
  axis_ASSERT(self && other, "Invalid argument.");

  if (!self || !other) {
    axis_ASSERT(0, "Invalid argument.");
    return -1;
  }

  axis_string_append_formatted(self, "/%s", other);
  axis_path_to_system_flavor(self);

  axis_string_t *realpath = axis_path_realpath(self);
  if (!realpath) {
    return -1;
  }

  axis_string_set_formatted(self, "%s", axis_string_get_raw_str(realpath));
  axis_string_destroy(realpath);

  return 0;
}
