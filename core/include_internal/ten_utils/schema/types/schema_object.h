//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_utils/ten_config.h"

#include "include_internal/ten_utils/schema/schema.h"
#include "ten_utils/lib/signature.h"

#define TEN_SCHEMA_OBJECT_SIGNATURE 0xA4E7414BCE9C9C3AU

typedef struct ten_schema_keyword_properties_t ten_schema_keyword_properties_t;
typedef struct ten_schema_keyword_required_t ten_schema_keyword_required_t;

typedef struct ten_schema_object_t {
  ten_schema_t hdr;
  ten_signature_t signature;
  ten_schema_keyword_properties_t *keyword_properties;
  ten_schema_keyword_required_t *keyword_required;
} ten_schema_object_t;

TEN_UTILS_PRIVATE_API bool ten_schema_object_check_integrity(
    ten_schema_object_t *self);

TEN_UTILS_PRIVATE_API ten_schema_object_t *ten_schema_object_create(void);

TEN_UTILS_PRIVATE_API void ten_schema_object_destroy(ten_schema_object_t *self);

TEN_UTILS_API ten_schema_t *ten_schema_object_peek_property_schema(
    ten_schema_t *self, const char *prop_name);
