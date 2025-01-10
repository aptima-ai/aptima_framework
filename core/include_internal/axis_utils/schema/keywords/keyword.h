//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include "axis_utils/container/hash_handle.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/value/value.h"

#define axis_SCHEMA_KEYWORD_SIGNATURE 0x94F75C7B1835B931U

typedef struct axis_schema_t axis_schema_t;
typedef struct axis_schema_keyword_t axis_schema_keyword_t;
typedef struct axis_schema_error_t axis_schema_error_t;

typedef void (*axis_schema_keyword_destroy_func_t)(axis_schema_keyword_t *self);

typedef bool (*axis_schema_keyword_validate_value_func_t)(
    axis_schema_keyword_t *self, axis_value_t *value,
    axis_schema_error_t *schema_err);

typedef bool (*axis_schema_keyword_adjust_value_func_t)(
    axis_schema_keyword_t *self, axis_value_t *value,
    axis_schema_error_t *schema_err);

/**
 * @brief Check if the keyword is compatible with the target keyword. Note that
 * the `self` or `target` might be NULL (i.e., the keyword is not defined in the
 * schema), all implementations should handle this case properly.
 */
typedef bool (*axis_schema_keyword_is_compatible_func_t)(
    axis_schema_keyword_t *self, axis_schema_keyword_t *target,
    axis_schema_error_t *schema_err);

typedef enum axis_SCHEMA_KEYWORD {
  axis_SCHEMA_KEYWORD_INVALID = 0,

  axis_SCHEMA_KEYWORD_TYPE,
  axis_SCHEMA_KEYWORD_PROPERTIES,
  axis_SCHEMA_KEYWORD_ITEMS,
  axis_SCHEMA_KEYWORD_REQUIRED,

  // It's just used to determine the range of keyword type.
  axis_SCHEMA_KEYWORD_LAST,
} axis_SCHEMA_KEYWORD;

typedef struct axis_schema_keyword_t {
  axis_signature_t signature;

  axis_SCHEMA_KEYWORD type;
  axis_hashhandle_t hh_in_keyword_map;

  axis_schema_t *owner;
  axis_schema_keyword_destroy_func_t destroy;
  axis_schema_keyword_validate_value_func_t validate_value;
  axis_schema_keyword_adjust_value_func_t adjust_value;
  axis_schema_keyword_is_compatible_func_t is_compatible;
} axis_schema_keyword_t;

axis_UTILS_PRIVATE_API bool axis_schema_keyword_check_integrity(
    axis_schema_keyword_t *self);

axis_UTILS_PRIVATE_API void axis_schema_keyword_init(axis_schema_keyword_t *self,
                                                   axis_SCHEMA_KEYWORD type);

axis_UTILS_PRIVATE_API void axis_schema_keyword_deinit(
    axis_schema_keyword_t *self);
