//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include "include_internal/axis_utils/schema/schema.h"
#include "axis_utils/lib/signature.h"

#define axis_SCHEMA_OBJECT_SIGNATURE 0xA4E7414BCE9C9C3AU

typedef struct axis_schema_keyword_properties_t axis_schema_keyword_properties_t;
typedef struct axis_schema_keyword_required_t axis_schema_keyword_required_t;

typedef struct axis_schema_object_t {
  axis_schema_t hdr;
  axis_signature_t signature;
  axis_schema_keyword_properties_t *keyword_properties;
  axis_schema_keyword_required_t *keyword_required;
} axis_schema_object_t;

axis_UTILS_PRIVATE_API bool axis_schema_object_check_integrity(
    axis_schema_object_t *self);

axis_UTILS_PRIVATE_API axis_schema_object_t *axis_schema_object_create(void);

axis_UTILS_PRIVATE_API void axis_schema_object_destroy(axis_schema_object_t *self);

axis_UTILS_API axis_schema_t *axis_schema_object_peek_property_schema(
    axis_schema_t *self, const char *prop_name);
