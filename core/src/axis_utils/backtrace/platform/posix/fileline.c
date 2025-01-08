//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
// This file is modified from
// https://github.com/ianlancetaylor/libbacktrace [BSD license]
//
#include "axis_utils/axis_config.h"

#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "include_internal/axis_utils/backtrace/platform/posix/config.h"  // IWYU pragma: keep
#include "axis_utils/log/log.h"

#ifdef HAVE_MACH_O_DYLD_H
#include <mach-o/dyld.h>
#endif

#include "include_internal/axis_utils/backtrace/backtrace.h"
#include "include_internal/axis_utils/backtrace/platform/posix/internal.h"
#include "axis_utils/lib/atomic_ptr.h"
#include "axis_utils/lib/file.h"
#include "axis_utils/macro/mark.h"

#ifdef HAVE_MACH_O_DYLD_H

#include "axis_utils/lib/alloc.h"

static char *macho_get_executable_path(axis_backtrace_t *self,
                                       axis_backtrace_error_func_t error_cb,
                                       void *data) {
  uint32_t len = 0;
  if (_NSGetExecutablePath(NULL, &len) == 0) {
    return NULL;
  }

  char *name = (char *)axis_malloc_without_backtrace(len);
  assert(name && "Failed to allocate memory.");

  if (_NSGetExecutablePath(name, &len) != 0) {
    axis_free_without_backtrace(name);
    return NULL;
  }

  return name;
}

#else /* !defined (HAVE_MACH_O_DYLD_H) */

#define macho_get_executable_path(self, error_cb, data) NULL

#endif /* !defined (HAVE_MACH_O_DYLD_H) */

/**
 * @brief Initialize the file_line information from the executable.
 *
 * @return 1 on success, 0 on failure.
 */
static int initialize_file_line_mechanism(axis_backtrace_t *self,
                                          axis_backtrace_error_func_t error_cb,
                                          void *data) {
  axis_backtrace_posix_t *posix_self = (axis_backtrace_posix_t *)self;
  assert(posix_self && "Invalid argument.");

  axis_atomic_t failed = axis_atomic_load(&posix_self->file_line_init_failed);
  if (failed) {
    axis_LOGE("Failed to read executable information.");
    return 0;
  }

  axis_backtrace_get_file_line_func_t get_file_line =
      (axis_backtrace_get_file_line_func_t)axis_atomic_ptr_load(
          (void *)&posix_self->get_file_line);
  if (get_file_line != NULL) {
    return 1;
  }

  // We have not initialized the information. Do it now.

  int descriptor = -1;
  const char *filename = NULL;
  bool called_error_callback = false;
  for (size_t pass = 0; pass < 4; ++pass) {
    switch (pass) {
      case 0:
        filename = "/proc/self/exe";
        break;
      case 1:
        filename = "/proc/curproc/file";
        break;
      case 2: {
        char buf[64] = {0};
        int written = snprintf(buf, sizeof(buf), "/proc/%ld/object/a.out",
                               (long)getpid());
        assert(written > 0);
        filename = buf;
        break;
      }
      case 3:
        filename = macho_get_executable_path(self, error_cb, data);
        break;
      default:
        abort();
        break;
    }

    if (filename == NULL) {
      // Failed to get the executable filename, try next way.
      continue;
    }

    bool does_not_exist = false;
    descriptor = axis_file_open(filename, &does_not_exist);
    if (descriptor < 0 && !does_not_exist) {
      called_error_callback = true;
      break;
    }

    if (descriptor >= 0) {
      // Successfully open the executable file.
      break;
    }

    // Failed to open the executable filename, try next way.
  }

  if (descriptor < 0) {
    if (!called_error_callback) {
      error_cb(data, "Failed to find executable to open.", 0, NULL);
    }
    failed = 1;
  }

  if (!failed) {
    if (!axis_backtrace_init_posix(self, filename, descriptor, error_cb, data,
                                  &get_file_line)) {
      failed = 1;
    }
  }

  if (failed) {
    axis_atomic_store(&posix_self->file_line_init_failed, 1);
    return 0;
  }

  // TODO(Wei): Note that if two threads initialize at once, one of the data
  // sets may be leaked. Might need to consider a new way to avoid this.
  axis_atomic_ptr_store((void *)&posix_self->get_file_line, get_file_line);

  return 1;
}

/**
 * @brief Given a PC, find the file name, line number, and function name.
 */
int axis_backtrace_get_file_line_info(axis_backtrace_t *self, uintptr_t pc,
                                     axis_backtrace_dump_file_line_func_t cb,
                                     axis_backtrace_error_func_t error_cb,
                                     void *data) {
  axis_backtrace_posix_t *posix_self = (axis_backtrace_posix_t *)self;
  assert(posix_self && "Invalid argument.");

  if (!initialize_file_line_mechanism(self, error_cb, data)) {
    return 0;
  }

  if (axis_atomic_load(&posix_self->file_line_init_failed)) {
    return 0;
  }

  return ((axis_backtrace_get_file_line_func_t)(posix_self->get_file_line))(
      self, pc, cb, error_cb, data);
}

/**
 * @brief Given a PC, find the symbol for it, and its value.
 */
int axis_backtrace_get_syminfo(axis_backtrace_t *self, uintptr_t pc,
                              axis_backtrace_dump_syminfo_func_t cb,
                              axis_backtrace_error_func_t error_cb, void *data) {
  axis_backtrace_posix_t *posix_self = (axis_backtrace_posix_t *)self;
  assert(posix_self && "Invalid argument.");

  if (!initialize_file_line_mechanism(self, error_cb, data)) {
    return 0;
  }

  if (axis_atomic_load(&posix_self->file_line_init_failed)) {
    return 0;
  }

  posix_self->get_syminfo(self, pc, cb, error_cb, data);

  return 1;
}

/**
 * @brief A axis_backtrace_dump_syminfo_func_t that can call into a
 * axis_backtrace_dump_file_line_func_t, used when we have a symbol table but no
 * debug info.
 */
void backtrace_dump_syminfo_to_dump_file_line_cb(
    axis_backtrace_t *self, uintptr_t pc, const char *symname,
    axis_UNUSED uintptr_t sym_val, axis_UNUSED uintptr_t sym_size, void *data) {
  struct backtrace_call_full *bt_data = (struct backtrace_call_full *)data;
  assert(bt_data && "Invalid argument.");

  bt_data->ret =
      bt_data->dump_file_line_cb(self, pc, NULL, 0, symname, bt_data->data);
}

/**
 * @brief An error callback that corresponds to
 * backtrace_syminfo_to_full_callback.
 */
void backtrace_dump_syminfo_to_dump_file_line_error_cb(axis_backtrace_t *self,
                                                       const char *msg,
                                                       int errnum, void *data) {
  struct backtrace_call_full *bt_data = (struct backtrace_call_full *)data;
  assert(bt_data && "Invalid argument.");

  bt_data->error_cb(self, msg, errnum, bt_data->data);
}
