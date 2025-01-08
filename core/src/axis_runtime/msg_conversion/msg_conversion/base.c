//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/msg_conversion/msg_conversion/base.h"

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/msg_conversion/msg_conversion/per_property/per_property.h"
#include "axis_runtime/common/errno.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_kv.h"

void axis_msg_conversion_destroy(axis_msg_conversion_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  switch (self->type) {
    case axis_MSG_CONVERSION_TYPE_PER_PROPERTY:
      axis_msg_conversion_per_property_destroy(
          (axis_msg_conversion_per_property_t *)self);
      break;
    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }
}

axis_msg_conversion_t *axis_msg_conversion_from_json(axis_json_t *json,
                                                   axis_error_t *err) {
  axis_ASSERT(json, "Invalid argument.");

  const char *type = axis_json_object_peek_string(json, axis_STR_TYPE);

  if (axis_c_string_is_equal(type, axis_STR_PER_PROPERTY)) {
    return (axis_msg_conversion_t *)axis_msg_conversion_per_property_from_json(
        json, err);
  } else {
    if (err) {
      axis_error_set(err, axis_ERRNO_INVALID_JSON,
                    "Invalid message conversion operation type %s", type);
    }
    axis_ASSERT(0, "Should not happen.");
    return NULL;
  }
}

axis_json_t *axis_msg_conversion_to_json(axis_msg_conversion_t *self,
                                       axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");

  switch (self->type) {
    case axis_MSG_CONVERSION_TYPE_PER_PROPERTY:
      return axis_msg_conversion_per_property_to_json(
          (axis_msg_conversion_per_property_t *)self, err);
    default:
      axis_ASSERT(0, "Should not happen.");
      return NULL;
  }
}

axis_msg_conversion_t *axis_msg_conversion_from_value(axis_value_t *value,
                                                    axis_error_t *err) {
  axis_value_t *type_value = axis_value_object_peek(value, axis_STR_TYPE);
  if (!type_value) {
    if (err) {
      axis_error_set(err, axis_ERRNO_INVALID_JSON, "operation_type is missing.");
    }
    return NULL;
  }

  if (!axis_value_is_string(type_value)) {
    if (err) {
      axis_error_set(err, axis_ERRNO_INVALID_JSON,
                    "operation_type is not a string.");
    }
    return NULL;
  }

  const char *type_str = axis_value_peek_raw_str(type_value, err);
  if (!type_str) {
    axis_ASSERT(0, "Should not happen.");
    return NULL;
  }

  if (!strcmp(type_str, axis_STR_PER_PROPERTY)) {
    return (axis_msg_conversion_t *)axis_msg_conversion_per_property_from_value(
        value, err);
  } else {
    if (err) {
      axis_error_set(err, axis_ERRNO_INVALID_JSON,
                    "Unsupported operation type %s", type_str);
    }
    return NULL;
  }
}

axis_value_t *axis_msg_conversion_to_value(axis_msg_conversion_t *self,
                                         axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");

  switch (self->type) {
    case axis_MSG_CONVERSION_TYPE_PER_PROPERTY:
      return axis_msg_conversion_per_property_to_value(
          (axis_msg_conversion_per_property_t *)self, err);
    default:
      axis_ASSERT(0, "Should not happen.");
      return NULL;
  }
}

axis_shared_ptr_t *axis_msg_conversion_convert(axis_msg_conversion_t *self,
                                             axis_shared_ptr_t *msg,
                                             axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(msg && axis_msg_check_integrity(msg), "Invalid argument.");

  return self->operation(self, msg, err);
}
