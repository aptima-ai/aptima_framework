//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include <stdbool.h>

#include "aptima_utils/lib/json.h"
#include "aptima_utils/lib/signature.h"
#include "aptima_utils/lib/string.h"

typedef struct aptima_value_t aptima_value_t;

typedef struct aptima_value_kv_t {
  aptima_signature_t signature;

  aptima_string_t key;
  aptima_value_t *value;
} aptima_value_kv_t;

aptima_UTILS_API bool aptima_value_kv_check_integrity(aptima_value_kv_t *self);

aptima_UTILS_API aptima_value_kv_t *aptima_value_kv_create_empty(const char *name);

aptima_UTILS_API aptima_value_kv_t *aptima_value_kv_create(const char *name,
                                                  aptima_value_t *value);

aptima_UTILS_API void aptima_value_kv_destroy(aptima_value_kv_t *self);

// This special destroy function for the value will only deinitialize the key
// field and will not handle the value field.
aptima_UTILS_API void aptima_value_kv_destroy_key_only(aptima_value_kv_t *self);

aptima_UTILS_API aptima_value_kv_t *aptima_value_kv_clone(aptima_value_kv_t *target);

aptima_UTILS_API aptima_string_t *aptima_value_kv_get_key(aptima_value_kv_t *self);

aptima_UTILS_API aptima_value_t *aptima_value_kv_get_value(aptima_value_kv_t *self);

aptima_UTILS_API void aptima_value_kv_reset_to_value(aptima_value_kv_t *self,
                                               aptima_value_t *value);

aptima_UTILS_API aptima_string_t *aptima_value_kv_to_string(aptima_value_kv_t *self,
                                                   aptima_error_t *err);

aptima_UTILS_API aptima_value_kv_t *aptima_value_kv_from_json(const char *key,
                                                     aptima_json_t *json);

aptima_UTILS_API void aptima_value_kv_to_json(aptima_json_t *json, aptima_value_kv_t *kv);
