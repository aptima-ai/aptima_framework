//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_utils/backtrace/backtrace.h"

#include <assert.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include_internal/axis_utils/backtrace/common.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/log/log.h"

/**
 * @note On Mac, we are currently using a simple method instead of a complicated
 * posix method to dump backtrace. So we only need a field of
 * 'axis_backtrace_common_t'.
 */
typedef struct axis_backtrace_mac_t {
  axis_backtrace_common_t common;
} axis_backtrace_mac_t;

axis_backtrace_t *axis_backtrace_create(void) {
  axis_backtrace_mac_t *self =
      axis_malloc_without_backtrace(sizeof(axis_backtrace_mac_t));
  assert(self && "Failed to allocate memory.");

  axis_backtrace_common_init(&self->common, axis_backtrace_default_dump_cb,
                            axis_backtrace_default_error_cb);

  return (axis_backtrace_t *)self;
}

void axis_backtrace_destroy(axis_backtrace_t *self) {
  assert(self && "Invalid argument.");

  axis_backtrace_common_deinit(self);

  axis_free_without_backtrace(self);
}

void axis_backtrace_dump(axis_backtrace_t *self, size_t skip) {
  assert(self && "Invalid argument.");

  // NOTE: Currently, the only way to get backtrace via
  // 'axis_backtrace_dump_posix' is to create .dsym for each executable and
  // libraries. Otherwise, it will show
  //
  // "no debug info in Mach-O executable".
  //
  // Therefore, we use the glibc builtin method (backtrace_symbols) for now.

  void *call_stack[128];
  int frames = backtrace(call_stack, 128);
  char **strs = backtrace_symbols(call_stack, frames);
  for (size_t i = skip; i < frames; ++i) {
    axis_LOGE("%s", strs[i]);
  }
  free(strs);
}
