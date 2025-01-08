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

#define axis_SCHEMA_ARRAY_SIGNATURE 0xAE6AB8E5EC904110U

typedef struct axis_schema_keyword_items_t axis_schema_keyword_items_t;

typedef struct axis_schema_array_t {
  axis_schema_t hdr;
  axis_signature_t signature;
  axis_schema_keyword_items_t *keyword_items;
} axis_schema_array_t;

axis_UTILS_PRIVATE_API bool axis_schema_array_check_integrity(
    axis_schema_array_t *self);

axis_UTILS_PRIVATE_API axis_schema_array_t *axis_schema_array_create(void);

axis_UTILS_PRIVATE_API void axis_schema_array_destroy(axis_schema_array_t *self);
