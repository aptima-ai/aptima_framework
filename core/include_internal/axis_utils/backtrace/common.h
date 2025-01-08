//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include "include_internal/axis_utils/backtrace/backtrace.h"

typedef struct axis_backtrace_common_t {
  axis_backtrace_dump_file_line_func_t dump_cb;
  axis_backtrace_error_func_t error_cb;
  void *cb_data;  // The user-defined argument to the above callback functions.
} axis_backtrace_common_t;

axis_UTILS_PRIVATE_API axis_backtrace_t *g_axis_backtrace;

axis_UTILS_PRIVATE_API void axis_backtrace_common_init(
    axis_backtrace_common_t *self, axis_backtrace_dump_file_line_func_t dump_cb,
    axis_backtrace_error_func_t error_cb);

axis_UTILS_PRIVATE_API void axis_backtrace_common_deinit(axis_backtrace_t *self);

axis_UTILS_PRIVATE_API int axis_backtrace_default_dump_cb(
    axis_backtrace_t *self, uintptr_t pc, const char *filename, int lineno,
    const char *function, void *data);

axis_UTILS_PRIVATE_API void axis_backtrace_default_error_cb(axis_backtrace_t *self,
                                                          const char *msg,
                                                          int errnum,
                                                          void *data);
