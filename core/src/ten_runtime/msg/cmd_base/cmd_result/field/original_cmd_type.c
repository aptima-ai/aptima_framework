//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/msg/cmd_base/cmd_result/field/original_cmd_type.h"

#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd_result/cmd.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "include_internal/ten_utils/value/value_set.h"
#include "ten_runtime/msg/cmd_result/cmd_result.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"

bool ten_cmd_result_put_original_cmd_type_to_json(ten_msg_t *self,
                                                  ten_json_t *json,
                                                  ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self) &&
                 ten_raw_msg_get_type(self) == TEN_MSG_TYPE_CMD_RESULT,
             "Should not happen.");

  const char *origin_cmd_type_str = ten_msg_type_to_string(
      ten_raw_cmd_result_get_original_cmd_type((ten_cmd_result_t *)self));
  if (origin_cmd_type_str != NULL) {
    ten_json_t *ten_json =
        ten_json_object_peek_object_forcibly(json, TEN_STR_UNDERLINE_TEN);
    TEN_ASSERT(ten_json, "Should not happen.");

    ten_json_object_set_new(ten_json, TEN_STR_ORIGINAL_CMD_TYPE,
                            ten_json_create_string(origin_cmd_type_str));
  }

  return true;
}

bool ten_cmd_result_get_original_cmd_type_from_json(
    ten_msg_t *self, ten_json_t *json, TEN_UNUSED ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self) &&
                 ten_raw_msg_get_type(self) == TEN_MSG_TYPE_CMD_RESULT,
             "Should not happen.");
  TEN_ASSERT(json && ten_json_check_integrity(json), "Should not happen.");

  ten_json_t *ten_json =
      ten_json_object_peek_object(json, TEN_STR_UNDERLINE_TEN);
  if (!ten_json) {
    return true;
  }

  ten_json_t *original_cmd_type_json =
      ten_json_object_peek(ten_json, TEN_STR_ORIGINAL_CMD_TYPE);
  if (!original_cmd_type_json) {
    return true;
  }

  if (ten_json_is_string(original_cmd_type_json)) {
    const char *original_cmd_type_str =
        ten_json_object_peek_string(ten_json, TEN_STR_ORIGINAL_CMD_TYPE);
    const char *original_cmd_name_str =
        ten_json_object_peek_string(ten_json, TEN_STR_ORIGINAL_CMD_NAME);

    TEN_MSG_TYPE msg_type = ten_msg_type_from_type_and_name_string(
        original_cmd_type_str, original_cmd_name_str);

    ten_value_set_int32(&((ten_cmd_result_t *)self)->original_cmd_type,
                        msg_type);
  } else {
    TEN_LOGW("original_cmd_type should be a string.");
  }

  return true;
}

void ten_cmd_result_copy_original_cmd_type(
    ten_msg_t *self, ten_msg_t *src,
    TEN_UNUSED ten_list_t *excluded_field_ids) {
  TEN_ASSERT(self && src &&
                 ten_raw_cmd_base_check_integrity((ten_cmd_base_t *)src) &&
                 ten_raw_msg_get_type(src) == TEN_MSG_TYPE_CMD_RESULT,
             "Should not happen.");

  ten_value_set_int32(
      &((ten_cmd_result_t *)self)->original_cmd_type,
      ten_raw_cmd_result_get_original_cmd_type((ten_cmd_result_t *)src));
}

bool ten_cmd_result_process_original_cmd_type(
    ten_msg_t *self, ten_raw_msg_process_one_field_func_t cb, void *user_data,
    ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self), "Should not happen.");

  ten_msg_field_process_data_t original_cmd_type_field;
  ten_msg_field_process_data_init(
      &original_cmd_type_field, TEN_STR_ORIGINAL_CMD_TYPE,
      &((ten_cmd_result_t *)self)->original_cmd_type, false);

  return cb(self, &original_cmd_type_field, user_data, err);
}
