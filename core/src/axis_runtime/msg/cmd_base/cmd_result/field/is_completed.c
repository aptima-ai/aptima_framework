//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/msg/cmd_base/cmd_result/field/is_completed.h"

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd_result/cmd.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "axis_runtime/msg/cmd_result/cmd_result.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"

void axis_cmd_result_copy_is_completed(
    axis_msg_t *self, axis_msg_t *src,
    axis_UNUSED axis_list_t *excluded_field_ids) {
  axis_ASSERT(self && src &&
                 axis_raw_cmd_base_check_integrity((axis_cmd_base_t *)src) &&
                 axis_raw_msg_get_type(src) == axis_MSG_TYPE_CMD_RESULT,
             "Should not happen.");

  axis_raw_cmd_result_set_completed(
      (axis_cmd_result_t *)self,
      axis_raw_cmd_result_is_completed((axis_cmd_result_t *)src, NULL), NULL);
}

bool axis_cmd_result_process_is_completed(
    axis_msg_t *self, axis_raw_msg_process_one_field_func_t cb, void *user_data,
    axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");

  axis_msg_field_process_data_t is_completed_field;
  axis_msg_field_process_data_init(&is_completed_field, axis_STR_IS_FINAL,
                                  &((axis_cmd_result_t *)self)->is_completed,
                                  false);

  return cb(self, &is_completed_field, user_data, err);
}
