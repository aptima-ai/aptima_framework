//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/schema_store/property.h"

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_utils/schema/constant_str.h"
#include "include_internal/axis_utils/schema/keywords/keyword.h"
#include "include_internal/axis_utils/schema/schema.h"
#include "include_internal/axis_utils/value/constant_str.h"
#include "axis_utils/container/list.h"
#include "axis_utils/container/list_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/memory.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_is.h"
#include "axis_utils/value/value_kv.h"
#include "axis_utils/value/value_merge.h"
#include "axis_utils/value/value_object.h"

static void axis_schema_object_kv_destroy_key_only(axis_value_kv_t *self) {
  axis_ASSERT(self && axis_value_kv_check_integrity(self), "Invalid argument.");

  axis_string_deinit(&self->key);

  // Value is not owned by the key-value pair, do not destroy it.
  self->value = NULL;
  axis_FREE(self);
}

axis_schema_t *axis_schemas_parse_schema_object_for_property(
    axis_value_t *schemas_content) {
  axis_ASSERT(schemas_content && axis_value_check_integrity(schemas_content),
             "Invalid argument.");

  if (!axis_value_is_object(schemas_content)) {
    axis_ASSERT(0, "The schema should be an object contains `property`.");
    return NULL;
  }

  axis_value_t *property_schema_content =
      axis_value_object_peek(schemas_content, axis_STR_PROPERTY);
  if (!property_schema_content) {
    return NULL;
  }

  // The structure of the `schemas_content` is not a standard `object` schema,
  // create a new object schema for the property schema.
  axis_list_t object_schema_fields = axis_LIST_INIT_VAL;

  axis_list_push_ptr_back(
      &object_schema_fields,
      axis_value_kv_create(axis_SCHEMA_KEYWORD_STR_TYPE,
                          axis_value_create_string(axis_STR_OBJECT)),
      (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);

  axis_list_push_ptr_back(
      &object_schema_fields,
      axis_value_kv_create(axis_SCHEMA_KEYWORD_STR_PROPERTIES,
                          property_schema_content),
      (axis_ptr_listnode_destroy_func_t)axis_schema_object_kv_destroy_key_only);

  axis_value_t *required_schema_content =
      axis_value_object_peek(schemas_content, axis_SCHEMA_KEYWORD_STR_REQUIRED);
  if (required_schema_content) {
    axis_list_push_ptr_back(
        &object_schema_fields,
        axis_value_kv_create(axis_SCHEMA_KEYWORD_STR_REQUIRED,
                            required_schema_content),
        (axis_ptr_listnode_destroy_func_t)axis_schema_object_kv_destroy_key_only);
  }

  axis_value_t *object_schema_content =
      axis_value_create_object_with_move(&object_schema_fields);
  axis_list_clear(&object_schema_fields);

  axis_schema_t *schema = axis_schema_create_from_value(object_schema_content);
  axis_ASSERT(schema, "Should not happen.");

  axis_value_destroy(object_schema_content);

  return schema;
}
