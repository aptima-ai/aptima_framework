//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_utils/schema/keywords/keyword_items.h"

#include "include_internal/axis_utils/schema/keywords/keyword.h"
#include "include_internal/axis_utils/schema/schema.h"
#include "include_internal/axis_utils/schema/types/schema_array.h"
#include "axis_runtime/common/errno.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/macro/memory.h"
#include "axis_utils/value/type_operation.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_get.h"
#include "axis_utils/value/value_is.h"

bool axis_schema_keyword_items_check_integrity(
    axis_schema_keyword_items_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  if (axis_signature_get(&self->signature) !=
      axis_SCHEMA_KEYWORD_ITEMS_SIGNATURE) {
    return false;
  }

  if (!axis_schema_keyword_check_integrity(&self->hdr)) {
    return false;
  }

  return true;
}

static bool axis_schema_keyword_items_validate_value(
    axis_schema_keyword_t *self_, axis_value_t *value,
    axis_schema_error_t *schema_err) {
  axis_ASSERT(self_ && axis_schema_keyword_check_integrity(self_),
             "Invalid argument.");
  axis_ASSERT(value && axis_value_check_integrity(value), "Invalid argument.");
  axis_ASSERT(schema_err && axis_schema_error_check_integrity(schema_err),
             "Invalid argument.");

  if (!axis_value_is_array(value)) {
    axis_error_set(schema_err->err, axis_ERRNO_GENERIC,
                  "the value should be an array, but is: %s",
                  axis_type_to_string(axis_value_get_type(value)));
    return false;
  }

  axis_schema_keyword_items_t *self = (axis_schema_keyword_items_t *)self_;
  axis_ASSERT(axis_schema_keyword_items_check_integrity(self),
             "Invalid argument.");

  if (!self->item_schema) {
    axis_ASSERT(0, "Should not happen.");
    return true;
  }

  axis_value_array_foreach(value, value_iter) {
    axis_value_t *value_field = axis_ptr_listnode_get(value_iter.node);
    axis_ASSERT(value_field && axis_value_check_integrity(value_field),
               "Invalid argument.");

    if (!axis_schema_validate_value_with_schema_error(self->item_schema,
                                                     value_field, schema_err)) {
      axis_string_prepend_formatted(&schema_err->path, "[%d]", value_iter.index);
      return false;
    }
  }

  return true;
}

static void axis_schema_keyword_items_destroy(axis_schema_keyword_t *self_) {
  axis_ASSERT(self_, "Invalid argument.");

  axis_schema_keyword_items_t *self = (axis_schema_keyword_items_t *)self_;
  axis_ASSERT(axis_schema_keyword_items_check_integrity(self),
             "Invalid argument.");

  axis_signature_set(&self->signature, 0);
  axis_schema_keyword_deinit(&self->hdr);
  axis_schema_destroy(self->item_schema);
  axis_FREE(self);
}

static bool axis_schema_keyword_items_adjust_value(
    axis_schema_keyword_t *self_, axis_value_t *value,
    axis_schema_error_t *schema_err) {
  axis_ASSERT(self_ && axis_schema_keyword_check_integrity(self_),
             "Invalid argument.");
  axis_ASSERT(value && axis_value_check_integrity(value), "Invalid argument.");
  axis_ASSERT(schema_err && axis_schema_error_check_integrity(schema_err),
             "Invalid argument.");

  axis_schema_keyword_items_t *self = (axis_schema_keyword_items_t *)self_;
  axis_ASSERT(axis_schema_keyword_items_check_integrity(self),
             "Invalid argument.");

  if (!axis_value_is_array(value)) {
    axis_error_set(schema_err->err, axis_ERRNO_GENERIC,
                  "the value should be an array, but is: %s",
                  axis_type_to_string(axis_value_get_type(value)));
    return false;
  }

  if (!self->item_schema) {
    axis_ASSERT(0, "Should not happen.");
    return true;
  }

  axis_value_array_foreach(value, value_iter) {
    axis_value_t *value_field = axis_ptr_listnode_get(value_iter.node);
    axis_ASSERT(value_field && axis_value_check_integrity(value_field),
               "Invalid argument.");

    if (!axis_schema_adjust_value_type_with_schema_error(
            self->item_schema, value_field, schema_err)) {
      axis_string_prepend_formatted(&schema_err->path, "[%d]", value_iter.index);
      return false;
    }
  }

  return true;
}

// Items compatibility:
// 1. The source collection needs to be a subset of the target collection (Not
// support, there is no item count information now).
// 2. The type of the same property in the source collection should be
// compatible with the target.
//
// Note that the `self` and `target` items keyword should not be NULL, otherwise
// their schemas are invalid.
static bool axis_schema_keyword_items_is_compatible(
    axis_schema_keyword_t *self_, axis_schema_keyword_t *target_,
    axis_schema_error_t *schema_err) {
  axis_ASSERT(self_ && target_, "Invalid argument.");
  axis_ASSERT(schema_err && axis_schema_error_check_integrity(schema_err),
             "Invalid argument.");

  axis_schema_keyword_items_t *self = (axis_schema_keyword_items_t *)self_;
  axis_ASSERT(axis_schema_keyword_items_check_integrity(self),
             "Invalid argument.");

  axis_schema_keyword_items_t *target = (axis_schema_keyword_items_t *)target_;
  axis_ASSERT(axis_schema_keyword_items_check_integrity(target),
             "Invalid argument.");

  bool success = axis_schema_is_compatible_with_schema_error(
      self->item_schema, target->item_schema, schema_err);
  if (!success) {
    axis_string_prepend_formatted(&schema_err->path, "[]");
  }

  return success;
}

static axis_schema_keyword_items_t *axis_schema_keyword_items_create(
    axis_schema_array_t *owner, axis_value_t *value) {
  axis_ASSERT(owner && axis_schema_array_check_integrity(owner),
             "Invalid argument.");
  axis_ASSERT(value, "Invalid argument.");

  axis_schema_keyword_items_t *self =
      axis_MALLOC(sizeof(axis_schema_keyword_items_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_signature_set(&self->signature, axis_SCHEMA_KEYWORD_ITEMS_SIGNATURE);

  axis_schema_keyword_init(&self->hdr, axis_SCHEMA_KEYWORD_ITEMS);
  self->hdr.owner = &owner->hdr;
  self->hdr.destroy = axis_schema_keyword_items_destroy;
  self->hdr.validate_value = axis_schema_keyword_items_validate_value;
  self->hdr.adjust_value = axis_schema_keyword_items_adjust_value;
  self->hdr.is_compatible = axis_schema_keyword_items_is_compatible;

  self->item_schema = axis_schema_create_from_value(value);
  if (!self->item_schema) {
    axis_ASSERT(0, "Failed to parse schema keyword [items].");
    return NULL;
  }

  return self;
}

axis_schema_keyword_t *axis_schema_keyword_items_create_from_value(
    axis_schema_t *owner, axis_value_t *value) {
  axis_ASSERT(owner && axis_schema_check_integrity(owner), "Invalid argument.");
  axis_ASSERT(value && axis_value_check_integrity(value), "Invalid argument.");

  if (!axis_value_is_object(value)) {
    axis_ASSERT(0, "The schema keyword `items` must be an object.");
    return NULL;
  }

  axis_schema_array_t *schema_array = (axis_schema_array_t *)owner;
  axis_ASSERT(schema_array && axis_schema_array_check_integrity(schema_array),
             "Invalid argument.");

  axis_schema_keyword_items_t *keyword_items =
      axis_schema_keyword_items_create(schema_array, value);
  if (!keyword_items) {
    return NULL;
  }

  schema_array->keyword_items = keyword_items;

  return &keyword_items->hdr;
}
