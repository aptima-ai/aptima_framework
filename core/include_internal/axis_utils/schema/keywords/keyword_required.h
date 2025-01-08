//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"
#include "axis_utils/axis_config.h"

#include "include_internal/axis_utils/schema/keywords/keyword.h"
#include "axis_utils/lib/signature.h"

#define axis_SCHEMA_KEYWORD_REQUIRED_SIGNATURE 0x5955F1F62BCEFD92U

typedef struct axis_schema_keyword_required_t {
  axis_schema_keyword_t hdr;
  axis_signature_t signature;

  axis_list_t required_properties;
} axis_schema_keyword_required_t;

axis_UTILS_PRIVATE_API bool axis_schema_keyword_required_check_integrity(
    axis_schema_keyword_required_t *self);

axis_UTILS_PRIVATE_API axis_schema_keyword_t *
axis_schema_keyword_required_create_from_value(axis_schema_t *owner,
                                              axis_value_t *value);
