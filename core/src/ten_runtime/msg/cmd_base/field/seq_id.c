//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/msg/cmd_base/field/seq_id.h"

#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd_base.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"

bool ten_cmd_base_put_seq_id_to_json(ten_msg_t *self, ten_json_t *json,
                                     ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self) &&
                 ten_raw_msg_is_cmd_base(self) && json,
             "Should not happen.");

  ten_json_t *ten_json =
      ten_json_object_peek_object_forcibly(json, TEN_STR_UNDERLINE_TEN);
  TEN_ASSERT(ten_json, "Should not happen.");

  ten_cmd_base_t *cmd = (ten_cmd_base_t *)self;
  ten_json_t *seq_id_json =
      ten_json_create_string(ten_string_get_raw_str(&cmd->seq_id));
  TEN_ASSERT(seq_id_json, "Should not happen.");

  ten_json_object_set_new(ten_json, TEN_STR_SEQ_ID, seq_id_json);

  return true;
}

bool ten_cmd_base_get_seq_id_from_json(ten_msg_t *self, ten_json_t *json,
                                       TEN_UNUSED ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self) &&
                 ten_raw_msg_is_cmd_base(self),
             "Should not happen.");
  TEN_ASSERT(json && ten_json_check_integrity(json), "Should not happen.");

  ten_json_t *ten_json =
      ten_json_object_peek_object(json, TEN_STR_UNDERLINE_TEN);
  if (!ten_json) {
    return true;
  }

  ten_json_t *seq_id_json = ten_json_object_peek(ten_json, TEN_STR_SEQ_ID);
  if (!seq_id_json) {
    return true;
  }

  if (ten_json_is_string(seq_id_json)) {
    ten_raw_cmd_base_set_seq_id((ten_cmd_base_t *)self,
                                ten_json_peek_string_value(seq_id_json));
  } else {
    TEN_LOGW("seq_id should be a string.");
  }

  return true;
}

void ten_cmd_base_copy_seq_id(ten_msg_t *self, ten_msg_t *src,
                              TEN_UNUSED ten_list_t *excluded_field_ids) {
  TEN_ASSERT(src && ten_raw_msg_check_integrity(src), "Should not happen.");

  ten_string_init_formatted(
      &((ten_cmd_base_t *)self)->seq_id, "%s",
      ten_string_get_raw_str(&((ten_cmd_base_t *)src)->seq_id));
}
