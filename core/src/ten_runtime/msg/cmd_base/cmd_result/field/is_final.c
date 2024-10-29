//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/msg/cmd_base/cmd_result/field/is_final.h"

#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd_result/cmd.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "ten_runtime/msg/cmd_result/cmd_result.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"

bool ten_cmd_result_put_is_final_to_json(ten_msg_t *self, ten_json_t *json,
                                         ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self) &&
                 ten_raw_msg_get_type(self) == TEN_MSG_TYPE_CMD_RESULT,
             "Should not happen.");

  ten_json_t *ten_json =
      ten_json_object_peek_object_forcibly(json, TEN_STR_UNDERLINE_TEN);
  TEN_ASSERT(ten_json, "Should not happen.");

  ten_json_object_set_new(ten_json, TEN_STR_IS_FINAL,
                          ten_json_create_boolean(ten_raw_cmd_result_is_final(
                              (ten_cmd_result_t *)self, NULL)));

  return true;
}

bool ten_cmd_result_get_is_final_from_json(ten_msg_t *self, ten_json_t *json,
                                           TEN_UNUSED ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self) &&
                 ten_raw_msg_get_type(self) == TEN_MSG_TYPE_CMD_RESULT,
             "Should not happen.");
  TEN_ASSERT(json && ten_json_check_integrity(json), "Should not happen.");

  ten_json_t *ten_json =
      ten_json_object_peek_object(json, TEN_STR_UNDERLINE_TEN);
  if (!ten_json) {
    return true;
  }

  ten_json_t *is_final_json = ten_json_object_peek(ten_json, TEN_STR_IS_FINAL);
  if (!is_final_json) {
    return true;
  }

  if (ten_json_is_boolean(is_final_json)) {
    bool is_final = ten_json_object_get_boolean(ten_json, TEN_STR_IS_FINAL);

    ten_raw_cmd_result_set_final((ten_cmd_result_t *)self, is_final, NULL);
  } else {
    TEN_LOGW("is_final should be an boolean.");
  }

  return true;
}

void ten_cmd_result_copy_is_final(ten_msg_t *self, ten_msg_t *src,
                                  TEN_UNUSED ten_list_t *excluded_field_ids) {
  TEN_ASSERT(self && src &&
                 ten_raw_cmd_base_check_integrity((ten_cmd_base_t *)src) &&
                 ten_raw_msg_get_type(src) == TEN_MSG_TYPE_CMD_RESULT,
             "Should not happen.");

  ten_raw_cmd_result_set_final(
      (ten_cmd_result_t *)self,
      ten_raw_cmd_result_is_final((ten_cmd_result_t *)src, NULL), NULL);
}

bool ten_cmd_result_process_is_final(ten_msg_t *self,
                                     ten_raw_msg_process_one_field_func_t cb,
                                     void *user_data, ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self), "Should not happen.");

  ten_msg_field_process_data_t is_final_field;
  ten_msg_field_process_data_init(&is_final_field, TEN_STR_IS_FINAL,
                                  &((ten_cmd_result_t *)self)->is_final, false);

  return cb(self, &is_final_field, user_data, err);
}
