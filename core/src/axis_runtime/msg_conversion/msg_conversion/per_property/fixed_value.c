//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/msg_conversion/msg_conversion/per_property/fixed_value.h"

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/msg/field/properties.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_utils/value/value_convert.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/value/type.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_get.h"
#include "axis_utils/value/value_json.h"
#include "axis_utils/value/value_kv.h"

static void axis_msg_conversion_per_property_rule_fixed_value_init(
    axis_msg_conversion_per_property_rule_fixed_value_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  self->value = NULL;
}

void axis_msg_conversion_per_property_rule_fixed_value_deinit(
    axis_msg_conversion_per_property_rule_fixed_value_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  if (self->value) {
    axis_value_destroy(self->value);
    self->value = NULL;
  }
}

bool axis_msg_conversion_per_property_rule_fixed_value_convert(
    axis_msg_conversion_per_property_rule_fixed_value_t *self,
    axis_shared_ptr_t *new_msg, const char *new_msg_property_path,
    axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(new_msg && axis_msg_check_integrity(new_msg), "Invalid argument.");
  axis_ASSERT(new_msg_property_path, "Invalid argument.");

  return axis_msg_set_property(new_msg, new_msg_property_path,
                              axis_value_clone(self->value), err);
}

bool axis_msg_conversion_per_property_rule_fixed_value_from_json(
    axis_msg_conversion_per_property_rule_fixed_value_t *self, axis_json_t *json,
    axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(json, "Invalid argument.");

  axis_msg_conversion_per_property_rule_fixed_value_init(self);

  axis_json_t *value_json = axis_json_object_peek(json, axis_STR_VALUE);
  axis_ASSERT(value_json, "Should not happen.");

  axis_TYPE data_type = axis_json_get_type(value_json);

  switch (data_type) {
    case axis_TYPE_INT64:
      self->value =
          axis_value_create_int64(axis_json_get_integer_value(value_json));
      break;
    case axis_TYPE_UINT64:
      self->value =
          axis_value_create_uint64(axis_json_get_integer_value(value_json));
      break;
    case axis_TYPE_FLOAT64:
      self->value =
          axis_value_create_float64(axis_json_get_real_value(value_json));
      break;
    case axis_TYPE_BOOL:
      self->value =
          axis_value_create_bool(axis_json_get_boolean_value(value_json));
      break;
    case axis_TYPE_STRING:
      self->value =
          axis_value_create_string(axis_json_peek_string_value(value_json));
      break;
    default:
      axis_ASSERT(0, "Handle more types: %d", data_type);
      break;
  }

  if (!self->value) {
    return false;
  }

  return true;
}

bool axis_msg_conversion_per_property_rule_fixed_value_to_json(
    axis_msg_conversion_per_property_rule_fixed_value_t *self, axis_json_t *json,
    axis_error_t *err) {
  axis_ASSERT(self && self->value && axis_value_check_integrity(self->value),
             "Invalid argument.");
  axis_ASSERT(json && axis_json_check_integrity(json), "Invalid argument.");

  switch (self->value->type) {
    case axis_TYPE_INT8:
    case axis_TYPE_INT16:
    case axis_TYPE_INT32:
    case axis_TYPE_INT64:
    case axis_TYPE_UINT8:
    case axis_TYPE_UINT16:
    case axis_TYPE_UINT32:
    case axis_TYPE_UINT64: {
      int64_t val = axis_value_get_int64(self->value, err);
      if (!axis_error_is_success(err)) {
        return false;
      }
      axis_json_object_set_new(json, axis_STR_VALUE,
                              axis_json_create_integer(val));
      break;
    }

    case axis_TYPE_FLOAT32:
    case axis_TYPE_FLOAT64: {
      double val = axis_value_get_float64(self->value, err);
      if (!axis_error_is_success(err)) {
        return false;
      }
      axis_json_object_set_new(json, axis_STR_VALUE, axis_json_create_real(val));
      break;
    }

    case axis_TYPE_STRING: {
      const char *value_str = axis_value_peek_raw_str(self->value, err);
      axis_json_object_set_new(json, axis_STR_VALUE,
                              axis_json_create_string(value_str));
      break;
    }

    case axis_TYPE_BOOL: {
      bool val = axis_value_get_bool(self->value, err);
      if (!axis_error_is_success(err)) {
        return false;
      }
      axis_json_object_set_new(json, axis_STR_VALUE,
                              axis_json_create_boolean(val));
      break;
    }

    default:
      axis_ASSERT(0, "Handle more types: %d", self->value->type);
      break;
  }

  return true;
}

bool axis_msg_conversion_per_property_rule_fixed_value_from_value(
    axis_msg_conversion_per_property_rule_fixed_value_t *self,
    axis_value_t *value, axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(value, "Invalid argument.");

  axis_msg_conversion_per_property_rule_fixed_value_init(self);

  axis_value_t *fixed_value = axis_value_object_peek(value, axis_STR_VALUE);
  axis_ASSERT(fixed_value && axis_value_check_integrity(fixed_value),
             "Should not happen.");

  axis_TYPE data_type = fixed_value->type;

  switch (data_type) {
    case axis_TYPE_INT8:
    case axis_TYPE_INT16:
    case axis_TYPE_INT32:
    case axis_TYPE_INT64:
    case axis_TYPE_UINT8:
    case axis_TYPE_UINT16:
    case axis_TYPE_UINT32:
    case axis_TYPE_UINT64:
    case axis_TYPE_FLOAT32:
    case axis_TYPE_FLOAT64:
    case axis_TYPE_BOOL:
    case axis_TYPE_STRING:
      self->value = axis_value_clone(fixed_value);
      break;

    default:
      axis_ASSERT(0, "Handle more types: %d", data_type);
      break;
  }

  if (!self->value) {
    return false;
  }

  return true;
}

void axis_msg_conversion_per_property_rule_fixed_value_to_value(
    axis_msg_conversion_per_property_rule_fixed_value_t *self,
    axis_value_t *value) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(value && axis_value_is_object(value), "Invalid argument.");

  axis_value_t *result = axis_value_clone(self->value);
  axis_value_kv_t *kv = axis_value_kv_create(axis_STR_VALUE, result);

  axis_list_push_ptr_back(&value->content.object, kv,
                         (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);
}
