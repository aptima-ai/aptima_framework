//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/msg/loop_fields.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"
#include "axis_utils/value/value_get.h"

void axis_raw_msg_name_copy(axis_msg_t *self, axis_msg_t *src,
                           axis_UNUSED axis_list_t *excluded_field_ids) {
  axis_ASSERT(src && axis_raw_msg_check_integrity(src), "Should not happen.");
  axis_string_copy(axis_value_peek_string(&self->name),
                  axis_value_peek_string(&src->name));
}

bool axis_raw_msg_name_process(axis_msg_t *self,
                              axis_raw_msg_process_one_field_func_t cb,
                              void *user_data, axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");

  axis_msg_field_process_data_t name_field;
  axis_msg_field_process_data_init(&name_field, axis_STR_NAME, &self->name,
                                  false);

  bool rc = cb(self, &name_field, user_data, err);

  if (name_field.value_is_changed_after_process) {
    axis_MSG_TYPE msg_type_spec_by_name = axis_msg_type_from_unique_name_string(
        axis_value_peek_raw_str(name_field.field_value, err));
    if (msg_type_spec_by_name != axis_MSG_TYPE_INVALID) {
      self->type = msg_type_spec_by_name;
    }
  }

  return rc;
}
