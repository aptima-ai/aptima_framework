//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_utils/backtrace/common.h"

#include <assert.h>
#include <inttypes.h>
#include <string.h>

#include "include_internal/axis_utils/backtrace/backtrace.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/log/log.h"
#include "axis_utils/macro/mark.h"

axis_backtrace_t *g_axis_backtrace;

#if defined(OS_WINDOWS)
// There is no 'strerror_r in Windows, use strerror_s instead. Note that the
// parameter order of strerror_s is different from strerror_r.
#define strerror_r(errno, buf, len) strerror_s(buf, len, errno)
#endif

/**
 * @brief Call strerror_r and get the full error message. Allocate memory for
 * the entire string with malloc. Return string. Caller must free string.
 * If malloc fails, return NULL.
 */
static char *axis_strerror(int errnum) {
  size_t size = 1024;
  char *buf = axis_malloc_without_backtrace(size);
  assert(buf && "Failed to allocate memory.");

  while (strerror_r(errnum, buf, size) == -1) {
    size *= 2;
    buf = axis_realloc_without_backtrace(buf, size);
    assert(buf && "Failed to allocate memory.");
  }

  return buf;
}

int axis_backtrace_default_dump_cb(axis_backtrace_t *self_, uintptr_t pc,
                                  const char *filename, int lineno,
                                  const char *function, axis_UNUSED void *data) {
  axis_backtrace_common_t *self = (axis_backtrace_common_t *)self_;
  assert(self && "Invalid argument.");

  axis_LOGE("%s:%d %s (0x%0" PRIxPTR ")", filename, lineno, function, pc);

  return 0;
}

void axis_backtrace_default_error_cb(axis_backtrace_t *self_, const char *msg,
                                    int errnum, axis_UNUSED void *data) {
  axis_backtrace_common_t *self = (axis_backtrace_common_t *)self_;
  assert(self && "Invalid argument.");

  axis_LOGE("%s", msg);

  if (errnum > 0) {
    char *buf = axis_strerror(errnum);
    axis_LOGE(": %s", buf);

    axis_free_without_backtrace(buf);
  }
}

void axis_backtrace_common_init(axis_backtrace_common_t *self,
                               axis_backtrace_dump_file_line_func_t dump_cb,
                               axis_backtrace_error_func_t error_cb) {
  assert(self && "Invalid argument.");

  self->dump_cb = dump_cb;
  self->error_cb = error_cb;
}

void axis_backtrace_common_deinit(axis_backtrace_t *self) {
  axis_backtrace_common_t *common_self = (axis_backtrace_common_t *)self;
  assert(common_self && "Invalid argument.");
}

void axis_backtrace_create_global(void) {
  g_axis_backtrace = axis_backtrace_create();
}

void axis_backtrace_destroy_global(void) {
  axis_backtrace_destroy(g_axis_backtrace);
}

void axis_backtrace_dump_global(size_t skip) {
  const char *enable_backtrace_dump = getenv("axis_ENABLE_BACKTRACE_DUMP");
  if (enable_backtrace_dump && !strcmp(enable_backtrace_dump, "true")) {
    axis_backtrace_dump(g_axis_backtrace, skip);
  } else {
    axis_LOGI("Backtrace dump is disabled by axis_ENABLE_BACKTRACE_DUMP.");
  }
}
