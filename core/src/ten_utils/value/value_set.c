//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_utils/value/value_set.h"

#include "ten_utils/value/type.h"
#include "ten_utils/value/value.h"

bool ten_value_set_int64(ten_value_t *self, int64_t value) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(self->type == TEN_TYPE_INT64, "Invalid argument.");

  self->content.int64 = value;

  return true;
}

bool ten_value_set_int32(ten_value_t *self, int32_t value) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(self->type == TEN_TYPE_INT32, "Invalid argument.");

  self->content.int32 = value;

  return true;
}

bool ten_value_set_int16(ten_value_t *self, int16_t value) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(self->type == TEN_TYPE_INT16, "Invalid argument.");

  self->content.int16 = value;

  return true;
}

bool ten_value_set_int8(ten_value_t *self, int8_t value) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(self->type == TEN_TYPE_INT8, "Invalid argument.");

  self->content.int8 = value;

  return true;
}

bool ten_value_set_uint64(ten_value_t *self, uint64_t value) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(self->type == TEN_TYPE_UINT64, "Invalid argument.");

  self->content.uint64 = value;

  return true;
}

bool ten_value_set_uint32(ten_value_t *self, uint32_t value) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(self->type == TEN_TYPE_UINT32, "Invalid argument.");

  self->content.uint32 = value;

  return true;
}

bool ten_value_set_uint16(ten_value_t *self, uint16_t value) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(self->type == TEN_TYPE_UINT16, "Invalid argument.");

  self->content.uint16 = value;

  return true;
}

bool ten_value_set_uint8(ten_value_t *self, uint8_t value) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(self->type == TEN_TYPE_UINT8, "Invalid argument.");

  self->content.uint8 = value;

  return true;
}

bool ten_value_set_bool(ten_value_t *self, bool value) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(self->type == TEN_TYPE_BOOL, "Invalid argument.");

  self->content.boolean = value;

  return true;
}
