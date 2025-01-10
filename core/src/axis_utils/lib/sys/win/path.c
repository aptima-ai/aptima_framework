//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/path.h"

#include <Shlwapi.h>
#include <Windows.h>
#include <direct.h>
#include <io.h>
#include <stdlib.h>
#include <winbase.h>

#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"

axis_string_t *axis_path_get_cwd() {
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

  axis_string_t *ret = axis_string_create_formatted(buf);
  free(buf);
  return ret;

error:
  if (buf) {
    free(buf);
  }

  return NULL;
}

axis_string_t *axis_path_get_home_path() {
  char buf[1024] = {0};
  DWORD rc = GetEnvironmentVariableA("USERPROFILE", buf, 1024);

  if (rc == 0 || rc == 1024) {
    return NULL;
  }

  return axis_string_create_formatted("%.*s", rc, buf);
}

static axis_string_t *axis_path_get_binary_path(HMODULE self) {
  char *buf = NULL;
  size_t expect_size = MAX_PATH;
  axis_string_t *full_path = NULL;
  axis_string_t *dir = NULL;

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

  full_path = axis_string_create_formatted(buf);
  if (!full_path) {
    goto error;
  }

  free(buf);
  dir = axis_path_get_dirname(full_path);
  axis_string_destroy(full_path);
  if (!dir) {
    return NULL;
  }

  axis_string_t *abs = axis_path_realpath(dir);
  axis_string_destroy(dir);
  return abs;

error:
  if (buf) {
    free(buf);
  }

  if (full_path) {
    axis_string_destroy(full_path);
  }

  return NULL;
}

axis_string_t *axis_path_get_executable_path(void) {
  return axis_path_get_binary_path(NULL);
}

axis_string_t *axis_path_get_module_path(const void *addr) {
  HMODULE self = NULL;

  if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                              GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          (LPCSTR)addr, &self)) {
    goto error;
  }

  return axis_path_get_binary_path(self);

error:

  return NULL;
}

int axis_path_to_system_flavor(axis_string_t *path) {
  size_t len = 0;

  if (!path || axis_string_is_empty(path)) {
    return -1;
  }

  len = axis_string_len(path);

  for (size_t i = 0; i < len; i++) {
    if (path->buf[i] == '/') {
      path->buf[i] = '\\';
    }
  }

  return 0;
}

axis_string_t *axis_path_realpath(const axis_string_t *path) {
  char *buf = NULL;

  if (!path || axis_string_is_empty(path)) {
    return NULL;
  }

  buf = _fullpath(NULL, axis_string_get_raw_str(path), 0);
  if (buf == NULL) {
    return NULL;
  }

  axis_string_t *ret = axis_string_create_formatted(buf);
  free(buf);
  return ret;
}

int axis_path_is_dir(const axis_string_t *path) {
  DWORD stat;
  if (!path || axis_string_is_empty(path)) {
    return 0;
  }

  stat = GetFileAttributesA(axis_string_get_raw_str(path));
  if (stat == INVALID_FILE_ATTRIBUTES) {
    return 0;
  }

  return (stat & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

int axis_path_mkdir(const axis_string_t *path, int recursive) {
  if (!path || axis_string_is_empty(path)) {
    return -1;
  }

  if (axis_path_is_dir(path)) {
    return 0;
  }

  if (recursive) {
    axis_string_t *parent = axis_path_get_dirname(path);
    if (!parent) {
      return -1;
    }

    int ret = axis_path_mkdir(parent, 1);
    axis_string_destroy(parent);
    if (ret != 0) {
      return ret;
    }
  }

  if (axis_path_is_dir(path)) {
    return 0;
  }

  // The api returns nonzero if succeeds and zero if fails on Windows, which is
  // opposite to that on Linux.
  int ret = CreateDirectoryA(axis_string_get_raw_str(path), NULL);
  if (!ret) {
    return -1;
  }
  return 0;
}

int axis_path_create_temp_dir(const char *base_path,
                             axis_string_t *tmp_dir_path) {
  axis_ASSERT(base_path && tmp_dir_path, "Invalid argument.");

  axis_string_init_from_c_str(tmp_dir_path, base_path, strlen(base_path));
  axis_path_join_c_str(tmp_dir_path, "tmpdir.XXXXXX");

  _mktemp_s(axis_string_get_raw_str(tmp_dir_path),
            axis_string_len(tmp_dir_path) + 1);
  axis_path_mkdir(tmp_dir_path, 1);

  return 0;
}

int axis_path_exists(const char *path) {
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

struct axis_path_itor_t {
  WIN32_FIND_DATAA data;
  axis_dir_fd_t *dir;
};

struct axis_dir_fd_t {
  HANDLE dir;
  axis_path_itor_t itor;
  axis_string_t *path;
};

axis_dir_fd_t *axis_path_open_dir(const char *path) {
  axis_dir_fd_t *dir = NULL;

  if (!path || !strlen(path)) {
    goto error;
  }

  dir = (axis_dir_fd_t *)malloc(sizeof(axis_dir_fd_t));
  if (dir == NULL) {
    goto error;
  }

  memset(dir, 0, sizeof(axis_dir_fd_t));

  // An argument of "C:\Windows" returns information about the directory
  // "C:\Windows", not about a directory or file in "C:\Windows". To examine the
  // files and directories in "C:\Windows", use an lpFileName of "C:\Windows*"
  // or "C:\Windows\*".
  axis_string_t *search_path = NULL;

  if (axis_c_string_ends_with(path, "\\")) {
    search_path = axis_string_create_formatted("%s*", path);
  } else {
    search_path = axis_string_create_formatted("%s\\*", path);
  }

  dir->dir =
      FindFirstFileA(axis_string_get_raw_str(search_path), &dir->itor.data);
  axis_string_destroy(search_path);

  if (dir->dir == INVALID_HANDLE_VALUE) {
    goto error;
  }

  dir->path = axis_string_create_from_c_str(path, strlen(path));
  dir->itor.dir = dir;
  return dir;

error:
  axis_path_close_dir(dir);
  return NULL;
}

int axis_path_close_dir(axis_dir_fd_t *dir) {
  if (!dir) {
    return -1;
  }

  if (dir->dir) {
    FindClose(dir->dir);
  }

  if (dir->path) {
    axis_string_destroy(dir->path);
  }

  free(dir);
  return 0;
}

axis_path_itor_t *axis_path_get_first(axis_dir_fd_t *dir) {
  if (!dir) {
    return NULL;
  }

  if (dir->dir == INVALID_HANDLE_VALUE) {
    return NULL;
  }

  return &dir->itor;
}

axis_path_itor_t *axis_path_get_next(axis_path_itor_t *itor) {
  if (!itor) {
    return NULL;
  }

  if (!FindNextFileA(itor->dir->dir, &itor->data)) {
    return NULL;
  }

  return itor;
}

axis_string_t *axis_path_itor_get_name(axis_path_itor_t *itor) {
  if (!itor) {
    return NULL;
  }

  return axis_string_create_formatted(itor->data.cFileName);
}

axis_string_t *axis_path_itor_get_full_name(axis_path_itor_t *itor) {
  if (!itor) {
    return NULL;
  }

  axis_string_t *name = axis_path_itor_get_name(itor);
  if (!name) {
    return NULL;
  }

  axis_string_t *full_name = axis_string_clone(itor->dir->path);
  if (!full_name) {
    axis_string_destroy(name);
    return NULL;
  }

  axis_string_append_formatted(full_name, "/");
  axis_string_append_formatted(full_name, axis_string_get_raw_str(name));
  axis_string_destroy(name);

  axis_path_to_system_flavor(full_name);
  return full_name;
}

int axis_path_change_cwd(axis_string_t *dirname) {
  axis_ASSERT(dirname, "Invalid argument.");
  _chdir(axis_string_get_raw_str(dirname));
  return 0;
}

int axis_path_is_absolute(const axis_string_t *path) {
  if (!path || axis_string_is_empty(path)) {
    return 0;
  }

  const char *str = axis_string_get_raw_str(path);
  size_t len = axis_string_len(path);

  if (len >= 2) {
    // Check if it starts with double backslashes (UNC path).
    if (str[0] == '\\' && str[1] == '\\') {
      return 1;
    }

    // Check if it starts with a drive letter, such as "C:\".
    if (isalpha(str[0]) && str[1] == ':' && (str[2] == '\\' || str[2] == '/')) {
      return 1;
    }
  }

  // In other cases, treat it as a relative path.
  return 0;
}

int axis_path_make_symlink(const char *target, const char *linkpath) {
  axis_ASSERT(target && strlen(target) && linkpath && strlen(linkpath),
             "Invalid argument.");

  return !CreateSymbolicLinkA(linkpath, target,
                              SYMBOLIC_LINK_FLAG_DIRECTORY |
                                  SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE);
}

int axis_path_is_symlink(const char *path) {
  axis_ASSERT(path && strlen(path), "Invalid argument.");

  DWORD stat;
  stat = GetFileAttributesA(path);
  if (stat == INVALID_FILE_ATTRIBUTES) {
    return 0;
  }

  return (stat & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
}
