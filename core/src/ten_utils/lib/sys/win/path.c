//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "ten_utils/lib/path.h"

#include <Shlwapi.h>
#include <Windows.h>
#include <direct.h>
#include <io.h>
#include <stdlib.h>
#include <winbase.h>

#include "ten_utils/macro/check.h"
#include "ten_utils/lib/string.h"

ten_string_t *ten_path_get_cwd() {
  char *buf = NULL;
  DWORD size = 0;

  size = GetCurrentDirectoryA(0, NULL);
  if (size == 0) {
    goto error;
  }

  buf = (char *)malloc(size);
  if (buf == NULL) {
    goto error;
  }

  size = GetCurrentDirectoryA(size, buf);
  if (size == 0) {
    goto error;
  }

  ten_string_t *ret = ten_string_create_formatted(buf);
  free(buf);
  return ret;

error:
  if (buf) {
    free(buf);
  }

  return NULL;
}

ten_string_t *ten_path_get_home_path() {
  char buf[1024] = {0};
  DWORD rc = GetEnvironmentVariableA("USERPROFILE", buf, 1024);

  if (rc == 0 || rc == 1024) {
    return NULL;
  }

  return ten_string_create_formatted("%.*s", rc, buf);
}

static ten_string_t *ten_path_get_binary_path(HMODULE self) {
  char *buf = NULL;
  size_t expect_size = MAX_PATH;
  ten_string_t *full_path = NULL;
  ten_string_t *dir = NULL;

  buf = (char *)malloc(expect_size);
  if (buf == NULL) {
    goto error;
  }

  expect_size = GetModuleFileNameA(self, buf, expect_size);
  if (expect_size > MAX_PATH) {
    free(buf);

    buf = (char *)malloc(expect_size);
    if (buf == NULL) {
      goto error;
    }

    if (GetModuleFileNameA(self, buf, expect_size) == 0) {
      goto error;
    }
  }

  full_path = ten_string_create_formatted(buf);
  if (!full_path) {
    goto error;
  }

  free(buf);
  dir = ten_path_get_dirname(full_path);
  ten_string_destroy(full_path);
  if (!dir) {
    return NULL;
  }

  ten_string_t *abs = ten_path_realpath(dir);
  ten_string_destroy(dir);
  return abs;

error:
  if (buf) {
    free(buf);
  }

  if (full_path) {
    ten_string_destroy(full_path);
  }

  return NULL;
}

ten_string_t *ten_path_get_executable_path() {
  return ten_path_get_binary_path(NULL);
}

ten_string_t *ten_path_get_module_path(const void *addr) {
  HMODULE self = NULL;

  if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                              GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          (LPCSTR)addr, &self)) {
    goto error;
  }

  return ten_path_get_binary_path(self);

error:

  return NULL;
}

int ten_path_to_system_flavor(ten_string_t *path) {
  size_t len = 0;

  if (!path || ten_string_is_empty(path)) {
    return -1;
  }

  len = ten_string_len(path);

  for (size_t i = 0; i < len; i++) {
    if (path->buf[i] == '/') {
      path->buf[i] = '\\';
    }
  }

  return 0;
}

ten_string_t *ten_path_realpath(const ten_string_t *path) {
  char *buf = NULL;

  if (!path || ten_string_is_empty(path)) {
    return NULL;
  }

  buf = _fullpath(NULL, ten_string_get_raw_str(path), 0);
  if (buf == NULL) {
    return NULL;
  }

  ten_string_t *ret = ten_string_create_formatted(buf);
  free(buf);
  return ret;
}

int ten_path_is_dir(const ten_string_t *path) {
  DWORD stat;
  if (!path || ten_string_is_empty(path)) {
    return 0;
  }

  stat = GetFileAttributesA(ten_string_get_raw_str(path));
  if (stat == INVALID_FILE_ATTRIBUTES) {
    return 0;
  }

  return (stat & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

int ten_path_mkdir(const ten_string_t *path, int recursive) {
  if (!path || ten_string_is_empty(path)) {
    return -1;
  }

  if (ten_path_is_dir(path)) {
    return 0;
  }

  if (recursive) {
    ten_string_t *parent = ten_path_get_dirname(path);
    if (!parent) {
      return -1;
    }

    int ret = ten_path_mkdir(parent, 1);
    ten_string_destroy(parent);
    if (ret != 0) {
      return ret;
    }
  }

  if (ten_path_is_dir(path)) {
    return 0;
  }

  // The api returns nonzero if succeeds and zero if fails on Windows, which is
  // opposite to that on Linux.
  int ret = CreateDirectoryA(ten_string_get_raw_str(path), NULL);
  if (!ret) {
    return -1;
  }
  return 0;
}

int ten_path_create_temp_dir(const char *base_path,
                             ten_string_t *tmp_dir_path) {
  TEN_ASSERT(base_path && tmp_dir_path, "Invalid argument.");

  ten_string_copy_c_str(tmp_dir_path, base_path, strlen(base_path));
  ten_path_join_c_str(tmp_dir_path, "tmpdir.XXXXXX");

  _mktemp_s(ten_string_get_raw_str(tmp_dir_path),
            ten_string_len(tmp_dir_path) + 1);
  ten_path_mkdir(tmp_dir_path, 1);

  return 0;
}

int ten_path_exists(const char *path) {
  DWORD stat;
  if (!path || !*path) {
    return 0;
  }

  stat = GetFileAttributesA(path);
  if (stat == INVALID_FILE_ATTRIBUTES) {
    return 0;
  }

  return 1;
}

struct ten_path_itor_t {
  WIN32_FIND_DATAA data;
  ten_dir_fd_t *dir;
};

struct ten_dir_fd_t {
  HANDLE dir;
  ten_path_itor_t itor;
  ten_string_t *path;
};

ten_dir_fd_t *ten_path_open_dir(const char *path) {
  ten_dir_fd_t *dir = NULL;

  if (!path || !strlen(path)) {
    goto error;
  }

  dir = (ten_dir_fd_t *)malloc(sizeof(ten_dir_fd_t));
  if (dir == NULL) {
    goto error;
  }

  memset(dir, 0, sizeof(ten_dir_fd_t));

  // An argument of "C:\Windows" returns information about the directory
  // "C:\Windows", not about a directory or file in "C:\Windows". To examine the
  // files and directories in "C:\Windows", use an lpFileName of "C:\Windows*"
  // or "C:\Windows\*".
  ten_string_t *search_path = NULL;

  if (ten_c_string_ends_with(path, "\\")) {
    search_path = ten_string_create_formatted("%s*", path);
  } else {
    search_path = ten_string_create_formatted("%s\\*", path);
  }

  dir->dir =
      FindFirstFileA(ten_string_get_raw_str(search_path), &dir->itor.data);
  ten_string_destroy(search_path);

  if (dir->dir == INVALID_HANDLE_VALUE) {
    goto error;
  }

  dir->path = ten_string_create_from_c_str(path, strlen(path));
  dir->itor.dir = dir;
  return dir;

error:
  ten_path_close_dir(dir);
  return NULL;
}

int ten_path_close_dir(ten_dir_fd_t *dir) {
  if (!dir) {
    return -1;
  }

  if (dir->dir) {
    FindClose(dir->dir);
  }

  if (dir->path) {
    ten_string_destroy(dir->path);
  }

  free(dir);
  return 0;
}

ten_path_itor_t *ten_path_get_first(ten_dir_fd_t *dir) {
  if (!dir) {
    return NULL;
  }

  if (dir->dir == INVALID_HANDLE_VALUE) {
    return NULL;
  }

  return &dir->itor;
}

ten_path_itor_t *ten_path_get_next(ten_path_itor_t *itor) {
  if (!itor) {
    return NULL;
  }

  if (!FindNextFileA(itor->dir->dir, &itor->data)) {
    return NULL;
  }

  return itor;
}

ten_string_t *ten_path_itor_get_name(ten_path_itor_t *itor) {
  if (!itor) {
    return NULL;
  }

  return ten_string_create_formatted(itor->data.cFileName);
}

ten_string_t *ten_path_itor_get_full_name(ten_path_itor_t *itor) {
  if (!itor) {
    return NULL;
  }

  ten_string_t *name = ten_path_itor_get_name(itor);
  if (!name) {
    return NULL;
  }

  ten_string_t *full_name = ten_string_clone(itor->dir->path);
  if (!full_name) {
    ten_string_destroy(name);
    return NULL;
  }

  ten_string_append_formatted(full_name, "/");
  ten_string_append_formatted(full_name, ten_string_get_raw_str(name));
  ten_string_destroy(name);

  ten_path_to_system_flavor(full_name);
  return full_name;
}

int ten_path_change_cwd(ten_string_t *dirname) {
  TEN_ASSERT(dirname, "Invalid argument.");
  _chdir(ten_string_get_raw_str(dirname));
  return 0;
}

int ten_path_is_absolute(const ten_string_t *path) {
  return PathIsRelativeA(ten_string_get_raw_str(path)) == TRUE ? 0 : 1;
}

int ten_path_make_symlink(const char *target, const char *linkpath) {
  TEN_ASSERT(target && strlen(target) && linkpath && strlen(linkpath),
             "Invalid argument.");

  return !CreateSymbolicLinkA(linkpath, target,
                              SYMBOLIC_LINK_FLAG_DIRECTORY |
                                  SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE);
}

int ten_path_is_symlink(const char *path) {
  TEN_ASSERT(path && strlen(path), "Invalid argument.");

  DWORD stat;
  stat = GetFileAttributesA(path);
  if (stat == INVALID_FILE_ATTRIBUTES) {
    return 0;
  }

  return (stat & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
}
