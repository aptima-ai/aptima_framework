//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_utils/backtrace/backtrace.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include_internal/axis_utils/backtrace/common.h"
#include "include_internal/axis_utils/backtrace/platform/posix/internal.h"
#include "axis_utils/lib/alloc.h"

axis_backtrace_t *axis_backtrace_create(void) {
  axis_backtrace_posix_t *self =
      axis_malloc_without_backtrace(sizeof(axis_backtrace_posix_t));
  assert(self && "Failed to allocate memory.");

  axis_backtrace_common_init(&self->common, axis_backtrace_default_dump_cb,
                            axis_backtrace_default_error_cb);

  self->get_file_line = NULL;
  self->get_file_line_data = NULL;
  self->get_syminfo = NULL;
  self->get_syminfo_data = NULL;
  self->file_line_init_failed = 0;

  return (axis_backtrace_t *)self;
}

void axis_backtrace_destroy(axis_backtrace_t *self) {
  assert(self && "Invalid argument.");

  axis_backtrace_common_deinit(self);

  axis_free_without_backtrace(self);
}

void axis_backtrace_dump(axis_backtrace_t *self, size_t skip) {
  assert(self && "Invalid argument.");
  axis_backtrace_dump_posix(self, skip);
}
