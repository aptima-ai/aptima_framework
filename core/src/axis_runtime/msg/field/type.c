//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/msg/field/type.h"

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_get.h"
#include "axis_utils/value/value_kv.h"

bool axis_raw_msg_type_from_json(axis_msg_t *self, axis_json_t *json,
                                axis_UNUSED axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(json && axis_json_check_integrity(json), "Should not happen.");

  axis_json_t *axis_json =
      axis_json_object_peek_object(json, axis_STR_UNDERLINE_TEN);
  if (!axis_json) {
    return true;
  }

  axis_json_t *type_json = axis_json_object_peek(axis_json, axis_STR_TYPE);
  if (!type_json) {
    return true;
  }

  axis_json_t *name_json = axis_json_object_peek(axis_json, axis_STR_NAME);

  self->type = axis_msg_type_from_type_and_name_string(
      axis_json_peek_string_value(type_json),
      name_json ? axis_json_peek_string_value(name_json) : NULL);

  return true;
}

bool axis_raw_msg_type_to_json(axis_msg_t *self, axis_json_t *json,
                              axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self) && json,
             "Should not happen.");

  axis_json_t *axis_json =
      axis_json_object_peek_object_forcibly(json, axis_STR_UNDERLINE_TEN);
  axis_ASSERT(axis_json, "Should not happen.");

  axis_json_t *type_json = axis_json_create_string(
      axis_msg_type_to_string(axis_raw_msg_get_type(self)));
  axis_ASSERT(type_json, "Should not happen.");

  axis_json_object_set_new(axis_json, axis_STR_TYPE, type_json);

  return true;
}

void axis_raw_msg_type_copy(axis_msg_t *self, axis_msg_t *src,
                           axis_list_t *excluded_field_ids) {
  axis_ASSERT(src && axis_raw_msg_check_integrity(src), "Should not happen.");
  self->type = src->type;
}

bool axis_raw_msg_type_process(axis_msg_t *self,
                              axis_raw_msg_process_one_field_func_t cb,
                              void *user_data, axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");

  axis_value_t *type_value = axis_value_create_string(
      axis_msg_type_to_string(axis_raw_msg_get_type(self)));

  axis_msg_field_process_data_t type_field;
  axis_msg_field_process_data_init(&type_field, axis_STR_TYPE, type_value, false);

  bool rc = cb(self, &type_field, user_data, err);

  if (type_field.value_is_changed_after_process) {
    self->type = axis_msg_type_from_type_string(
        axis_value_peek_raw_str(type_field.field_value, err));
  }

  axis_value_destroy(type_value);
  return rc;
}
