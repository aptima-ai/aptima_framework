//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
// This file is modified from
// https://github.com/ianlancetaylor/libbacktrace [BSD license]
//
#include "include_internal/axis_utils/backtrace/platform/posix/linux/debugfile.h"

#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "include_internal/axis_utils/backtrace/platform/posix/linux/crc32.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/file.h"
#include "axis_utils/macro/check.h"

#define SYSTEM_BUILD_ID_DIR "/usr/lib/debug/.build-id/"

/**
 * @brief Open a separate debug info file, using the build ID to find it.
 *
 * @return an open file descriptor, or -1.
 *
 * @note The GDB manual says that the only place gdb looks for a debug file when
 * the build ID is known is in /usr/lib/debug/.build-id.
 */
int elf_open_debug_file_by_build_id(axis_backtrace_t *self,
                                    const char *build_id_data,
                                    size_t build_id_size) {
  axis_ASSERT(self, "Invalid argument.");

  const char *const prefix = SYSTEM_BUILD_ID_DIR;
  const size_t prefix_len = strlen(prefix);
  const char *const suffix = ".debug";
  const size_t suffix_len = strlen(suffix);
  bool does_not_exist = false;

  size_t len = prefix_len + build_id_size * 2 + suffix_len + 2;

  char *build_id_filename = axis_malloc_without_backtrace(len);
  axis_ASSERT(build_id_filename, "Failed to allocate memory.");
  if (build_id_filename == NULL) {
    return -1;
  }

  char *t = build_id_filename;
  memcpy(t, prefix, prefix_len);

  t += prefix_len;
  for (size_t i = 0; i < build_id_size; i++) {
    unsigned char b = (unsigned char)build_id_data[i];
    unsigned char nib = (b & 0xf0) >> 4;

    *t++ = (char)(nib < 10 ? '0' + nib : 'a' + nib - 10);
    nib = b & 0x0f;
    *t++ = (char)(nib < 10 ? '0' + nib : 'a' + nib - 10);

    if (i == 0) {
      *t++ = '/';
    }
  }

  memcpy(t, suffix, suffix_len);
  t[suffix_len] = '\0';

  int ret = axis_file_open(build_id_filename, &does_not_exist);

  axis_free_without_backtrace(build_id_filename);

  // gdb checks that the debuginfo file has the same build ID note. That seems
  // kind of pointless to me--why would it have the right name but not the right
  // build ID?--so skipping the check.

  return ret;
}

/// Return whether @a filename is a symlink.
static int elf_is_symlink(const char *filename) {
  struct stat st;

  if (lstat(filename, &st) < 0) {
    return 0;
  }

  return S_ISLNK(st.st_mode);
}

/**
 * @brief Return the results of reading the symlink @a filename in a buffer.
 * Return the length of the buffer in @a *len.
 */
static char *elf_readlink(axis_backtrace_t *self, const char *filename,
                          size_t *plen) {
  axis_ASSERT(self, "Invalid argument.");

  size_t len = 128;

  while (1) {
    char *buf = axis_malloc_without_backtrace(len);
    axis_ASSERT(buf, "Failed to allocate memory.");
    if (buf == NULL) {
      return NULL;
    }

    ssize_t rl = readlink(filename, buf, len);
    if (rl < 0) {
      axis_free_without_backtrace(buf);
      return NULL;
    }

    if ((size_t)rl < len - 1) {
      buf[rl] = '\0';
      *plen = len;
      return buf;
    }

    axis_free_without_backtrace(buf);

    // Enlarge the buffer, and read again.
    len *= 2;
  }
}

/**
 * @brief Try to open a file whose name is @a prefix (length @a prefix_len)
 * concatenated with @a prefix2 (length @a prefix2_len) concatenated with
 * @a debug_link_name.
 *
 * @return an open file descriptor, or -1.
 */
static int elf_try_debug_file(axis_backtrace_t *self, const char *prefix,
                              size_t prefix_len, const char *prefix2,
                              size_t prefix2_len, const char *debug_link_name) {
  axis_ASSERT(self, "Invalid argument.");

  bool does_not_exist = false;

  size_t debug_link_len = strlen(debug_link_name);
  size_t try_len = prefix_len + prefix2_len + debug_link_len + 1;
  char *try = axis_malloc_without_backtrace(try_len);
  axis_ASSERT(try, "Failed to allocate memory.");
  if (try == NULL) {
    return -1;
  }

  memcpy(try, prefix, prefix_len);
  memcpy(try + prefix_len, prefix2, prefix2_len);
  memcpy(try + prefix_len + prefix2_len, debug_link_name, debug_link_len);
  try[prefix_len + prefix2_len + debug_link_len] = '\0';

  int ret = axis_file_open(try, &does_not_exist);

  axis_free_without_backtrace(try);

  return ret;
}

/**
 * @brief Find a separate debug info file, using the debug_link section data to
 * find it.
 *
 * @return an open file descriptor, or -1.
 */
static int elf_find_debug_file_by_debug_link(axis_backtrace_t *self,
                                             const char *filename,
                                             const char *debug_link_name) {
  axis_ASSERT(self, "Invalid argument.");

  const char *slash = NULL;

  /// Resolve symlinks in @a filename. Since @a filename is fairly likely to be
  /// /proc/self/exe, symlinks are common. We don't try to resolve the whole
  /// path name, just the base name.

  int ret = -1;
  char *alc = NULL;
  size_t alc_len = 0;
  while (elf_is_symlink(filename)) {
    size_t new_len = 0;

    char *new_buf = elf_readlink(self, filename, &new_len);
    if (new_buf == NULL) {
      break;
    }

    if (new_buf[0] == '/') {
      // Absolute path. Use it directly, and no more filename handling is needed
      // for this round.

      filename = new_buf;
    } else {
      slash = strrchr(filename, '/');
      if (slash == NULL) {
        // Basename only. Use it directly, and no more filename handling is
        // needed for this round.

        filename = new_buf;
      } else {
        slash++;

        // Ex:
        //
        // filename: a/b/c/d
        // symlink content: x/y/z
        //
        // => a/b/c/x/y/z

        size_t clen = slash - filename + strlen(new_buf) + 1;
        char *c = axis_malloc_without_backtrace(clen);
        axis_ASSERT(c, "Failed to allocate memory.");
        if (c == NULL) {
          goto done;
        }

        memcpy(c, filename, slash - filename);
        memcpy(c + (slash - filename), new_buf, strlen(new_buf));
        c[slash - filename + strlen(new_buf)] = '\0';

        axis_free_without_backtrace(new_buf);

        filename = c;
        new_buf = c;
        new_len = clen;
      }
    }

    if (alc != NULL) {
      axis_free_without_backtrace(alc);
    }

    alc = new_buf;
    alc_len = new_len;
  }

  const char *prefix = NULL;
  size_t prefix_len = 0;

  /// Alternative 1: Look for @a debug_link_name in the same directory as @a
  /// filename

  slash = strrchr(filename, '/');
  if (slash == NULL) {
    prefix = "";
    prefix_len = 0;
  } else {
    slash++;
    prefix = filename;
    prefix_len = slash - filename;
  }

  int debug_file_fd =
      elf_try_debug_file(self, prefix, prefix_len, "", 0, debug_link_name);
  if (debug_file_fd >= 0) {
    ret = debug_file_fd;
    goto done;
  }

  /// Alternative 2: Look for @a debug_link_name in a .debug subdirectory of @a
  /// filename

  debug_file_fd = elf_try_debug_file(self, prefix, prefix_len, ".debug/",
                                     strlen(".debug/"), debug_link_name);
  if (debug_file_fd >= 0) {
    ret = debug_file_fd;
    goto done;
  }

  /// Alternative 3: Look for @a debug_link_name in /usr/lib/debug

  debug_file_fd =
      elf_try_debug_file(self, "/usr/lib/debug/", strlen("/usr/lib/debug/"),
                         prefix, prefix_len, debug_link_name);
  if (debug_file_fd >= 0) {
    ret = debug_file_fd;
  }

done:
  if (alc != NULL && alc_len > 0) {
    axis_free_without_backtrace(alc);
  }

  return ret;
}

/**
 * @brief Open a separate debug info file, using the debug_link section data to
 * find it.
 *
 * @return an open file descriptor, or -1.
 */
int elf_open_debug_file_by_debug_link(
    axis_backtrace_t *self, const char *filename, const char *debug_link_name,
    uint32_t debug_link_crc, axis_backtrace_error_func_t error_cb, void *data) {
  axis_ASSERT(self, "Invalid argument.");

  int debug_file_fd =
      elf_find_debug_file_by_debug_link(self, filename, debug_link_name);
  if (debug_file_fd < 0) {
    return -1;
  }

  if (debug_link_crc != 0) {
    uint32_t got_crc = elf_crc32_file(self, debug_file_fd, error_cb, data);
    if (got_crc != debug_link_crc) {
      /// CRC checksum error, the found debug file is not the correct one for @a
      /// filename.
      axis_file_close(debug_file_fd);
      return -1;
    }
  }

  return debug_file_fd;
}
