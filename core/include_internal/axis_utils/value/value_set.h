//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stdbool.h>

#include "axis_utils/value/value.h"

axis_UTILS_API bool axis_value_set_int64(axis_value_t *self, int64_t value);

axis_UTILS_API bool axis_value_set_int32(axis_value_t *self, int32_t value);

axis_UTILS_API bool axis_value_set_int16(axis_value_t *self, int16_t value);

axis_UTILS_API bool axis_value_set_int8(axis_value_t *self, int8_t value);

axis_UTILS_API bool axis_value_set_uint64(axis_value_t *self, uint64_t value);

axis_UTILS_API bool axis_value_set_uint32(axis_value_t *self, uint32_t value);

axis_UTILS_API bool axis_value_set_uint16(axis_value_t *self, uint16_t value);

axis_UTILS_API bool axis_value_set_uint8(axis_value_t *self, uint8_t value);

axis_UTILS_API bool axis_value_set_bool(axis_value_t *self, bool value);

axis_UTILS_API bool axis_value_set_float32(axis_value_t *self, float value);

axis_UTILS_API bool axis_value_set_float64(axis_value_t *self, double value);

axis_UTILS_API bool axis_value_set_string(axis_value_t *self, const char *str);

axis_UTILS_API bool axis_value_set_string_with_size(axis_value_t *self,
                                                  const char *str, size_t len);

axis_UTILS_API bool axis_value_set_array_with_move(axis_value_t *self,
                                                 axis_list_t *value);

axis_UTILS_API bool axis_value_set_object_with_move(axis_value_t *self,
                                                  axis_list_t *value);
