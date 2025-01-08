//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stdbool.h>

#include "axis_utils/value/value_kv.h"

axis_UTILS_API bool axis_value_is_object(axis_value_t *self);

axis_UTILS_API bool axis_value_is_array(axis_value_t *self);

axis_UTILS_API bool axis_value_is_string(axis_value_t *self);

axis_UTILS_API bool axis_value_is_invalid(axis_value_t *self);

axis_UTILS_API bool axis_value_is_int8(axis_value_t *self);

axis_UTILS_API bool axis_value_is_int16(axis_value_t *self);

axis_UTILS_API bool axis_value_is_int32(axis_value_t *self);

axis_UTILS_API bool axis_value_is_int64(axis_value_t *self);

axis_UTILS_API bool axis_value_is_uint8(axis_value_t *self);

axis_UTILS_API bool axis_value_is_uint16(axis_value_t *self);

axis_UTILS_API bool axis_value_is_uint32(axis_value_t *self);

axis_UTILS_API bool axis_value_is_uint64(axis_value_t *self);

axis_UTILS_API bool axis_value_is_float32(axis_value_t *self);

axis_UTILS_API bool axis_value_is_float64(axis_value_t *self);

axis_UTILS_API bool axis_value_is_number(axis_value_t *self);

axis_UTILS_API bool axis_value_is_null(axis_value_t *self);

axis_UTILS_API bool axis_value_is_bool(axis_value_t *self);

axis_UTILS_API bool axis_value_is_ptr(axis_value_t *self);

axis_UTILS_API bool axis_value_is_buf(axis_value_t *self);
