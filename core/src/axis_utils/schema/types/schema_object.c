//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_utils/schema/types/schema_object.h"

#include "include_internal/axis_utils/schema/keywords/keyword_properties.h"
#include "include_internal/axis_utils/schema/schema.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/memory.h"

bool axis_schema_object_check_integrity(axis_schema_object_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  if (axis_signature_get(&self->signature) != axis_SCHEMA_OBJECT_SIGNATURE) {
    return false;
  }

  return true;
}

axis_schema_object_t *axis_schema_object_create(void) {
  axis_schema_object_t *self = axis_MALLOC(sizeof(axis_schema_object_t));
  if (!self) {
    axis_ASSERT(0, "Failed to allocate memory.");
    return NULL;
  }

  axis_signature_set(&self->signature, axis_SCHEMA_OBJECT_SIGNATURE);
  axis_schema_init(&self->hdr);
  self->keyword_properties = NULL;
  self->keyword_required = NULL;

  return self;
}

void axis_schema_object_destroy(axis_schema_object_t *self) {
  axis_ASSERT(self && axis_schema_object_check_integrity(self),
             "Invalid argument.");

  axis_signature_set(&self->signature, 0);

  // The 'keyword_properties' will be destroyed from 'self->hdr.keywords'.
  axis_schema_deinit(&self->hdr);
  self->keyword_properties = NULL;
  self->keyword_required = NULL;
  axis_FREE(self);
}

axis_schema_t *axis_schema_object_peek_property_schema(axis_schema_t *self,
                                                     const char *prop_name) {
  axis_ASSERT(self && axis_schema_check_integrity(self), "Invalid argument.");
  axis_ASSERT(prop_name, "Invalid argument.");

  axis_schema_object_t *object_schema = (axis_schema_object_t *)self;
  axis_ASSERT(axis_schema_object_check_integrity(object_schema),
             "Invalid argument.");

  return axis_schema_keyword_properties_peek_property_schema(
      object_schema->keyword_properties, prop_name);
}
