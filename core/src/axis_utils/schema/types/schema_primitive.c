//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_utils/schema/types/schema_primitive.h"

#include "include_internal/axis_utils/schema/schema.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/memory.h"

bool axis_schema_primitive_check_integrity(axis_schema_primitive_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  if (axis_signature_get(&self->signature) != axis_SCHEMA_PRIMITIVE_SIGNATURE) {
    return false;
  }

  if (!axis_schema_check_integrity(&self->hdr)) {
    return false;
  }

  return true;
}

axis_schema_primitive_t *axis_schema_primitive_create(void) {
  axis_schema_primitive_t *self = axis_MALLOC(sizeof(axis_schema_primitive_t));
  if (self == NULL) {
    axis_ASSERT(0, "Failed to allocate memory.");
    return NULL;
  }

  axis_signature_set(&self->signature, axis_SCHEMA_PRIMITIVE_SIGNATURE);
  axis_schema_init(&self->hdr);

  return self;
}

void axis_schema_primitive_destroy(axis_schema_primitive_t *self) {
  axis_ASSERT(self && axis_schema_primitive_check_integrity(self),
             "Invalid argument.");

  axis_signature_set(&self->signature, 0);
  axis_schema_deinit(&self->hdr);

  axis_FREE(self);
}
