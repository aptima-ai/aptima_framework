//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include "include_internal/axis_runtime/extension_group/axis_env/metadata.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/value/value.h"

#define axis_PLACEHOLDER_SIGNATURE 0xE7AF02ECD77D2DCCU

typedef enum axis_PLACEHOLDER_SCOPE {
  axis_PLACEHOLDER_SCOPE_INVALID,

  axis_PLACEHOLDER_SCOPE_ENV
} axis_PLACEHOLDER_SCOPE;

/**
 * The format is as follows:
 *
 * ${scope:variable|default_value}
 *
 * Possible options for scope:
 * - env
 *
 * The '|default_value' part is optional.
 */
typedef struct axis_placeholder_t {
  axis_signature_t signature;

  axis_PLACEHOLDER_SCOPE scope;
  axis_string_t variable;
  axis_value_t default_value;
} axis_placeholder_t;

axis_UTILS_API bool axis_c_str_is_placeholder(const char *input);

axis_UTILS_API axis_placeholder_t *axis_placeholder_create(void);

axis_UTILS_API void axis_placeholder_destroy(axis_placeholder_t *self);

axis_UTILS_API void axis_placeholder_init(axis_placeholder_t *self);

axis_UTILS_API void axis_placeholder_deinit(axis_placeholder_t *self);

axis_UTILS_API bool axis_placeholder_parse(axis_placeholder_t *self,
                                         const char *input, axis_error_t *err);

axis_UTILS_API bool axis_placeholder_resolve(axis_placeholder_t *self,
                                           axis_value_t *placeholder_value,
                                           axis_error_t *err);
