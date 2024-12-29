//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_utils/ten_config.h"

#include "include_internal/ten_utils/schema/schema.h"
#include "ten_utils/lib/signature.h"

#define TEN_SCHEMA_ARRAY_SIGNATURE 0xAE6AB8E5EC904110U

typedef struct ten_schema_keyword_items_t ten_schema_keyword_items_t;

typedef struct ten_schema_array_t {
  ten_schema_t hdr;
  ten_signature_t signature;
  ten_schema_keyword_items_t *keyword_items;
} ten_schema_array_t;

TEN_UTILS_PRIVATE_API bool ten_schema_array_check_integrity(
    ten_schema_array_t *self);

TEN_UTILS_PRIVATE_API ten_schema_array_t *ten_schema_array_create(void);

TEN_UTILS_PRIVATE_API void ten_schema_array_destroy(ten_schema_array_t *self);
