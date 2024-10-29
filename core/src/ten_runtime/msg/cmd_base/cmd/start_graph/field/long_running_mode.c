//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/extension/extension_info/extension_info.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd/cmd.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd/start_graph/cmd.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "include_internal/ten_utils/value/value_set.h"
#include "ten_runtime/msg/cmd/start_graph/cmd.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"

bool ten_cmd_start_graph_put_long_running_mode_to_json(ten_msg_t *self,
                                                       ten_json_t *json,
                                                       ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self) && json &&
                 ten_raw_msg_get_type(self) == TEN_MSG_TYPE_CMD_START_GRAPH,
             "Should not happen.");

  ten_json_t *ten_json =
      ten_json_object_peek_object_forcibly(json, TEN_STR_UNDERLINE_TEN);
  TEN_ASSERT(ten_json, "Should not happen.");

  ten_cmd_start_graph_t *cmd = (ten_cmd_start_graph_t *)self;
  ten_json_object_set_new(
      ten_json, TEN_STR_LONG_RUNNING_MODE,
      ten_json_create_boolean(
          ten_raw_cmd_start_graph_get_long_running_mode(cmd)));

  return true;
}

bool ten_cmd_start_graph_get_long_running_mode_from_json(
    ten_msg_t *self, ten_json_t *json, TEN_UNUSED ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self) &&
                 ten_raw_msg_get_type(self) == TEN_MSG_TYPE_CMD_START_GRAPH,
             "Should not happen.");
  TEN_ASSERT(json && ten_json_check_integrity(json), "Should not happen.");

  ten_json_t *ten_json =
      ten_json_object_peek_object(json, TEN_STR_UNDERLINE_TEN);
  if (!ten_json) {
    return true;
  }

  ten_json_t *long_running_mode_json =
      ten_json_object_peek(ten_json, TEN_STR_LONG_RUNNING_MODE);
  if (!long_running_mode_json) {
    return true;
  }

  if (ten_json_is_boolean(long_running_mode_json)) {
    ten_cmd_start_graph_t *cmd = (ten_cmd_start_graph_t *)self;

    ten_value_set_bool(&cmd->long_running_mode,
                       ten_json_get_boolean_value(long_running_mode_json));
  } else {
    TEN_LOGW("long_running_mode should be a boolean value.");
  }

  return true;
}

void ten_cmd_start_graph_copy_long_running_mode(
    ten_msg_t *self, ten_msg_t *src, ten_list_t *excluded_field_ids) {
  TEN_ASSERT(self && src && ten_raw_cmd_check_integrity((ten_cmd_t *)src) &&
                 ten_raw_msg_get_type(src) == TEN_MSG_TYPE_CMD_START_GRAPH,
             "Should not happen.");

  ten_value_set_bool(&((ten_cmd_start_graph_t *)self)->long_running_mode,
                     ten_raw_cmd_start_graph_get_long_running_mode(
                         (ten_cmd_start_graph_t *)src));
}

bool ten_cmd_start_graph_process_long_running_mode(
    ten_msg_t *self, ten_raw_msg_process_one_field_func_t cb, void *user_data,
    ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self), "Should not happen.");

  ten_msg_field_process_data_t long_running_mode_field;
  ten_msg_field_process_data_init(
      &long_running_mode_field, TEN_STR_LONG_RUNNING_MODE,
      &((ten_cmd_start_graph_t *)self)->long_running_mode, false);

  return cb(self, &long_running_mode_field, user_data, err);
}
