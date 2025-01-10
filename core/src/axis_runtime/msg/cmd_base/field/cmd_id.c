//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/msg/cmd_base/field/cmd_id.h"

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd_base.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"
#include "axis_utils/value/value_get.h"

void axis_cmd_base_copy_cmd_id(axis_msg_t *self, axis_msg_t *src,
                              axis_UNUSED axis_list_t *excluded_field_ids) {
  axis_ASSERT(src && axis_raw_msg_check_integrity(src), "Should not happen.");

  axis_string_set_formatted(
      axis_value_peek_string(&((axis_cmd_base_t *)self)->cmd_id), "%s",
      axis_value_peek_raw_str(&((axis_cmd_base_t *)src)->cmd_id, NULL));
}

bool axis_cmd_base_process_cmd_id(axis_msg_t *self,
                                 axis_raw_msg_process_one_field_func_t cb,
                                 void *user_data, axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");

  axis_cmd_base_t *cmd = (axis_cmd_base_t *)self;

  axis_msg_field_process_data_t cmd_id_field;
  axis_msg_field_process_data_init(&cmd_id_field, axis_STR_CMD_ID, &cmd->cmd_id,
                                  false);

  return cb(self, &cmd_id_field, user_data, err);
}
