//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_utils/schema/keywords/keyword_required.h"

#include "include_internal/axis_utils/schema/keywords/keyword.h"
#include "include_internal/axis_utils/schema/schema.h"
#include "include_internal/axis_utils/schema/types/schema_object.h"
#include "axis_runtime/common/errno.h"
#include "axis_utils/container/list.h"
#include "axis_utils/container/list_node_str.h"
#include "axis_utils/container/list_str.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/memory.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_is.h"
#include "axis_utils/value/value_object.h"

bool axis_schema_keyword_required_check_integrity(
    axis_schema_keyword_required_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  if (axis_signature_get(&self->signature) !=
      axis_SCHEMA_KEYWORD_REQUIRED_SIGNATURE) {
    return false;
  }

  return true;
}

static void axis_schema_keyword_required_destroy(axis_schema_keyword_t *self_) {
  axis_ASSERT(self_, "Invalid argument.");

  axis_schema_keyword_required_t *self = (axis_schema_keyword_required_t *)self_;
  axis_ASSERT(axis_schema_keyword_required_check_integrity(self),
             "Invalid argument.");

  axis_list_clear(&self->required_properties);
  axis_schema_keyword_deinit(&self->hdr);
  axis_FREE(self);
}

static bool axis_schema_keyword_required_validate_value(
    axis_schema_keyword_t *self_, axis_value_t *value,
    axis_schema_error_t *schema_err) {
  axis_ASSERT(self_ && value && schema_err, "Invalid argument.");
  axis_ASSERT(axis_value_check_integrity(value), "Invalid argument.");
  axis_ASSERT(axis_schema_error_check_integrity(schema_err), "Invalid argument.");

  if (!axis_value_is_object(value)) {
    axis_error_set(schema_err->err, axis_ERRNO_GENERIC,
                  "the value should be an object");
    return false;
  }

  axis_schema_keyword_required_t *self = (axis_schema_keyword_required_t *)self_;
  axis_ASSERT(axis_schema_keyword_required_check_integrity(self),
             "Invalid argument.");

  if (axis_list_size(&self->required_properties) == 0) {
    axis_ASSERT(0, "Should not happen.");
    return false;
  }

  axis_string_t absent_keys;
  axis_string_init(&absent_keys);

  axis_list_foreach (&self->required_properties, iter) {
    axis_string_t *required_property = axis_str_listnode_get(iter.node);
    axis_ASSERT(
        required_property && axis_string_check_integrity(required_property),
        "Should not happen.");

    axis_value_t *expected_to_be_present =
        axis_value_object_peek(value, axis_string_get_raw_str(required_property));
    if (!expected_to_be_present) {
      bool is_last =
          axis_list_size(&self->required_properties) == iter.index + 1;
      axis_string_append_formatted(&absent_keys, is_last ? "'%s'" : "'%s', ",
                                  axis_string_get_raw_str(required_property));
    }
  }

  bool result = true;
  if (axis_string_len(&absent_keys) > 0) {
    axis_error_set(schema_err->err, axis_ERRNO_GENERIC,
                  "the required properties are absent: %s",
                  axis_string_get_raw_str(&absent_keys));
    result = false;
  }

  axis_string_deinit(&absent_keys);

  return result;
}

static bool axis_schema_keyword_required_adjust_value(
    axis_schema_keyword_t *self_, axis_value_t *value,
    axis_schema_error_t *schema_err) {
  axis_ASSERT(self_ && value && schema_err, "Invalid argument.");

  // There is no need to adjust the value for the schema keyword 'required'.
  return true;
}

// Required compatibility:
// 1. The source collection needs to be a superset of the target collection.
// 2. Or the target required keyword is undefined.
static bool axis_schema_keyword_required_is_compatible(
    axis_schema_keyword_t *self_, axis_schema_keyword_t *target_,
    axis_schema_error_t *schema_err) {
  axis_ASSERT(schema_err && axis_schema_error_check_integrity(schema_err),
             "Invalid argument.");

  if (!self_) {
    axis_error_set(schema_err->err, axis_ERRNO_GENERIC,
                  "the `required` in the source schema is undefined");
    return false;
  }

  if (!target_) {
    return true;
  }

  axis_schema_keyword_required_t *self = (axis_schema_keyword_required_t *)self_;
  axis_schema_keyword_required_t *target =
      (axis_schema_keyword_required_t *)target_;

  axis_ASSERT(axis_schema_keyword_required_check_integrity(self),
             "Invalid argument.");
  axis_ASSERT(axis_schema_keyword_required_check_integrity(target),
             "Invalid argument.");

  if (axis_list_size(&self->required_properties) <
      axis_list_size(&target->required_properties)) {
    axis_error_set(schema_err->err, axis_ERRNO_GENERIC,
                  "required is incompatible, the size of the source can not be "
                  "less than the target.");
    return false;
  }

  axis_string_t missing_keys;
  axis_string_init(&missing_keys);

  axis_list_foreach (&target->required_properties, iter) {
    axis_string_t *target_required = axis_str_listnode_get(iter.node);
    axis_ASSERT(target_required && axis_string_check_integrity(target_required),
               "Should not happen.");

    if (!axis_list_find_string(&self->required_properties,
                              axis_string_get_raw_str(target_required))) {
      axis_string_append_formatted(&missing_keys, "'%s', ",
                                  axis_string_get_raw_str(target_required));
    }
  }

  bool success = axis_string_is_empty(&missing_keys);
  if (!success) {
    axis_error_set(schema_err->err, axis_ERRNO_GENERIC,
                  "required is incompatible, the properties [%s] are defined "
                  "in the source but not in the target",
                  axis_string_get_raw_str(&missing_keys));
  }

  axis_string_deinit(&missing_keys);

  return success;
}

static axis_schema_keyword_required_t *axis_schema_keyword_required_create(
    axis_schema_object_t *owner) {
  axis_ASSERT(owner, "Invalid argument.");

  axis_schema_keyword_required_t *self =
      axis_MALLOC(sizeof(axis_schema_keyword_required_t));
  if (!self) {
    axis_ASSERT(0, "Failed to allocate memory.");
    return NULL;
  }

  axis_signature_set(&self->signature, axis_SCHEMA_KEYWORD_REQUIRED_SIGNATURE);
  axis_list_init(&self->required_properties);

  axis_schema_keyword_init(&self->hdr, axis_SCHEMA_KEYWORD_REQUIRED);

  self->hdr.owner = &owner->hdr;
  self->hdr.destroy = axis_schema_keyword_required_destroy;
  self->hdr.validate_value = axis_schema_keyword_required_validate_value;
  self->hdr.adjust_value = axis_schema_keyword_required_adjust_value;
  self->hdr.is_compatible = axis_schema_keyword_required_is_compatible;

  owner->keyword_required = self;

  return self;
}

axis_schema_keyword_t *axis_schema_keyword_required_create_from_value(
    axis_schema_t *owner, axis_value_t *value) {
  axis_ASSERT(owner && axis_schema_check_integrity(owner), "Invalid argument.");
  axis_ASSERT(value && axis_value_check_integrity(value), "Invalid argument.");

  if (!axis_value_is_array(value)) {
    axis_ASSERT(0, "The schema keyword 'required' should be an array.");
    return NULL;
  }

  axis_schema_object_t *schema_object = (axis_schema_object_t *)owner;
  axis_ASSERT(schema_object && axis_schema_object_check_integrity(schema_object),
             "Invalid argument.");

  axis_schema_keyword_required_t *self =
      axis_schema_keyword_required_create(schema_object);
  if (!self) {
    return NULL;
  }

  bool has_error = false;

  axis_value_array_foreach(value, iter) {
    axis_value_t *item = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(item && axis_value_check_integrity(item), "Should not happen.");

    if (!axis_value_is_string(item)) {
      axis_ASSERT(0,
                 "The schema keyword 'required' should be an array of "
                 "strings.");
      has_error = true;
      break;
    }

    const char *required_property = axis_value_peek_raw_str(item, NULL);
    axis_ASSERT(required_property, "Should not happen.");

    axis_list_push_str_back(&self->required_properties, required_property);
  }

  if (has_error) {
    axis_schema_keyword_required_destroy(&self->hdr);
    return NULL;
  }

  if (axis_list_size(&self->required_properties) == 0) {
    axis_schema_keyword_required_destroy(&self->hdr);
    axis_ASSERT(0, "The schema keyword 'required' should not be empty.");
    return NULL;
  }

  return &self->hdr;
}
