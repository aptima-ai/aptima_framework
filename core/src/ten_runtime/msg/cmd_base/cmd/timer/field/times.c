//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <stdint.h>

#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd/timer/cmd.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "include_internal/ten_utils/value/value_set.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"

bool ten_cmd_timer_put_times_to_json(ten_msg_t *self, ten_json_t *json,
                                     ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self) && json &&
                 ten_raw_msg_get_type(self) == TEN_MSG_TYPE_CMD_TIMER,
             "Should not happen.");

  ten_json_t *ten_json =
      ten_json_object_peek_object_forcibly(json, TEN_STR_UNDERLINE_TEN);
  TEN_ASSERT(ten_json, "Should not happen.");

  ten_json_object_set_new(ten_json, TEN_STR_TIMES,
                          ten_json_create_integer(ten_raw_cmd_timer_get_times(
                              (ten_cmd_timer_t *)self)));

  return true;
}

bool ten_cmd_timer_get_times_from_json(ten_msg_t *self, ten_json_t *json,
                                       TEN_UNUSED ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self) && json &&
                 ten_raw_msg_get_type(self) == TEN_MSG_TYPE_CMD_TIMER,
             "Should not happen.");

  ten_json_t *ten_json =
      ten_json_object_peek_object(json, TEN_STR_UNDERLINE_TEN);
  if (!ten_json) {
    return true;
  }

  ten_json_t *times_json = ten_json_object_peek(ten_json, TEN_STR_TIMES);
  if (!times_json) {
    return true;
  }

  if (ten_json_is_integer(times_json)) {
    int64_t times = ten_json_get_integer_value(times_json);
    if (times > INT32_MAX || times < INT32_MIN) {
      TEN_LOGW("The value of times is too large.");
      return false;
    }

    ten_cmd_timer_t *cmd = (ten_cmd_timer_t *)self;
    ten_value_set_int32(&cmd->times, (int32_t)times);
  } else {
    TEN_LOGW("times should be an integer.");
  }

  return true;
}

bool ten_cmd_timer_process_times(ten_msg_t *self,
                                 ten_raw_msg_process_one_field_func_t cb,
                                 void *user_data, ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self), "Should not happen.");

  ten_msg_field_process_data_t times_field;
  ten_msg_field_process_data_init(&times_field, TEN_STR_TIMES,
                                  &((ten_cmd_timer_t *)self)->times, false);

  return cb(self, &times_field, user_data, err);
}
