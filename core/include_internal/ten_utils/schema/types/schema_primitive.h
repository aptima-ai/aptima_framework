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

#define TEN_SCHEMA_PRIMITIVE_SIGNATURE 0x368E9692BBD7548DU

// The definition of the schema of a primitive type (i.e.,
// int/uint/float/double/string/bool).
//
// Note that we only support `type` keyword now, so the type `buf` and `ptr` are
// treated as primitive.
typedef struct ten_schema_primitive_t {
  ten_schema_t hdr;
  ten_signature_t signature;
} ten_schema_primitive_t;

TEN_UTILS_PRIVATE_API bool ten_schema_primitive_check_integrity(
    ten_schema_primitive_t *self);

TEN_UTILS_PRIVATE_API ten_schema_primitive_t *ten_schema_primitive_create(void);

TEN_UTILS_PRIVATE_API void ten_schema_primitive_destroy(
    ten_schema_primitive_t *self);
