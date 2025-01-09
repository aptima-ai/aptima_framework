//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include <stdbool.h>

#include "aptima_utils/value/value_kv.h"

aptima_UTILS_API bool aptima_value_is_object(aptima_value_t *self);

aptima_UTILS_API bool aptima_value_is_array(aptima_value_t *self);

aptima_UTILS_API bool aptima_value_is_string(aptima_value_t *self);

aptima_UTILS_API bool aptima_value_is_invalid(aptima_value_t *self);

aptima_UTILS_API bool aptima_value_is_int8(aptima_value_t *self);

aptima_UTILS_API bool aptima_value_is_int16(aptima_value_t *self);

aptima_UTILS_API bool aptima_value_is_int32(aptima_value_t *self);

aptima_UTILS_API bool aptima_value_is_int64(aptima_value_t *self);

aptima_UTILS_API bool aptima_value_is_uint8(aptima_value_t *self);

aptima_UTILS_API bool aptima_value_is_uint16(aptima_value_t *self);

aptima_UTILS_API bool aptima_value_is_uint32(aptima_value_t *self);

aptima_UTILS_API bool aptima_value_is_uint64(aptima_value_t *self);

aptima_UTILS_API bool aptima_value_is_float32(aptima_value_t *self);

aptima_UTILS_API bool aptima_value_is_float64(aptima_value_t *self);

aptima_UTILS_API bool aptima_value_is_number(aptima_value_t *self);

aptima_UTILS_API bool aptima_value_is_null(aptima_value_t *self);

aptima_UTILS_API bool aptima_value_is_bool(aptima_value_t *self);

aptima_UTILS_API bool aptima_value_is_ptr(aptima_value_t *self);

aptima_UTILS_API bool aptima_value_is_buf(aptima_value_t *self);
