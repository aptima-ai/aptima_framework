//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/file.h"

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "axis_utils/lib/path.h"
#include "axis_utils/log/log.h"

// Mac OS X 10.6 does not support O_CLOEXEC.
#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif

// Mac OS does not support FD_CLOEXEC.
#ifndef FD_CLOEXEC
#define FD_CLOEXEC 1
#endif

int axis_file_get_fd(FILE *fp) { return fileno(fp); }

int axis_file_size(const char *filename) {
  assert(filename);

  struct stat file_info;
  if (stat(filename, &file_info) == 0) {
    return file_info.st_size;
  }
  return -1;
}

int axis_file_chmod(const char *filename, uint32_t mode) {
  assert(filename);

  return chmod(filename, mode);
}

int axis_file_clone_permission(const char *src_filename,
                              const char *dest_filename) {
  assert(src_filename && dest_filename);

  int result = 0;
  struct stat file_stat;

  result = stat(src_filename, &file_stat);
  if (result) {
    return result;
  }

  result = chmod(dest_filename, file_stat.st_mode);

  return result;
}

int axis_file_clone_permission_by_fd(int src_fd, int dest_fd) {
  int result = 0;
  struct stat file_stat;

  result = fstat(src_fd, &file_stat);
  if (result) {
    return result;
  }

  result = fchmod(dest_fd, file_stat.st_mode);

  return result;
}

int axis_file_clear_open_file_content(FILE *fp) {
  assert(fp);

  rewind(fp);
  return ftruncate(axis_file_get_fd(fp), 0);
}

char *axis_symlink_file_read(const char *path) {
  assert(path && axis_path_exists(path) && axis_path_is_symlink(path));

  char *buf = NULL;
  struct stat st;
  if (lstat(path, &st) == 0) {
    long len = st.st_size + 1;
    buf = malloc(len);
    ssize_t rc = readlink(path, buf, len);
    assert(rc == len - 1);

    buf[len - 1] = '\0';
  }

  return buf;
}

int axis_symlink_file_copy(const char *src_file, const char *dest_file) {
  assert(src_file && axis_path_is_symlink(src_file));

  char *link_target = axis_symlink_file_read(src_file);
  if (link_target) {
    int rc = axis_path_make_symlink(link_target, dest_file);
    free(link_target);
    return rc;
  }

  return -1;
}

/**
 * @brief Open a file for reading.
 */
int axis_file_open(const char *filename, bool *does_not_exist) {
  if (does_not_exist != NULL) {
    *does_not_exist = false;
  }

  int fd = open(filename, (O_RDONLY | O_CLOEXEC));
  if (fd < 0) {
    if (does_not_exist != NULL) {
      *does_not_exist = true;
    } else {
      axis_LOGE("Failed to open %s", filename);
    }

    return -1;
  }

  // Set FD_CLOEXEC just in case the kernel does not support O_CLOEXEC. It
  // doesn't matter if this fails for some reason.
  //
  // TODO(Wei): At some point it should be safe to only do this if O_CLOEXEC ==
  // 0.
  fcntl(fd, F_SETFD, FD_CLOEXEC);

  return fd;
}

/**
 * @brief Close @a fd.
 */
bool axis_file_close(int fd) {
  if (close(fd) < 0) {
    axis_LOGE("Failed to close %d", fd);
    return false;
  }
  return true;
}
