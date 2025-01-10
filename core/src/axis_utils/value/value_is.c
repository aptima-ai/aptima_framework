//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <float.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>

#include "axis_utils/macro/check.h"
#include "axis_utils/value/type.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_kv.h"

bool axis_value_is_object(axis_value_t *self) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");
  return (self->type == axis_TYPE_OBJECT) ? true : false;
}

bool axis_value_is_array(axis_value_t *self) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");
  return (self->type == axis_TYPE_ARRAY) ? true : false;
}

bool axis_value_is_string(axis_value_t *self) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");
  return (self->type == axis_TYPE_STRING) ? true : false;
}

bool axis_value_is_invalid(axis_value_t *self) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");
  return (self->type == axis_TYPE_INVALID) ? true : false;
}

bool axis_value_is_int8(axis_value_t *self) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");
  return (self->type == axis_TYPE_INT8) ? true : false;
}

bool axis_value_is_int16(axis_value_t *self) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");
  return (self->type == axis_TYPE_INT16) ? true : false;
}

bool axis_value_is_int32(axis_value_t *self) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");
  return (self->type == axis_TYPE_INT32) ? true : false;
}

bool axis_value_is_int64(axis_value_t *self) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");
  return (self->type == axis_TYPE_INT64) ? true : false;
}

bool axis_value_is_uint8(axis_value_t *self) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");
  return (self->type == axis_TYPE_UINT8) ? true : false;
}

bool axis_value_is_uint16(axis_value_t *self) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");
  return (self->type == axis_TYPE_UINT16) ? true : false;
}

bool axis_value_is_uint32(axis_value_t *self) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");
  return (self->type == axis_TYPE_UINT32) ? true : false;
}

bool axis_value_is_uint64(axis_value_t *self) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");
  return (self->type == axis_TYPE_UINT64) ? true : false;
}

bool axis_value_is_float32(axis_value_t *self) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");
  return (self->type == axis_TYPE_FLOAT32) ? true : false;
}

bool axis_value_is_float64(axis_value_t *self) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");
  return (self->type == axis_TYPE_FLOAT64) ? true : false;
}

bool axis_value_is_number(axis_value_t *self) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");

  if (axis_value_is_int32(self) || axis_value_is_int64(self) ||
      axis_value_is_float32(self) || axis_value_is_float64(self)) {
    return true;
  }
  return false;
}

bool axis_value_is_null(axis_value_t *self) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");
  return (self->type == axis_TYPE_NULL) ? true : false;
}

bool axis_value_is_bool(axis_value_t *self) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");
  return (self->type == axis_TYPE_BOOL) ? true : false;
}

bool axis_value_is_ptr(axis_value_t *self) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");
  return (self->type == axis_TYPE_PTR) ? true : false;
}

bool axis_value_is_buf(axis_value_t *self) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");
  return (self->type == axis_TYPE_BUF) ? true : false;
}
