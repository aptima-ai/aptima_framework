//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd/timer/cmd.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/log/log.h"
#include "ten_utils/macro/check.h"

bool ten_cmd_timer_put_timeout_in_us_to_json(ten_msg_t *self, ten_json_t *json,
                                             ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self) && json &&
                 ten_raw_msg_get_type(self) == TEN_MSG_TYPE_CMD_TIMER,
             "Should not happen.");

  ten_json_t *ten_json =
      ten_json_object_peek_object_forcibly(json, TEN_STR_UNDERLINE_TEN);
  TEN_ASSERT(ten_json, "Should not happen.");

  uint64_t timeout_in_us =
      ten_raw_cmd_timer_get_timeout_in_us((ten_cmd_timer_t *)self);

  if (timeout_in_us > INT64_MAX) {
    TEN_LOGE("Timeout duration (%zu) is overloaded", timeout_in_us);
    return false;
  }

  ten_json_object_set_new(ten_json, TEN_STR_TIMEOUT_IN_US,
                          ten_json_create_integer((int64_t)timeout_in_us));

  return true;
}

bool ten_cmd_timer_get_timeout_in_us_from_json(ten_msg_t *self,
                                               ten_json_t *json,
                                               bool remove_from_json,
                                               TEN_UNUSED ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self) &&
                 ten_raw_msg_get_type(self) == TEN_MSG_TYPE_CMD_TIMER,
             "Should not happen.");
  TEN_ASSERT(json && ten_json_check_integrity(json), "Should not happen.");

  ten_json_t *ten_json =
      ten_json_object_peek_object(json, TEN_STR_UNDERLINE_TEN);
  if (!ten_json) {
    return true;
  }

  ten_json_t *timeout_in_us_json =
      ten_json_object_peek(ten_json, TEN_STR_TIMEOUT_IN_US);
  if (!timeout_in_us_json) {
    return true;
  }

  if (ten_json_is_integer(timeout_in_us_json)) {
    ten_cmd_timer_t *cmd = (ten_cmd_timer_t *)self;

    int64_t timeout_in_us = ten_json_get_integer_value(timeout_in_us_json);
    if (timeout_in_us < 0) {
      TEN_LOGE("Invalid negative timeout value %ld", timeout_in_us);
      return false;
    }
    cmd->timeout_in_us = (uint64_t)timeout_in_us;
  } else {
    TEN_LOGW("timeout_in_us should be an integer.");
  }

  if (remove_from_json) {
    ten_json_object_del(ten_json, TEN_STR_TIMEOUT_IN_US);
  }

  return true;
}
