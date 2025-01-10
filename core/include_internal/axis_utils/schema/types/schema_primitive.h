//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include "include_internal/axis_utils/schema/schema.h"
#include "axis_utils/lib/signature.h"

#define axis_SCHEMA_PRIMITIVE_SIGNATURE 0x368E9692BBD7548DU

// The definition of the schema of a primitive type (i.e.,
// int/uint/float/double/string/bool).
//
// Note that we only support `type` keyword now, so the type `buf` and `ptr` are
// treated as primitive.
typedef struct axis_schema_primitive_t {
  axis_schema_t hdr;
  axis_signature_t signature;
} axis_schema_primitive_t;

axis_UTILS_PRIVATE_API bool axis_schema_primitive_check_integrity(
    axis_schema_primitive_t *self);

axis_UTILS_PRIVATE_API axis_schema_primitive_t *axis_schema_primitive_create(void);

axis_UTILS_PRIVATE_API void axis_schema_primitive_destroy(
    axis_schema_primitive_t *self);
