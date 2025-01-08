//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/file.h"

#include <assert.h>
#include <io.h>
#include <stdio.h>

#include "axis_utils/lib/path.h"

int axis_file_get_fd(FILE *fp) { return _fileno(fp); }

int axis_file_size(const char *filename) {
  assert(filename);

  assert(0 && "Need to implement this.");
  return -1;
}

int axis_file_chmod(const char *filename, uint32_t mode) {
  assert(filename);

  // TODO(ZhangXianyao): Need to implement.
  (void)mode;

  return 0;
}

int axis_file_clone_permission(const char *src_filename,
                              const char *dest_filename) {
  assert(src_filename && dest_filename);

  // TODO(ZhangXianyao): Need to implement.
  return 0;
}

int axis_file_clone_permission_by_fd(int src_fd, int dest_fd) {
  // TODO(ZhangXianyao): Need to implement.
  (void)src_fd;
  (void)dest_fd;

  return 0;
}

int axis_file_clear_open_file_content(FILE *fp) {
  assert(fp);

  rewind(fp);
  return _chsize(axis_file_get_fd(fp), 0);
}

char *axis_symlink_file_read(const char *path) {
  assert(path && axis_path_is_symlink(path));

  // TODO(ZhangXianyao): Need to implement.
  return NULL;
}

int axis_symlink_file_copy(const char *src_file, const char *dest_file) {
  assert(src_file && axis_path_is_symlink(src_file));

  // TODO(ZhangXianyao): Need to implement.
  return 0;
}
