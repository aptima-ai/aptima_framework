//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_utils/schema/keywords/keyword_properties.h"

#include "include_internal/axis_utils/schema/keywords/keyword.h"
#include "include_internal/axis_utils/schema/schema.h"
#include "include_internal/axis_utils/schema/types/schema_object.h"
#include "axis_runtime/common/errno.h"
#include "axis_utils/container/hash_table.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/memory.h"
#include "axis_utils/value/type_operation.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_is.h"
#include "axis_utils/value/value_kv.h"

bool axis_schema_keyword_properties_check_integrity(
    axis_schema_keyword_properties_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  if (axis_signature_get(&self->signature) !=
      axis_SCHEMA_KEYWORD_PROPERTIES_SIGNATURE) {
    return false;
  }

  if (!axis_schema_keyword_check_integrity(&self->hdr)) {
    return false;
  }

  return true;
}

static void axis_schema_keyword_properties_destroy(axis_schema_keyword_t *self_) {
  axis_ASSERT(self_, "Invalid argument.");

  axis_schema_keyword_properties_t *self =
      (axis_schema_keyword_properties_t *)self_;
  axis_ASSERT(axis_schema_keyword_properties_check_integrity(self),
             "Invalid argument.");

  axis_signature_set(&self->signature, 0);
  axis_hashtable_deinit(&self->properties);
  axis_schema_keyword_deinit(&self->hdr);
  axis_FREE(self);
}

axis_schema_t *axis_schema_keyword_properties_peek_property_schema(
    axis_schema_keyword_properties_t *self, const char *prop_name) {
  axis_ASSERT(self && axis_schema_keyword_properties_check_integrity(self),
             "Invalid argument.");
  axis_ASSERT(prop_name && strlen(prop_name), "Invalid argument.");

  axis_hashhandle_t *hh =
      axis_hashtable_find_string(&self->properties, prop_name);
  if (hh) {
    axis_schema_object_property_t *property_schema =
        CONTAINER_OF_FROM_OFFSET(hh, self->properties.hh_offset);
    axis_ASSERT(property_schema, "Should not happen.");
    return property_schema->schema;
  }

  return NULL;
}

static bool axis_schema_keyword_properties_validate_value(
    axis_schema_keyword_t *self_, axis_value_t *value,
    axis_schema_error_t *schema_err) {
  axis_ASSERT(self_ && value && schema_err, "Invalid argument.");
  axis_ASSERT(axis_schema_keyword_check_integrity(self_), "Invalid argument.");
  axis_ASSERT(axis_value_check_integrity(value), "Invalid argument.");
  axis_ASSERT(axis_schema_error_check_integrity(schema_err), "Invalid argument.");

  if (!axis_value_is_object(value)) {
    axis_error_set(schema_err->err, axis_ERRNO_GENERIC,
                  "the value should be an object, but is: %s",
                  axis_type_to_string(axis_value_get_type(value)));
    return false;
  }

  axis_schema_keyword_properties_t *self =
      (axis_schema_keyword_properties_t *)self_;
  axis_ASSERT(self && axis_schema_keyword_properties_check_integrity(self),
             "Invalid argument.");

  // Only check the fields the `value` has, but not all the fields defined in
  // the schema. In other words, the default value of the `required` keyword is
  // empty.
  axis_value_object_foreach(value, iter) {
    axis_value_kv_t *kv = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(kv && axis_value_kv_check_integrity(kv), "Should not happen.");

    axis_string_t *field_key = &kv->key;
    axis_value_t *field_value = kv->value;
    axis_schema_t *prop_schema =
        axis_schema_keyword_properties_peek_property_schema(
            self, axis_string_get_raw_str(field_key));
    if (!prop_schema) {
      // The schema of some property might not be defined, it's ok. Using the
      // `required` keyword to check if the property is required.
      continue;
    }

    if (!axis_schema_validate_value_with_schema_error(prop_schema, field_value,
                                                     schema_err)) {
      axis_string_prepend_formatted(&schema_err->path, ".%s",
                                   axis_string_get_raw_str(field_key));
      return false;
    }
  }

  return true;
}

static bool axis_schema_keyword_properties_adjust_value(
    axis_schema_keyword_t *self_, axis_value_t *value,
    axis_schema_error_t *schema_err) {
  axis_ASSERT(self_ && value && schema_err, "Invalid argument.");
  axis_ASSERT(axis_schema_keyword_check_integrity(self_), "Invalid argument.");
  axis_ASSERT(axis_value_check_integrity(value), "Invalid argument.");
  axis_ASSERT(axis_schema_error_check_integrity(schema_err), "Invalid argument.");

  if (!axis_value_is_object(value)) {
    axis_error_set(schema_err->err, axis_ERRNO_GENERIC,
                  "the value should be an object, but is: %s",
                  axis_type_to_string(axis_value_get_type(value)));
    return false;
  }

  axis_schema_keyword_properties_t *self =
      (axis_schema_keyword_properties_t *)self_;
  axis_ASSERT(self && axis_schema_keyword_properties_check_integrity(self),
             "Invalid argument.");

  axis_value_object_foreach(value, iter) {
    axis_value_kv_t *kv = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(kv && axis_value_kv_check_integrity(kv), "Should not happen.");

    axis_string_t *field_key = &kv->key;
    axis_value_t *field_value = kv->value;
    axis_schema_t *prop_schema =
        axis_schema_keyword_properties_peek_property_schema(
            self, axis_string_get_raw_str(field_key));
    if (!prop_schema) {
      continue;
    }

    if (!axis_schema_adjust_value_type_with_schema_error(
            prop_schema, field_value, schema_err)) {
      axis_string_prepend_formatted(&schema_err->path, ".%s",
                                   axis_string_get_raw_str(field_key));
      return false;
    }
  }

  return true;
}

// Properties compatibility:
// The type of the same property in the source collection should be compatible
// with the target.
//
// Note that the `self` and `target` properties keyword should not be NULL,
// otherwise their schemas are invalid.
static bool axis_schema_keyword_properties_is_compatible(
    axis_schema_keyword_t *self_, axis_schema_keyword_t *target_,
    axis_schema_error_t *schema_err) {
  axis_ASSERT(self_ && target_, "Invalid argument.");
  axis_ASSERT(schema_err && axis_schema_error_check_integrity(schema_err),
             "Invalid argument.");

  axis_schema_keyword_properties_t *self =
      (axis_schema_keyword_properties_t *)self_;
  axis_ASSERT(self && axis_schema_keyword_properties_check_integrity(self),
             "Invalid argument.");

  axis_schema_keyword_properties_t *target =
      (axis_schema_keyword_properties_t *)target_;
  axis_ASSERT(target && axis_schema_keyword_properties_check_integrity(target),
             "Invalid argument.");

  axis_string_t incompatible_fields;
  axis_string_init(&incompatible_fields);

  axis_hashtable_foreach(&self->properties, iter) {
    axis_schema_object_property_t *property =
        CONTAINER_OF_FROM_OFFSET(iter.node, self->properties.hh_offset);
    axis_ASSERT(property, "Should not happen.");

    axis_schema_t *target_prop_schema =
        axis_schema_keyword_properties_peek_property_schema(
            target, axis_string_get_raw_str(&property->name));
    if (!target_prop_schema) {
      continue;
    }

    if (!axis_schema_is_compatible_with_schema_error(
            property->schema, target_prop_schema, schema_err)) {
      // Ex:
      // .a[0]: type is incompatible, ...; .b: ...
      const char *separator =
          axis_string_is_empty(&incompatible_fields) ? "" : "; ";
      axis_string_append_formatted(&incompatible_fields, "%s.%s%s: %s",
                                  separator,
                                  axis_string_get_raw_str(&property->name),
                                  axis_string_get_raw_str(&schema_err->path),
                                  axis_error_errmsg(schema_err->err));
    }

    axis_schema_error_reset(schema_err);
  }

  bool success = axis_string_is_empty(&incompatible_fields);

  if (!success) {
    axis_error_set(schema_err->err, axis_ERRNO_GENERIC, "{ %s }",
                  axis_string_get_raw_str(&incompatible_fields));
  }

  axis_string_deinit(&incompatible_fields);

  return success;
}

static axis_schema_keyword_properties_t *axis_schema_keyword_properties_create(
    axis_schema_object_t *owner) {
  axis_schema_keyword_properties_t *self =
      axis_MALLOC(sizeof(axis_schema_keyword_properties_t));
  if (self == NULL) {
    axis_ASSERT(0, "Failed to allocate memory.");
    return NULL;
  }

  axis_signature_set(&self->signature, axis_SCHEMA_KEYWORD_PROPERTIES_SIGNATURE);

  axis_schema_keyword_init(&self->hdr, axis_SCHEMA_KEYWORD_PROPERTIES);
  self->hdr.destroy = axis_schema_keyword_properties_destroy;
  self->hdr.owner = &owner->hdr;
  self->hdr.validate_value = axis_schema_keyword_properties_validate_value;
  self->hdr.adjust_value = axis_schema_keyword_properties_adjust_value;
  self->hdr.is_compatible = axis_schema_keyword_properties_is_compatible;

  axis_hashtable_init(&self->properties, offsetof(axis_schema_object_property_t,
                                                 hh_in_properties_table));

  owner->keyword_properties = self;

  return self;
}

static axis_schema_object_property_t *axis_schema_object_property_create(
    const char *name, axis_value_t *value) {
  axis_schema_object_property_t *self =
      axis_MALLOC(sizeof(axis_schema_object_property_t));
  if (self == NULL) {
    axis_ASSERT(0, "Failed to allocate memory.");
    return NULL;
  }

  axis_signature_set(&self->signature, axis_SCHEMA_OBJECT_PROPERTY_SIGNATURE);
  axis_string_init_formatted(&self->name, "%s", name);
  self->schema = axis_schema_create_from_value(value);
  if (!self->schema) {
    axis_ASSERT(0, "Failed to parse schema for property %s.", name);
    return NULL;
  }

  return self;
}

static void axis_schema_object_property_destroy(
    axis_schema_object_property_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_schema_destroy(self->schema);
  axis_string_deinit(&self->name);
  axis_signature_set(&self->signature, 0);
  axis_FREE(self);
}

static void axis_schema_keyword_properties_append_item(
    axis_schema_keyword_properties_t *self,
    axis_schema_object_property_t *property) {
  axis_ASSERT(self && property, "Invalid argument.");

  axis_hashtable_add_string(&self->properties, &property->hh_in_properties_table,
                           axis_string_get_raw_str(&property->name),
                           axis_schema_object_property_destroy);
}

axis_schema_keyword_t *axis_schema_keyword_properties_create_from_value(
    axis_schema_t *owner, axis_value_t *value) {
  axis_ASSERT(owner && value, "Invalid argument.");
  axis_ASSERT(axis_schema_check_integrity(owner), "Invalid argument.");
  axis_ASSERT(axis_value_check_integrity(value), "Invalid argument.");

  if (!axis_value_is_object(value)) {
    axis_ASSERT(0, "The schema keyword properties must be an object.");
    return NULL;
  }

  axis_schema_object_t *schema_object = (axis_schema_object_t *)owner;
  axis_ASSERT(axis_schema_object_check_integrity(schema_object),
             "Invalid argument.");

  axis_schema_keyword_properties_t *keyword_properties =
      axis_schema_keyword_properties_create(schema_object);

  axis_value_object_foreach(value, iter) {
    axis_value_kv_t *field_kv = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(field_kv && axis_value_kv_check_integrity(field_kv),
               "Should not happen.");

    axis_string_t *field_key = &field_kv->key;
    axis_value_t *field_value = field_kv->value;

    axis_schema_object_property_t *property = axis_schema_object_property_create(
        axis_string_get_raw_str(field_key), field_value);
    if (!property) {
      axis_ASSERT(property, "Invalid schema property at %s.",
                 axis_string_get_raw_str(field_key));
      return NULL;
    }

    axis_schema_keyword_properties_append_item(keyword_properties, property);
  }

  return &keyword_properties->hdr;
}
