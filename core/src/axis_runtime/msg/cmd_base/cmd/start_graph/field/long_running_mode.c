//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/extension/extension_info/extension_info.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/cmd.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/start_graph/cmd.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_utils/value/value_set.h"
#include "axis_runtime/msg/cmd/start_graph/cmd.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"

void axis_cmd_start_graph_copy_long_running_mode(
    axis_msg_t *self, axis_msg_t *src, axis_list_t *excluded_field_ids) {
  axis_ASSERT(self && src && axis_raw_cmd_check_integrity((axis_cmd_t *)src) &&
                 axis_raw_msg_get_type(src) == axis_MSG_TYPE_CMD_START_GRAPH,
             "Should not happen.");

  axis_value_set_bool(&((axis_cmd_start_graph_t *)self)->long_running_mode,
                     axis_raw_cmd_start_graph_get_long_running_mode(
                         (axis_cmd_start_graph_t *)src));
}

bool axis_cmd_start_graph_process_long_running_mode(
    axis_msg_t *self, axis_raw_msg_process_one_field_func_t cb, void *user_data,
    axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");

  axis_msg_field_process_data_t long_running_mode_field;
  axis_msg_field_process_data_init(
      &long_running_mode_field, axis_STR_LONG_RUNNING_MODE,
      &((axis_cmd_start_graph_t *)self)->long_running_mode, false);

  return cb(self, &long_running_mode_field, user_data, err);
}
