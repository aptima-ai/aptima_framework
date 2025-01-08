//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stdbool.h>

#include "axis_utils/value/value.h"  // IWYU pragma: keep
#include "axis_utils/value/value_kv.h"

#define axis_VALUE_SIGNATURE 0x1F30F97F37E6BC42U

axis_UTILS_PRIVATE_API bool axis_value_is_equal(axis_value_t *self,
                                              axis_value_t *target);

axis_UTILS_API axis_value_t *axis_value_create_vstring(const char *fmt, ...);

axis_UTILS_API axis_value_t *axis_value_create_vastring(const char *fmt,
                                                     va_list ap);

axis_UTILS_PRIVATE_API void axis_value_reset_to_null(axis_value_t *self);
