//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include <stdbool.h>
#include <stdint.h>

#include "aptima_utils/container/list.h"
#include "aptima_utils/lib/buf.h"
#include "aptima_utils/lib/error.h"
#include "aptima_utils/lib/string.h"
#include "aptima_utils/value/type.h"
#include "aptima_utils/value/value_kv.h"

aptima_UTILS_API aptima_TYPE aptima_value_get_type(aptima_value_t *self);

aptima_UTILS_API int8_t aptima_value_get_int8(aptima_value_t *self, aptima_error_t *err);

aptima_UTILS_API int16_t aptima_value_get_int16(aptima_value_t *self, aptima_error_t *err);

aptima_UTILS_API int32_t aptima_value_get_int32(aptima_value_t *self, aptima_error_t *err);

aptima_UTILS_API int64_t aptima_value_get_int64(aptima_value_t *self, aptima_error_t *err);

aptima_UTILS_API uint8_t aptima_value_get_uint8(aptima_value_t *self, aptima_error_t *err);

aptima_UTILS_API uint16_t aptima_value_get_uint16(aptima_value_t *self,
                                            aptima_error_t *err);

aptima_UTILS_API uint32_t aptima_value_get_uint32(aptima_value_t *self,
                                            aptima_error_t *err);

aptima_UTILS_API uint64_t aptima_value_get_uint64(aptima_value_t *self,
                                            aptima_error_t *err);

aptima_UTILS_API float aptima_value_get_float32(aptima_value_t *self, aptima_error_t *err);

aptima_UTILS_API double aptima_value_get_float64(aptima_value_t *self, aptima_error_t *err);

aptima_UTILS_API bool aptima_value_get_bool(aptima_value_t *self, aptima_error_t *err);

aptima_UTILS_API aptima_string_t *aptima_value_peek_string(aptima_value_t *self);

aptima_UTILS_API const char *aptima_value_peek_raw_str(aptima_value_t *self,
                                                 aptima_error_t *err);

aptima_UTILS_API void *aptima_value_get_ptr(aptima_value_t *self, aptima_error_t *err);

aptima_UTILS_API aptima_buf_t *aptima_value_peek_buf(aptima_value_t *self);

aptima_UTILS_API aptima_list_t *aptima_value_peek_array(aptima_value_t *self);

aptima_UTILS_API aptima_list_t *aptima_value_peek_object(aptima_value_t *self);

aptima_UTILS_API aptima_value_t *aptima_value_array_peek(aptima_value_t *self, size_t index,
                                                aptima_error_t *err);
