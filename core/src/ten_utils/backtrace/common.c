//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_utils/backtrace/common.h"

#include <assert.h>
#include <inttypes.h>
#include <string.h>

#include "include_internal/ten_utils/backtrace/backtrace.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/log/log.h"
#include "ten_utils/macro/mark.h"

ten_backtrace_t *g_ten_backtrace;

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
static char *ten_strerror(int errnum) {
  size_t size = 1024;
  char *buf = ten_malloc_without_backtrace(size);
  assert(buf && "Failed to allocate memory.");

  while (strerror_r(errnum, buf, size) == -1) {
    size *= 2;
    buf = ten_realloc_without_backtrace(buf, size);
    assert(buf && "Failed to allocate memory.");
  }

  return buf;
}

int ten_backtrace_default_dump_cb(ten_backtrace_t *self_, uintptr_t pc,
                                  const char *filename, int lineno,
                                  const char *function, TEN_UNUSED void *data) {
  ten_backtrace_common_t *self = (ten_backtrace_common_t *)self_;
  assert(self && "Invalid argument.");

  TEN_LOGE("%s:%d %s (0x%0" PRIxPTR ")", filename, lineno, function, pc);

  return 0;
}

void ten_backtrace_default_error_cb(ten_backtrace_t *self_, const char *msg,
                                    int errnum, TEN_UNUSED void *data) {
  ten_backtrace_common_t *self = (ten_backtrace_common_t *)self_;
  assert(self && "Invalid argument.");

  TEN_LOGE("%s", msg);

  if (errnum > 0) {
    char *buf = ten_strerror(errnum);
    TEN_LOGE(": %s", buf);

    ten_free_without_backtrace(buf);
  }
}

void ten_backtrace_common_init(ten_backtrace_common_t *self,
                               ten_backtrace_dump_file_line_func_t dump_cb,
                               ten_backtrace_error_func_t error_cb) {
  assert(self && "Invalid argument.");

  self->dump_cb = dump_cb;
  self->error_cb = error_cb;
}

void ten_backtrace_common_deinit(ten_backtrace_t *self) {
  ten_backtrace_common_t *common_self = (ten_backtrace_common_t *)self;
  assert(common_self && "Invalid argument.");
}

void ten_backtrace_create_global(void) {
  g_ten_backtrace = ten_backtrace_create();
}

void ten_backtrace_destroy_global(void) {
  ten_backtrace_destroy(g_ten_backtrace);
}

void ten_backtrace_dump_global(size_t skip) {
  const char *enable_backtrace_dump = getenv("TEN_ENABLE_BACKTRACE_DUMP");
  if (enable_backtrace_dump && !strcmp(enable_backtrace_dump, "true")) {
    ten_backtrace_dump(g_ten_backtrace, skip);
  } else {
    TEN_LOGI("Backtrace dump is disabled by TEN_ENABLE_BACKTRACE_DUMP.");
  }
}
