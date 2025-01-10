//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/msg_conversion/msg_conversion/per_property/per_property.h"

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/msg_conversion/msg_conversion/base.h"
#include "include_internal/axis_runtime/msg_conversion/msg_conversion/per_property/rules.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_is.h"

static axis_shared_ptr_t *axis_msg_conversion_per_property_convert(
    axis_msg_conversion_t *msg_conversion, axis_shared_ptr_t *msg,
    axis_error_t *err) {
  axis_ASSERT(msg_conversion, "Should not happen.");
  axis_ASSERT(msg && axis_msg_check_integrity(msg), "Invalid argument.");

  axis_msg_conversion_per_property_t *per_property_msg_conversion =
      (axis_msg_conversion_per_property_t *)msg_conversion;

  axis_shared_ptr_t *new_msg = NULL;

  if (axis_msg_get_type(msg) == axis_MSG_TYPE_CMD_RESULT) {
    new_msg = axis_result_conversion_per_property_rules_convert(
        per_property_msg_conversion->rules, msg, err);
  } else {
    new_msg = axis_msg_conversion_per_property_rules_convert(
        per_property_msg_conversion->rules, msg, err);
  }

  return new_msg;
}

axis_msg_conversion_per_property_t *axis_msg_conversion_per_property_create(
    axis_msg_conversion_per_property_rules_t *rules) {
  axis_msg_conversion_per_property_t *self =
      (axis_msg_conversion_per_property_t *)axis_MALLOC(
          sizeof(axis_msg_conversion_per_property_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  self->base.type = axis_MSG_CONVERSION_TYPE_PER_PROPERTY;
  self->base.operation = axis_msg_conversion_per_property_convert;

  self->rules = rules;

  return self;
}

void axis_msg_conversion_per_property_destroy(
    axis_msg_conversion_per_property_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_msg_conversion_per_property_rules_destroy(self->rules);

  axis_FREE(self);
}

axis_msg_conversion_per_property_t *axis_msg_conversion_per_property_from_json(
    axis_json_t *json, axis_error_t *err) {
  axis_msg_conversion_per_property_rules_t *rules =
      axis_msg_conversion_per_property_rules_from_json(
          axis_json_object_peek(json, axis_STR_RULES), err);
  if (!rules) {
    return NULL;
  }

  axis_msg_conversion_per_property_t *self =
      axis_msg_conversion_per_property_create(rules);

  axis_json_t *keep_original_json =
      axis_json_object_peek(json, axis_STR_KEEP_ORIGINAL);
  if (keep_original_json != NULL && axis_json_is_true(keep_original_json)) {
    self->rules->keep_original = true;
  }

  return self;
}

axis_json_t *axis_msg_conversion_per_property_to_json(
    axis_msg_conversion_per_property_t *self, axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");

  axis_json_t *json = axis_json_create_object();
  axis_json_object_set_new(json, axis_STR_TYPE,
                          axis_json_create_string(axis_STR_PER_PROPERTY));
  if (self->rules->keep_original) {
    axis_json_object_set_new(json, axis_STR_KEEP_ORIGINAL,
                            axis_json_create_boolean(true));
  }

  axis_json_t *rules_json =
      axis_msg_conversion_per_property_rules_to_json(self->rules, err);
  if (!rules_json) {
    axis_json_destroy(json);
    return NULL;
  }

  axis_json_object_set_new(json, axis_STR_RULES, rules_json);

  return json;
}

axis_msg_conversion_per_property_t *axis_msg_conversion_per_property_from_value(
    axis_value_t *value, axis_error_t *err) {
  if (!value || !axis_value_is_object(value)) {
    return NULL;
  }

  axis_msg_conversion_per_property_t *self =
      axis_msg_conversion_per_property_create(
          axis_msg_conversion_per_property_rules_from_value(
              axis_value_object_peek(value, axis_STR_RULES), err));

  axis_value_t *keep_original_value =
      axis_value_object_peek(value, axis_STR_KEEP_ORIGINAL);
  if (keep_original_value != NULL && axis_value_is_bool(keep_original_value)) {
    self->rules->keep_original = axis_value_get_bool(keep_original_value, err);
  }

  return self;
}

axis_value_t *axis_msg_conversion_per_property_to_value(
    axis_msg_conversion_per_property_t *self, axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");

  axis_value_t *result = axis_value_create_object_with_move(NULL);

  axis_value_kv_t *type_kv = axis_value_kv_create(
      axis_STR_TYPE, axis_value_create_string(axis_STR_PER_PROPERTY));
  axis_list_push_ptr_back(&result->content.object, type_kv,
                         (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);

  if (self->rules->keep_original) {
    axis_value_kv_t *keep_original_kv =
        axis_value_kv_create(axis_STR_KEEP_ORIGINAL,
                            axis_value_create_bool(self->rules->keep_original));
    axis_list_push_ptr_back(
        &result->content.object, keep_original_kv,
        (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);
  }

  axis_value_t *rules_value =
      axis_msg_conversion_per_property_rules_to_value(self->rules, err);
  if (!rules_value) {
    axis_value_destroy(result);
    return NULL;
  }

  axis_value_kv_t *rules_kv = axis_value_kv_create(axis_STR_RULES, rules_value);
  axis_list_push_ptr_back(&result->content.object, rules_kv,
                         (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);

  return result;
}
