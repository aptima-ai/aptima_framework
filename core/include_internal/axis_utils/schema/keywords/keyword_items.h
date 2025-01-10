//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include "include_internal/axis_utils/schema/keywords/keyword.h"
#include "axis_utils/lib/signature.h"

#define axis_SCHEMA_KEYWORD_ITEMS_SIGNATURE 0x8AD7BCDE1BA0ADE7U

typedef struct axis_schema_keyword_items_t {
  axis_schema_keyword_t hdr;
  axis_signature_t signature;

  // The type of the items must be same.
  axis_schema_t *item_schema;
} axis_schema_keyword_items_t;

axis_UTILS_PRIVATE_API bool axis_schema_keyword_items_check_integrity(
    axis_schema_keyword_items_t *self);

axis_UTILS_PRIVATE_API axis_schema_keyword_t *
axis_schema_keyword_items_create_from_value(axis_schema_t *owner,
                                           axis_value_t *value);
