//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "include_internal/ten_runtime/msg/cmd_base/cmd/stop_graph/field/graph_name.h"

#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/extension/extension_info/extension_info.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd/cmd.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd/stop_graph/cmd.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/log/log.h"
#include "ten_utils/macro/check.h"

bool ten_cmd_stop_graph_put_graph_name_to_json(ten_msg_t *self,
                                               ten_json_t *json,
                                               ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self) && json &&
                 ten_raw_msg_get_type(self) == TEN_MSG_TYPE_CMD_STOP_GRAPH,
             "Should not happen.");

  ten_json_t *ten_json =
      ten_json_object_peek_object_forcibly(json, TEN_STR_UNDERLINE_TEN);
  TEN_ASSERT(ten_json, "Should not happen.");

  ten_cmd_stop_graph_t *cmd = (ten_cmd_stop_graph_t *)self;
  ten_json_object_set_new(ten_json, TEN_STR_GRAPH_NAME,
                          ten_json_create_string(ten_string_get_raw_str(
                              ten_raw_cmd_stop_graph_get_graph_name(cmd))));

  return true;
}

bool ten_cmd_stop_graph_get_graph_name_from_json(ten_msg_t *self,
                                                 ten_json_t *json,
                                                 TEN_UNUSED ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self) &&
                 ten_raw_msg_get_type(self) == TEN_MSG_TYPE_CMD_STOP_GRAPH,
             "Should not happen.");
  TEN_ASSERT(json && ten_json_check_integrity(json), "Should not happen.");

  ten_json_t *ten_json =
      ten_json_object_peek_object(json, TEN_STR_UNDERLINE_TEN);
  if (!ten_json) {
    return true;
  }

  ten_json_t *graph_name_json =
      ten_json_object_peek(ten_json, TEN_STR_GRAPH_NAME);
  if (!graph_name_json) {
    return true;
  }

  if (ten_json_is_string(graph_name_json)) {
    ten_cmd_stop_graph_t *cmd = (ten_cmd_stop_graph_t *)self;
    ten_string_init_formatted(&cmd->graph_name,
                              ten_json_peek_string_value(graph_name_json));
  } else {
    TEN_LOGW("graph_name should be a string value.");
  }

  return true;
}

void ten_cmd_stop_graph_copy_graph_name(
    ten_msg_t *self, ten_msg_t *src,
    TEN_UNUSED ten_list_t *excluded_field_ids) {
  TEN_ASSERT(self && src && ten_raw_cmd_check_integrity((ten_cmd_t *)src) &&
                 ten_raw_msg_get_type(src) == TEN_MSG_TYPE_CMD_STOP_GRAPH,
             "Should not happen.");

  ten_string_copy(&((ten_cmd_stop_graph_t *)self)->graph_name,
                  &((ten_cmd_stop_graph_t *)src)->graph_name);
}
