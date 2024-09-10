//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/lib/file_lock.h"

#include <Windows.h>
#include <fileapi.h>
#include <io.h>

static OVERLAPPED overlap = {0};

int ten_file_writew_lock(int fd) {
  // Lock the full file.
  if (LockFileEx((HANDLE)_get_osfhandle(fd), LOCKFILE_EXCLUSIVE_LOCK, 0,
                 MAXDWORD, MAXDWORD, &overlap)) {
    return 0;
  }
  return -1;
}

int ten_file_unlock(int fd) {
  if (UnlockFileEx((HANDLE)_get_osfhandle(fd), 0, MAXDWORD, MAXDWORD,
                   &overlap)) {
    return 0;
  }
  return -1;
}
