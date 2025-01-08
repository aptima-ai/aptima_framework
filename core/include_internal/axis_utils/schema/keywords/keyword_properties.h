//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include "include_internal/axis_utils/schema/keywords/keyword.h"
#include "include_internal/axis_utils/schema/schema.h"
#include "axis_utils/container/hash_handle.h"
#include "axis_utils/container/hash_table.h"

#define axis_SCHEMA_OBJECT_PROPERTY_SIGNATURE 0x48C4F08A95436D91U
#define axis_SCHEMA_KEYWORD_PROPERTIES_SIGNATURE 0xB115E3DEF5E41A12U

typedef struct axis_schema_object_property_t {
  axis_signature_t signature;
  axis_hashhandle_t hh_in_properties_table;
  axis_string_t name;  // The name of the property in the object.
  axis_schema_t *schema;
} axis_schema_object_property_t;

typedef struct axis_schema_keyword_properties_t {
  axis_schema_keyword_t hdr;
  axis_signature_t signature;

  // The properties of the object. The type of the value is
  // axis_schema_object_property_t.
  axis_hashtable_t properties;
} axis_schema_keyword_properties_t;

axis_UTILS_PRIVATE_API bool axis_schema_keyword_properties_check_integrity(
    axis_schema_keyword_properties_t *self);

axis_UTILS_PRIVATE_API axis_schema_keyword_t *
axis_schema_keyword_properties_create_from_value(axis_schema_t *owner,
                                                axis_value_t *value);

axis_UTILS_PRIVATE_API axis_schema_t *
axis_schema_keyword_properties_peek_property_schema(
    axis_schema_keyword_properties_t *self, const char *prop_name);
