//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_utils/value/value_set.h"

#include "axis_utils/container/list.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/value/type.h"
#include "axis_utils/value/value.h"

bool axis_value_set_int64(axis_value_t *self, int64_t value) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(self->type == axis_TYPE_INT64, "Invalid argument.");

  self->content.int64 = value;

  return true;
}

bool axis_value_set_int32(axis_value_t *self, int32_t value) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(self->type == axis_TYPE_INT32, "Invalid argument.");

  self->content.int32 = value;

  return true;
}

bool axis_value_set_int16(axis_value_t *self, int16_t value) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(self->type == axis_TYPE_INT16, "Invalid argument.");

  self->content.int16 = value;

  return true;
}

bool axis_value_set_int8(axis_value_t *self, int8_t value) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(self->type == axis_TYPE_INT8, "Invalid argument.");

  self->content.int8 = value;

  return true;
}

bool axis_value_set_uint64(axis_value_t *self, uint64_t value) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(self->type == axis_TYPE_UINT64, "Invalid argument.");

  self->content.uint64 = value;

  return true;
}

bool axis_value_set_uint32(axis_value_t *self, uint32_t value) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(self->type == axis_TYPE_UINT32, "Invalid argument.");

  self->content.uint32 = value;

  return true;
}

bool axis_value_set_uint16(axis_value_t *self, uint16_t value) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(self->type == axis_TYPE_UINT16, "Invalid argument.");

  self->content.uint16 = value;

  return true;
}

bool axis_value_set_uint8(axis_value_t *self, uint8_t value) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(self->type == axis_TYPE_UINT8, "Invalid argument.");

  self->content.uint8 = value;

  return true;
}

bool axis_value_set_bool(axis_value_t *self, bool value) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(self->type == axis_TYPE_BOOL, "Invalid argument.");

  self->content.boolean = value;

  return true;
}

bool axis_value_set_float32(axis_value_t *self, float value) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(self->type == axis_TYPE_FLOAT32, "Invalid argument.");

  self->content.float32 = value;

  return true;
}

bool axis_value_set_float64(axis_value_t *self, double value) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(self->type == axis_TYPE_FLOAT64, "Invalid argument.");

  self->content.float64 = value;

  return true;
}

bool axis_value_set_string(axis_value_t *self, const char *str) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_value_is_string(self), "Invalid argument.");

  axis_string_set_formatted(&self->content.string, "%.*s", strlen(str), str);

  return true;
}

bool axis_value_set_string_with_size(axis_value_t *self, const char *str,
                                    size_t len) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");
  axis_ASSERT(axis_value_is_string(self), "Invalid argument.");
  axis_ASSERT(str, "Invalid argument.");

  axis_string_set_formatted(&self->content.string, "%.*s", len, str);

  return true;
}

bool axis_value_set_array_with_move(axis_value_t *self, axis_list_t *value) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(self->type == axis_TYPE_ARRAY, "Invalid argument.");
  axis_ASSERT(value, "Invalid argument.");

  axis_list_clear(&self->content.array);
  axis_list_swap(&self->content.array, value);

  return true;
}

bool axis_value_set_object_with_move(axis_value_t *self, axis_list_t *value) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(self->type == axis_TYPE_OBJECT, "Invalid argument.");
  axis_ASSERT(value, "Invalid argument.");

  axis_list_clear(&self->content.object);
  axis_list_swap(&self->content.object, value);

  return true;
}
