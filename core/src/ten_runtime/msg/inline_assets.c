//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "include_internal/ten_runtime/msg/msg.h"

extern inline bool ten_raw_msg_is_cmd_base(ten_msg_t *self);  // NOLINT

extern inline bool ten_raw_msg_is_cmd(ten_msg_t *self);  // NOLINT

extern inline bool ten_raw_msg_is_cmd_result(ten_msg_t *self);  // NOLINT

extern inline ten_msg_t *ten_msg_get_raw_msg(ten_shared_ptr_t *self);  // NOLINT

extern inline bool ten_msg_is_cmd_base(ten_shared_ptr_t *self);  // NOLINT

extern inline bool ten_msg_is_cmd(ten_shared_ptr_t *self);  // NOLINT

extern inline bool ten_msg_is_cmd_result(ten_shared_ptr_t *self);  // NOLINT

extern inline TEN_MSG_TYPE ten_raw_msg_get_type(ten_msg_t *self);  // NOLINT
