//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include "include_internal/axis_utils/schema/keywords/keyword.h"
#include "axis_utils/value/type.h"

#define axis_SCHEMA_KEYWORD_TYPE_SIGNATURE 0xC24816B665EF018FU

typedef struct axis_schema_t axis_schema_t;

typedef struct axis_schema_keyword_type_t {
  axis_schema_keyword_t hdr;
  axis_signature_t signature;
  axis_TYPE type;
} axis_schema_keyword_type_t;

axis_UTILS_PRIVATE_API bool axis_schema_keyword_type_check_integrity(
    axis_schema_keyword_type_t *self);

axis_UTILS_PRIVATE_API axis_schema_keyword_t *
axis_schema_keyword_type_create_from_value(axis_schema_t *owner,
                                          axis_value_t *value);
