//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/file_lock.h"

#include <errno.h>
#include <fcntl.h>

#include "axis_utils/log/log.h"

static int axis_file_lock_reg(int fd, int cmd, short type, off_t offset,
                             short whence, off_t len) {
  struct flock lock;

  lock.l_type = type;
  lock.l_start = offset;
  lock.l_whence = whence;
  lock.l_len = len;

  return fcntl(fd, cmd, &lock);
}

int axis_file_writew_lock(int fd) {
  // Lock all bytes in the file.
  int rc = axis_file_lock_reg(fd, F_SETLKW, F_WRLCK, 0, SEEK_SET, 0);
  if (rc == -1) {
    axis_LOGE("Failed to lock file: %d", errno);
  }
  return rc;
}

int axis_file_unlock(int fd) {
  int rc = axis_file_lock_reg(fd, F_SETLK, F_UNLCK, 0, SEEK_SET, 0);
  if (rc == -1) {
    axis_LOGE("Failed to unlock file: %d", errno);
  }
  return rc;
}
