//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/lib/file.h"

#include <assert.h>
#include <io.h>
#include <stdio.h>

#include "ten_utils/lib/path.h"

int ten_file_get_fd(FILE *fp) { return _fileno(fp); }

int ten_file_size(const char *filename) {
  assert(filename);

  assert(0 && "Need to implement this.");
  return -1;
}

int ten_file_chmod(const char *filename, uint32_t mode) {
  assert(filename);

  // TODO(ZhangXianyao): Need to implement.
  (void)mode;

  return 0;
}

int ten_file_clone_permission(const char *src_filename,
                              const char *dest_filename) {
  assert(src_filename && dest_filename);

  // TODO(ZhangXianyao): Need to implement.
  return 0;
}

int ten_file_clone_permission_by_fd(int src_fd, int dest_fd) {
  // TODO(ZhangXianyao): Need to implement.
  (void)src_fd;
  (void)dest_fd;

  return 0;
}

int ten_file_clear_open_file_content(FILE *fp) {
  assert(fp);

  rewind(fp);
  return _chsize(ten_file_get_fd(fp), 0);
}

char *ten_symlink_file_read(const char *path) {
  assert(path && ten_path_is_symlink(path));

  // TODO(ZhangXianyao): Need to implement.
  return NULL;
}

int ten_symlink_file_copy(const char *src_file, const char *dest_file) {
  assert(src_file && ten_path_is_symlink(src_file));

  // TODO(ZhangXianyao): Need to implement.
  return 0;
}
