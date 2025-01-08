//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_utils/schema/keywords/keyword.h"

#include "axis_utils/macro/check.h"

bool axis_schema_keyword_check_integrity(axis_schema_keyword_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  if (axis_signature_get(&self->signature) != axis_SCHEMA_KEYWORD_SIGNATURE) {
    return false;
  }

  if (self->type <= axis_SCHEMA_KEYWORD_INVALID ||
      self->type >= axis_SCHEMA_KEYWORD_LAST) {
    return false;
  }

  if (!self->destroy) {
    return false;
  }

  return true;
}

void axis_schema_keyword_init(axis_schema_keyword_t *self,
                             axis_SCHEMA_KEYWORD type) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(
      type > axis_SCHEMA_KEYWORD_INVALID && type < axis_SCHEMA_KEYWORD_LAST,
      "Invalid argument.");

  axis_signature_set(&self->signature, axis_SCHEMA_KEYWORD_SIGNATURE);

  self->type = type;
  self->owner = NULL;
  self->destroy = NULL;
  self->validate_value = NULL;
  self->adjust_value = NULL;
  self->is_compatible = NULL;
}

void axis_schema_keyword_deinit(axis_schema_keyword_t *self) {
  axis_ASSERT(self && axis_schema_keyword_check_integrity(self),
             "Invalid argument.");

  self->type = axis_SCHEMA_KEYWORD_INVALID;
  self->owner = NULL;
  self->destroy = NULL;
  self->validate_value = NULL;
  self->adjust_value = NULL;
}
