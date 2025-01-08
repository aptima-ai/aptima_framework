//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stdbool.h>

#include "axis_utils/lib/json.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/string.h"

typedef struct axis_value_t axis_value_t;

typedef struct axis_value_kv_t {
  axis_signature_t signature;

  axis_string_t key;
  axis_value_t *value;
} axis_value_kv_t;

axis_UTILS_API bool axis_value_kv_check_integrity(axis_value_kv_t *self);

axis_UTILS_API axis_value_kv_t *axis_value_kv_create_empty(const char *name);

axis_UTILS_API axis_value_kv_t *axis_value_kv_create(const char *name,
                                                  axis_value_t *value);

axis_UTILS_API void axis_value_kv_destroy(axis_value_kv_t *self);

// This special destroy function for the value will only deinitialize the key
// field and will not handle the value field.
axis_UTILS_API void axis_value_kv_destroy_key_only(axis_value_kv_t *self);

axis_UTILS_API axis_value_kv_t *axis_value_kv_clone(axis_value_kv_t *target);

axis_UTILS_API axis_string_t *axis_value_kv_get_key(axis_value_kv_t *self);

axis_UTILS_API axis_value_t *axis_value_kv_get_value(axis_value_kv_t *self);

axis_UTILS_API void axis_value_kv_reset_to_value(axis_value_kv_t *self,
                                               axis_value_t *value);

axis_UTILS_API axis_string_t *axis_value_kv_to_string(axis_value_kv_t *self,
                                                   axis_error_t *err);

axis_UTILS_API axis_value_kv_t *axis_value_kv_from_json(const char *key,
                                                     axis_json_t *json);

axis_UTILS_API void axis_value_kv_to_json(axis_json_t *json, axis_value_kv_t *kv);
