//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
// This file is modified from
// https://github.com/ianlancetaylor/libbacktrace [BSD license]
//
#include "axis_utils/axis_config.h"

#include "include_internal/axis_utils/backtrace/backtrace.h"

#include <assert.h>
#include <sys/types.h>

#include "include_internal/axis_utils/backtrace/platform/posix/internal.h"
#include "unwind.h"

/**
 * @brief Data passed through _Unwind_Backtrace.
 */
typedef struct backtrace_data {
  size_t skip;  // Number of frames to skip.
  axis_backtrace_t *axis_backtrace;
  int ret;  // Value to return from axis_backtrace_dump_posix.
} backtrace_data;

/**
 * @brief Unwind library callback routine. This is passed to _Unwind_Backtrace.
 */
static _Unwind_Reason_Code unwind(struct _Unwind_Context *context, void *data) {
  backtrace_data *bt_data = (backtrace_data *)data;
  int ip_before_insn = 0;

  uintptr_t pc = _Unwind_GetIPInfo(context, &ip_before_insn);

  if (bt_data->skip > 0) {
    --bt_data->skip;
    return _URC_NO_REASON;
  }

  if (!ip_before_insn) {
    --pc;
  }

  bt_data->ret = axis_backtrace_get_file_line_info(
      bt_data->axis_backtrace, pc,
      ((axis_backtrace_common_t *)bt_data->axis_backtrace)->dump_cb,
      ((axis_backtrace_common_t *)bt_data->axis_backtrace)->error_cb,
      ((axis_backtrace_common_t *)bt_data->axis_backtrace)->cb_data);
  if (bt_data->ret != 0) {
    return _URC_END_OF_STACK;
  }

  return _URC_NO_REASON;
}

/**
 * @brief Get a stack backtrace.
 */
int axis_backtrace_dump_posix(axis_backtrace_t *self, size_t skip) {
  assert(self && "Invalid argument.");

  backtrace_data bt_data;
  bt_data.skip = skip + 1;
  bt_data.axis_backtrace = self;
  bt_data.ret = 0;

  // _Unwind_Backtrace() performs a stack backtrace using unwind data.
  _Unwind_Backtrace(unwind, &bt_data);

  return bt_data.ret;
}
