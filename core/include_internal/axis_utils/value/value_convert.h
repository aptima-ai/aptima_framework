//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stdbool.h>

typedef struct axis_value_t axis_value_t;
typedef struct axis_error_t axis_error_t;

/**
 * @brief Convert the value to int8 if the value does not overflow, even though
 * the range of the value type is larger than int8. Note that only the integer
 * type (i.e., uint8/16/32/64, int8/16/32/64) can be converted to int8, the
 * accuracy of the value can not be lost.
 */
axis_UTILS_API bool axis_value_convert_to_int8(axis_value_t *self,
                                             axis_error_t *err);

axis_UTILS_API bool axis_value_convert_to_int16(axis_value_t *self,
                                              axis_error_t *err);

axis_UTILS_API bool axis_value_convert_to_int32(axis_value_t *self,
                                              axis_error_t *err);

axis_UTILS_API bool axis_value_convert_to_int64(axis_value_t *self,
                                              axis_error_t *err);

axis_UTILS_API bool axis_value_convert_to_uint8(axis_value_t *self,
                                              axis_error_t *err);

axis_UTILS_API bool axis_value_convert_to_uint16(axis_value_t *self,
                                               axis_error_t *err);

axis_UTILS_API bool axis_value_convert_to_uint32(axis_value_t *self,
                                               axis_error_t *err);

axis_UTILS_API bool axis_value_convert_to_uint64(axis_value_t *self,
                                               axis_error_t *err);

axis_UTILS_API bool axis_value_convert_to_float32(axis_value_t *self,
                                                axis_error_t *err);

axis_UTILS_API bool axis_value_convert_to_float64(axis_value_t *self,
                                                axis_error_t *err);
