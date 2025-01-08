//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/msg/field/src.h"

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/common/loc.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/value/value.h"

void axis_raw_msg_src_copy(axis_msg_t *self, axis_msg_t *src,
                          axis_list_t *excluded_field_ids) {
  axis_ASSERT(src && axis_raw_msg_check_integrity(src), "Should not happen.");
  axis_loc_set_from_loc(&self->src_loc, &src->src_loc);
}

bool axis_raw_msg_src_process(axis_msg_t *self,
                             axis_raw_msg_process_one_field_func_t cb,
                             void *user_data, axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");

  axis_value_t *src_value = axis_loc_to_value(&self->src_loc);
  axis_ASSERT(axis_value_is_object(src_value), "Should not happen.");

  axis_msg_field_process_data_t src_field;
  axis_msg_field_process_data_init(&src_field, axis_STR_SRC, src_value, false);

  bool rc = cb(self, &src_field, user_data, err);

  if (src_field.value_is_changed_after_process) {
    axis_loc_set_from_value(&self->src_loc, src_field.field_value);
  }

  axis_value_destroy(src_value);
  return rc;
}
