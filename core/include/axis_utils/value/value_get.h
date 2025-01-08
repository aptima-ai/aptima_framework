//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stdbool.h>
#include <stdint.h>

#include "axis_utils/container/list.h"
#include "axis_utils/lib/buf.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/value/type.h"
#include "axis_utils/value/value_kv.h"

axis_UTILS_API axis_TYPE axis_value_get_type(axis_value_t *self);

axis_UTILS_API int8_t axis_value_get_int8(axis_value_t *self, axis_error_t *err);

axis_UTILS_API int16_t axis_value_get_int16(axis_value_t *self, axis_error_t *err);

axis_UTILS_API int32_t axis_value_get_int32(axis_value_t *self, axis_error_t *err);

axis_UTILS_API int64_t axis_value_get_int64(axis_value_t *self, axis_error_t *err);

axis_UTILS_API uint8_t axis_value_get_uint8(axis_value_t *self, axis_error_t *err);

axis_UTILS_API uint16_t axis_value_get_uint16(axis_value_t *self,
                                            axis_error_t *err);

axis_UTILS_API uint32_t axis_value_get_uint32(axis_value_t *self,
                                            axis_error_t *err);

axis_UTILS_API uint64_t axis_value_get_uint64(axis_value_t *self,
                                            axis_error_t *err);

axis_UTILS_API float axis_value_get_float32(axis_value_t *self, axis_error_t *err);

axis_UTILS_API double axis_value_get_float64(axis_value_t *self, axis_error_t *err);

axis_UTILS_API bool axis_value_get_bool(axis_value_t *self, axis_error_t *err);

axis_UTILS_API axis_string_t *axis_value_peek_string(axis_value_t *self);

axis_UTILS_API const char *axis_value_peek_raw_str(axis_value_t *self,
                                                 axis_error_t *err);

axis_UTILS_API void *axis_value_get_ptr(axis_value_t *self, axis_error_t *err);

axis_UTILS_API axis_buf_t *axis_value_peek_buf(axis_value_t *self);

axis_UTILS_API axis_list_t *axis_value_peek_array(axis_value_t *self);

axis_UTILS_API axis_list_t *axis_value_peek_object(axis_value_t *self);

axis_UTILS_API axis_value_t *axis_value_array_peek(axis_value_t *self, size_t index,
                                                axis_error_t *err);
