//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/schema_store/msg.h"

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/schema_store/property.h"
#include "include_internal/axis_utils/schema/schema.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/memory.h"
#include "axis_utils/value/value.h"

bool axis_msg_schema_check_integrity(axis_msg_schema_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  if (axis_signature_get(&self->signature) != axis_MSG_SCHEMA_SIGNATURE) {
    return false;
  }

  return true;
}

void axis_msg_schema_init(axis_msg_schema_t *self,
                         axis_value_t *msg_schema_value) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(msg_schema_value && axis_value_check_integrity(msg_schema_value),
             "Invalid argument.");

  if (!axis_value_is_object(msg_schema_value)) {
    axis_ASSERT(0, "The schema should be an object.");
    return;
  }

  axis_signature_set(&self->signature, axis_MSG_SCHEMA_SIGNATURE);

  const char *msg_name =
      axis_value_object_peek_string(msg_schema_value, axis_STR_NAME);
  if (!msg_name) {
    msg_name = axis_STR_MSG_NAME_axis_EMPTY;
  }

  axis_string_init_formatted(&self->msg_name, "%s", msg_name);
  self->property =
      axis_schemas_parse_schema_object_for_property(msg_schema_value);
}

void axis_msg_schema_deinit(axis_msg_schema_t *self) {
  axis_ASSERT(self && axis_msg_schema_check_integrity(self), "Invalid argument.");

  axis_signature_set(&self->signature, 0);
  axis_string_deinit(&self->msg_name);
  if (self->property) {
    axis_schema_destroy(self->property);
    self->property = NULL;
  }
}

axis_msg_schema_t *axis_msg_schema_create(axis_value_t *msg_schema_value) {
  axis_ASSERT(msg_schema_value && axis_value_check_integrity(msg_schema_value),
             "Invalid argument.");

  axis_msg_schema_t *self = axis_MALLOC(sizeof(axis_msg_schema_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_msg_schema_init(self, msg_schema_value);

  return self;
}

void axis_msg_schema_destroy(axis_msg_schema_t *self) {
  axis_ASSERT(self && axis_msg_schema_check_integrity(self), "Invalid argument.");

  axis_msg_schema_deinit(self);
  axis_FREE(self);
}

bool axis_msg_schema_adjust_properties(axis_msg_schema_t *self,
                                      axis_value_t *msg_props,
                                      axis_error_t *err) {
  axis_ASSERT(self && axis_msg_schema_check_integrity(self), "Invalid argument.");
  axis_ASSERT(msg_props && axis_value_check_integrity(msg_props),
             "Invalid argument.");
  axis_ASSERT(err && axis_error_check_integrity(err), "Invalid argument.");

  if (!self->property) {
    // No `property` schema is defined, which is permitted in TEN runtime.
    return true;
  }

  return axis_schema_adjust_value_type(self->property, msg_props, err);
}

bool axis_msg_schema_validate_properties(axis_msg_schema_t *self,
                                        axis_value_t *msg_props,
                                        axis_error_t *err) {
  axis_ASSERT(self && axis_msg_schema_check_integrity(self), "Invalid argument.");
  axis_ASSERT(msg_props && axis_value_check_integrity(msg_props),
             "Invalid argument.");
  axis_ASSERT(err && axis_error_check_integrity(err), "Invalid argument.");

  if (!self->property) {
    // No `property` schema is defined, which is permitted in TEN runtime.
    return true;
  }

  return axis_schema_validate_value(self->property, msg_props, err);
}

const char *axis_msg_schema_get_msg_name(axis_msg_schema_t *self) {
  axis_ASSERT(self && axis_msg_schema_check_integrity(self), "Invalid argument.");

  return axis_string_get_raw_str(&self->msg_name);
}
