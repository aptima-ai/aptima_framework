//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/msg/cmd_base/cmd/stop_graph/field/graph_id.h"

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/extension/extension_info/extension_info.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/cmd.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/stop_graph/cmd.h"
#include "include_internal/axis_runtime/msg/loop_fields.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"

void axis_cmd_stop_graph_copy_graph_id(
    axis_msg_t *self, axis_msg_t *src,
    axis_UNUSED axis_list_t *excluded_field_ids) {
  axis_ASSERT(self && src && axis_raw_cmd_check_integrity((axis_cmd_t *)src) &&
                 axis_raw_msg_get_type(src) == axis_MSG_TYPE_CMD_STOP_GRAPH,
             "Should not happen.");

  axis_string_copy(
      axis_value_peek_string(&((axis_cmd_stop_graph_t *)self)->graph_id),
      axis_value_peek_string(&((axis_cmd_stop_graph_t *)src)->graph_id));
}

bool axis_cmd_stop_graph_process_graph_id(
    axis_msg_t *self, axis_raw_msg_process_one_field_func_t cb, void *user_data,
    axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");

  axis_msg_field_process_data_t graph_id_field;
  axis_msg_field_process_data_init(&graph_id_field, axis_STR_GRAPH_ID,
                                  &((axis_cmd_stop_graph_t *)self)->graph_id,
                                  false);

  return cb(self, &graph_id_field, user_data, err);
}
