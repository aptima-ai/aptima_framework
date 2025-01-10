//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/msg_conversion/msg_conversion/per_property/from_original.h"

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/msg/field/properties.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_kv.h"

static void axis_msg_conversion_per_property_rule_from_original_init(
    axis_msg_conversion_per_property_rule_from_original_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_string_init(&self->original_path);
}

void axis_msg_conversion_per_property_rule_from_original_deinit(
    axis_msg_conversion_per_property_rule_from_original_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_string_deinit(&self->original_path);
}

static axis_value_t *
axis_msg_conversion_per_property_rule_from_original_get_value(
    axis_msg_conversion_per_property_rule_from_original_t *self,
    axis_shared_ptr_t *msg) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(msg && axis_msg_check_integrity(msg), "Invalid argument.");

  return axis_msg_peek_property(
      msg, axis_string_get_raw_str(&self->original_path), NULL);
}

bool axis_msg_conversion_per_property_rule_from_original_convert(
    axis_msg_conversion_per_property_rule_from_original_t *self,
    axis_shared_ptr_t *msg, axis_shared_ptr_t *new_msg,
    const char *new_msg_property_path, axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(msg && axis_msg_check_integrity(msg), "Invalid argument.");
  axis_ASSERT(new_msg && axis_msg_check_integrity(new_msg), "Invalid argument.");
  axis_ASSERT(new_msg_property_path, "Invalid argument.");

  axis_value_t *msg_property =
      axis_msg_conversion_per_property_rule_from_original_get_value(self, msg);

  return axis_msg_set_property(
      new_msg, new_msg_property_path,
      msg_property ? axis_value_clone(msg_property) : axis_value_create_invalid(),
      err);
}

void axis_msg_conversion_per_property_rule_from_original_from_json(
    axis_msg_conversion_per_property_rule_from_original_t *self,
    axis_json_t *json) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(json, "Invalid argument.");

  axis_msg_conversion_per_property_rule_from_original_init(self);
  axis_string_set_formatted(&self->original_path,
                           axis_json_peek_string_value(axis_json_object_peek(
                               json, axis_STR_ORIGINAL_PATH)));
}

bool axis_msg_conversion_per_property_rule_from_original_to_json(
    axis_msg_conversion_per_property_rule_from_original_t *self,
    axis_json_t *json, axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(json && axis_json_check_integrity(json), "Invalid argument.");

  axis_json_object_set_new(
      json, axis_STR_ORIGINAL_PATH,
      axis_json_create_string(axis_string_get_raw_str(&self->original_path)));

  return true;
}

bool axis_msg_conversion_per_property_rule_from_original_from_value(
    axis_msg_conversion_per_property_rule_from_original_t *self,
    axis_value_t *value, axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(value, "Invalid argument.");

  axis_msg_conversion_per_property_rule_from_original_init(self);
  axis_string_set_formatted(
      &self->original_path,
      axis_value_peek_raw_str(
          axis_value_object_peek(value, axis_STR_ORIGINAL_PATH), err));

  return true;
}

void axis_msg_conversion_per_property_rule_from_original_to_value(
    axis_msg_conversion_per_property_rule_from_original_t *self,
    axis_value_t *value) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(value && axis_value_is_object(value), "Invalid argument.");

  axis_value_t *result =
      axis_value_create_string(axis_string_get_raw_str(&self->original_path));
  axis_value_kv_t *kv = axis_value_kv_create(axis_STR_ORIGINAL_PATH, result);

  axis_list_push_ptr_back(&value->content.object, kv,
                         (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);
}
