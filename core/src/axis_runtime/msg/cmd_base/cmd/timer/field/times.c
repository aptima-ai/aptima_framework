//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <stdint.h>

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/timer/cmd.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "axis_utils/macro/check.h"

bool axis_cmd_timer_process_times(axis_msg_t *self,
                                 axis_raw_msg_process_one_field_func_t cb,
                                 void *user_data, axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");

  axis_msg_field_process_data_t times_field;
  axis_msg_field_process_data_init(&times_field, axis_STR_TIMES,
                                  &((axis_cmd_timer_t *)self)->times, false);

  return cb(self, &times_field, user_data, err);
}
