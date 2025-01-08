//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_utils/schema/keywords/keyword_type.h"

#include <stdbool.h>

#include "include_internal/axis_utils/schema/keywords/keyword.h"
#include "include_internal/axis_utils/schema/schema.h"
#include "include_internal/axis_utils/value/value_convert.h"
#include "axis_runtime/common/errno.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/memory.h"
#include "axis_utils/value/type.h"
#include "axis_utils/value/type_operation.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_get.h"
#include "axis_utils/value/value_is.h"

bool axis_schema_keyword_type_check_integrity(axis_schema_keyword_type_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  if (!axis_schema_keyword_check_integrity(&self->hdr)) {
    return false;
  }

  if (axis_signature_get(&self->signature) !=
      axis_SCHEMA_KEYWORD_TYPE_SIGNATURE) {
    return false;
  }

  if (self->type == axis_TYPE_INVALID) {
    return false;
  }

  return true;
}

static void axis_schema_keyword_type_destroy(axis_schema_keyword_t *self_) {
  axis_ASSERT(self_, "Invalid argument.");

  axis_schema_keyword_type_t *self = (axis_schema_keyword_type_t *)self_;
  axis_ASSERT(axis_schema_keyword_type_check_integrity(self),
             "Invalid argument.");

  axis_schema_keyword_deinit(&self->hdr);
  self->type = axis_TYPE_INVALID;

  axis_FREE(self);
}

static bool axis_schema_keyword_type_validate_value(
    axis_schema_keyword_t *self_, axis_value_t *value,
    axis_schema_error_t *schema_err) {
  axis_ASSERT(self_ && value, "Invalid argument.");
  axis_ASSERT(schema_err && axis_schema_error_check_integrity(schema_err),
             "Invalid argument.");

  axis_schema_keyword_type_t *self = (axis_schema_keyword_type_t *)self_;
  axis_ASSERT(axis_schema_keyword_type_check_integrity(self),
             "Invalid argument.");

  axis_TYPE value_type = axis_value_get_type(value);
  if (!axis_type_is_compatible(value_type, self->type)) {
    axis_error_set(schema_err->err, axis_ERRNO_GENERIC,
                  "the value type does not match the schema type, given: %s, "
                  "expected: %s",
                  axis_type_to_string(value_type),
                  axis_type_to_string(self->type));
    return false;
  }

  return true;
}

// Automatically perform TEN-supported value conversion based on the type. This
// conversion is generally safe, meaning the value will not change as a result.
static bool axis_schema_keyword_type_adjust_value(
    axis_schema_keyword_t *self_, axis_value_t *value,
    axis_schema_error_t *schema_err) {
  axis_ASSERT(self_ && value, "Invalid argument.");
  axis_ASSERT(schema_err && axis_schema_error_check_integrity(schema_err),
             "Invalid argument.");

  axis_schema_keyword_type_t *self = (axis_schema_keyword_type_t *)self_;
  axis_ASSERT(axis_schema_keyword_type_check_integrity(self),
             "Invalid argument.");

  axis_TYPE schema_type = self->type;
  axis_TYPE value_type = axis_value_get_type(value);
  if (value_type == schema_type) {
    // The types are consistent, so there's no need to adjust the value; this is
    // a normal situation.
    return true;
  }

  axis_error_t *err = schema_err->err;
  switch (schema_type) {
    case axis_TYPE_INT8:
      return axis_value_convert_to_int8(value, err);
    case axis_TYPE_INT16:
      return axis_value_convert_to_int16(value, err);
    case axis_TYPE_INT32:
      return axis_value_convert_to_int32(value, err);
    case axis_TYPE_INT64:
      return axis_value_convert_to_int64(value, err);
    case axis_TYPE_UINT8:
      return axis_value_convert_to_uint8(value, err);
    case axis_TYPE_UINT16:
      return axis_value_convert_to_uint16(value, err);
    case axis_TYPE_UINT32:
      return axis_value_convert_to_uint32(value, err);
    case axis_TYPE_UINT64:
      return axis_value_convert_to_uint64(value, err);
    case axis_TYPE_FLOAT32:
      return axis_value_convert_to_float32(value, err);
    case axis_TYPE_FLOAT64:
      return axis_value_convert_to_float64(value, err);
    default:
      // Note that the format of error message should be same as the above
      // functions.
      axis_error_set(
          err, axis_ERRNO_GENERIC, "unsupported conversion from `%s` to `%s`",
          axis_type_to_string(value_type), axis_type_to_string(self->type));
      return false;
  }
}

// Type compatibility:
// The target type has a larger range of values than the source type.
//
// Note that the `self` and `target` type keyword should not be NULL, otherwise
// their schemas are invalid.
static bool axis_schema_keyword_type_is_compatible(
    axis_schema_keyword_t *self_, axis_schema_keyword_t *target_,
    axis_schema_error_t *schema_err) {
  axis_ASSERT(self_ && target_, "Invalid argument.");
  axis_ASSERT(schema_err && axis_schema_error_check_integrity(schema_err),
             "Invalid argument.");

  axis_schema_keyword_type_t *self = (axis_schema_keyword_type_t *)self_;
  axis_schema_keyword_type_t *target = (axis_schema_keyword_type_t *)target_;
  axis_ASSERT(axis_schema_keyword_type_check_integrity(self),
             "Invalid argument.");
  axis_ASSERT(axis_schema_keyword_type_check_integrity(target),
             "Invalid argument.");

  bool success = axis_type_is_compatible(self->type, target->type);
  if (!success) {
    axis_error_set(schema_err->err, axis_ERRNO_GENERIC,
                  "type is incompatible, source is [%s], but target is [%s]",
                  axis_type_to_string(self->type),
                  axis_type_to_string(target->type));
  }

  return success;
}

static axis_schema_keyword_type_t *axis_schema_keyword_type_create(
    axis_TYPE type, axis_schema_t *schema) {
  axis_schema_keyword_type_t *self =
      axis_MALLOC(sizeof(axis_schema_keyword_type_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_schema_keyword_init(&self->hdr, axis_SCHEMA_KEYWORD_TYPE);
  self->hdr.destroy = axis_schema_keyword_type_destroy;
  self->hdr.owner = schema;
  self->hdr.validate_value = axis_schema_keyword_type_validate_value;
  self->hdr.adjust_value = axis_schema_keyword_type_adjust_value;
  self->hdr.is_compatible = axis_schema_keyword_type_is_compatible;

  axis_signature_set(&self->signature, axis_SCHEMA_KEYWORD_TYPE_SIGNATURE);
  self->type = type;

  return self;
}

axis_schema_keyword_t *axis_schema_keyword_type_create_from_value(
    axis_schema_t *owner, axis_value_t *value) {
  axis_ASSERT(owner && value, "Invalid argument.");
  axis_ASSERT(axis_schema_check_integrity(owner), "Invalid argument.");
  axis_ASSERT(axis_value_check_integrity(value), "Invalid argument.");

  if (!axis_value_is_string(value)) {
    axis_ASSERT(0, "The schema type should be string.");
    return NULL;
  }

  axis_TYPE type = axis_type_from_string(axis_value_peek_raw_str(value, NULL));
  if (type == axis_TYPE_INVALID) {
    axis_ASSERT(0, "Invalid TEN type.");
    return NULL;
  }

  owner->keyword_type = axis_schema_keyword_type_create(type, owner);
  return &owner->keyword_type->hdr;
}
