//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#if defined(OS_LINUX)
#define _GNU_SOURCE
#endif

#include "axis_utils/lib/path.h"

#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>

#include "include_internal/axis_utils/log/log.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"

int axis_path_to_system_flavor(axis_string_t *path) {
  axis_ASSERT(path, "Invalid argument.");

  if (!path || axis_string_is_empty(path)) {
    return -1;
  }

  size_t len = axis_string_len(path);

#if defined(OS_WINDOWS)
  // Platform-specific handling: Convert '\' to '/' on Windows
  for (size_t i = 0; i < len; i++) {
    if (path->buf[i] == '\\') {
      path->buf[i] = '/';
    }
  }
#endif

  return 0;
}

axis_string_t *axis_path_get_cwd(void) {
  char *buf = axis_MALLOC(MAXPATHLEN);
  axis_ASSERT(buf, "Failed to allocate memory.");

  if (!getcwd(buf, MAXPATHLEN)) {
    axis_LOGE("Failed to get cwd: %d", errno);
    axis_FREE(buf);
    return NULL;
  }

  axis_string_t *ret = axis_string_create_formatted(buf);
  axis_FREE(buf);

  return ret;
}

axis_string_t *axis_path_get_home_path(void) {
  char *home = getenv("HOME");
  if (!home) {
    axis_LOGE("Failed to get HOME environment variable: %d", errno);
    return NULL;
  }

  return axis_string_create_formatted("%s", home);
}

axis_string_t *axis_path_get_module_path(const void *addr) {
  axis_ASSERT(addr, "Invalid argument.");

  Dl_info info;

  if (dladdr(addr, &info) == 0) {
    axis_LOGE("Failed to dladdr: %d", errno);
    axis_ASSERT(0, "Should not happen.");
    return NULL;
  }

  axis_string_t *full_path = axis_string_create_formatted(info.dli_fname);
  if (!full_path) {
    axis_LOGE("Failed to get full path of %s", info.dli_fname);
    axis_ASSERT(0, "Should not happen.");
    return NULL;
  }

  axis_string_t *dir = axis_path_get_dirname(full_path);
  axis_string_destroy(full_path);
  if (!dir) {
    axis_LOGE("Failed to get folder of %s", axis_string_get_raw_str(full_path));
    axis_ASSERT(0, "Should not happen.");
    return NULL;
  }

  axis_string_t *abs = axis_path_realpath(dir);
  axis_string_destroy(dir);
  if (!abs) {
    axis_LOGE("Failed to get realpath of %s", axis_string_get_raw_str(dir));
    axis_ASSERT(0, "Should not happen.");
    return NULL;
  }

  return abs;
}

axis_string_t *axis_path_realpath(const axis_string_t *path) {
  axis_ASSERT(path, "Invalid argument.");

  if (!path || axis_string_is_empty(path)) {
    axis_LOGE("Invalid argument.");
    return NULL;
  }

  char *buf = realpath(axis_string_get_raw_str(path), NULL);
  if (!buf) {
    return NULL;
  }

  axis_string_t *ret = axis_string_create_formatted(buf);
  axis_FREE(buf);
  return ret;
}

int axis_path_is_dir(const axis_string_t *path) {
  axis_ASSERT(path, "Invalid argument.");

  if (!path || axis_string_is_empty(path)) {
    axis_LOGE("Path is empty.");
    return 0;
  }

  struct stat st;
  if (stat(axis_string_get_raw_str(path), &st) != 0) {
    axis_LOGE("Failed to stat path: %s, errno: %d", axis_string_get_raw_str(path),
             errno);
    return 0;
  }

  return S_ISDIR(st.st_mode);
}

int axis_path_mkdir(const axis_string_t *path, int recursive) {
  axis_ASSERT(path, "Invalid argument.");

  if (!path || axis_string_is_empty(path)) {
    axis_LOGE("Invalid argument.");
    return -1;
  }

  if (axis_path_is_dir(path)) {
    axis_LOGW("Path %s is existed.", axis_string_get_raw_str(path));
    return 0;
  }

  if (recursive) {
    axis_string_t *parent = axis_path_get_dirname(path);
    if (!parent) {
      axis_LOGE("Failed to get parent folder of %s",
               axis_string_get_raw_str(path));
      return -1;
    }

    // Recursively create the parent directory if it doesn't exist
    if (!axis_path_exists(axis_string_get_raw_str(parent))) {
      int ret = axis_path_mkdir(parent, 1);
      axis_string_destroy(parent);

      if (ret != 0) {
        axis_LOGE("Failed to create parent folder %s",
                 axis_string_get_raw_str(parent));
        return ret;
      }
    } else {
      axis_string_destroy(parent);
    }
  }

  int rc = mkdir(axis_string_get_raw_str(path), 0755);
  if (rc) {
    axis_LOGE("Failed to create %s: %d", axis_string_get_raw_str(path), errno);
  }

  return rc;
}

int axis_path_exists(const char *path) {
  axis_ASSERT(path, "Invalid argument.");

  if (!path || !*path) {
    return 0;
  }

  struct stat st;
  if (stat(path, &st) != 0) {
    return 0;
  }

  return 1;
}

struct axis_path_itor_t {
  struct dirent *entry;
  axis_dir_fd_t *dir;
};

struct axis_dir_fd_t {
  DIR *dir;
  axis_path_itor_t itor;
  axis_string_t *path;
};

axis_dir_fd_t *axis_path_open_dir(const char *path) {
  if (!path || !strlen(path)) {
    axis_LOGE("Invalid argument.");
    return NULL;
  }

  axis_dir_fd_t *dir = axis_MALLOC(sizeof(axis_dir_fd_t));
  axis_ASSERT(dir, "Failed to allocate memory.");
  if (!dir) {
    axis_LOGE("Failed to allocate memory for axis_dir_fd_t.");
    return NULL;
  }

  memset(dir, 0, sizeof(axis_dir_fd_t));

  dir->dir = opendir(path);
  if (!dir->dir) {
    axis_LOGE("Failed to opendir %s: %d", path, errno);
    return NULL;
  }

  dir->itor.entry = NULL;
  dir->itor.dir = dir;
  dir->path = axis_string_create_from_c_str(path, strlen(path));

  return dir;
}

int axis_path_close_dir(axis_dir_fd_t *dir) {
  axis_ASSERT(dir, "Invalid argument.");
  if (!dir) {
    axis_LOGE("Invalid argument.");
    return -1;
  }

  if (dir->dir) {
    closedir(dir->dir);
  }

  if (dir->path) {
    axis_string_destroy(dir->path);
  }

  axis_FREE(dir);
  return 0;
}

axis_path_itor_t *axis_path_get_first(axis_dir_fd_t *dir) {
  axis_ASSERT(dir, "Invalid argument.");
  if (!dir) {
    axis_LOGE("Invalid argument.");
    return NULL;
  }

  // readdir is actually thread-safe in Linux.
  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  dir->itor.entry = readdir(dir->dir);
  return &dir->itor;
}

axis_path_itor_t *axis_path_get_next(axis_path_itor_t *itor) {
  axis_ASSERT(itor, "Invalid argument.");
  if (!itor) {
    axis_LOGE("Invalid argument.");
    return NULL;
  }

  // readdir is actually thread-safe in Linux.
  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  itor->entry = readdir(itor->dir->dir);
  if (!itor->entry) {
    return NULL;
  }

  return itor;
}

axis_string_t *axis_path_itor_get_name(axis_path_itor_t *itor) {
  axis_ASSERT(itor, "Invalid argument.");
  if (!itor) {
    axis_LOGE("Invalid argument.");
    return NULL;
  }

  return axis_string_create_formatted(itor->entry->d_name);
}

axis_string_t *axis_path_itor_get_full_name(axis_path_itor_t *itor) {
  axis_ASSERT(itor, "Invalid argument.");
  if (!itor) {
    axis_LOGE("Invalid argument.");
    return NULL;
  }

  axis_string_t *short_name = axis_path_itor_get_name(itor);
  if (!short_name) {
    return NULL;
  }

  axis_string_t *full_name = axis_string_clone(itor->dir->path);
  if (!full_name) {
    axis_string_destroy(short_name);
    return NULL;
  }

  axis_string_append_formatted(full_name, "/%s",
                              axis_string_get_raw_str(short_name));
  axis_string_destroy(short_name);

  axis_path_to_system_flavor(full_name);
  return full_name;
}

int axis_path_change_cwd(axis_string_t *dirname) {
  axis_ASSERT(dirname, "Invalid argument.");

  int rc = chdir(axis_string_get_raw_str(dirname));
  if (rc) {
    axis_LOGE("Failed to chdir to %s", axis_string_get_raw_str(dirname));
    return -1;
  }
  return 0;
}

int axis_path_is_absolute(const axis_string_t *path) {
  axis_ASSERT(path, "Invalid argument.");

  if (axis_string_starts_with(path, "/")) {
    return 1;
  }
  return 0;
}

int axis_path_make_symlink(const char *target, const char *link_path) {
  axis_ASSERT(target && strlen(target) && link_path && strlen(link_path),
             "Invalid argument.");

  return symlink(target, link_path);
}

int axis_path_is_symlink(const char *path) {
  axis_ASSERT(path && strlen(path), "Invalid argument.");

  struct stat st;
  if (lstat(path, &st) != 0) {
    return 0;
  }

  return S_ISLNK(st.st_mode);
}

int axis_path_create_temp_dir(const char *base_path,
                             axis_string_t *tmp_dir_path) {
  axis_ASSERT(base_path && tmp_dir_path, "Invalid argument.");

  axis_string_init_from_c_str(tmp_dir_path, base_path, strlen(base_path));

  axis_path_join_c_str(tmp_dir_path, "tmpdir.XXXXXX");
  axis_UNUSED char *result =
      mkdtemp((char *)axis_string_get_raw_str(tmp_dir_path));
  axis_ASSERT(result, "Should not happen.");

  return 0;
}
