//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include "stdint.h"

typedef struct axis_waitable_object_t axis_waitable_number_t;

axis_UTILS_API axis_waitable_number_t *axis_waitable_number_create(
    int64_t init_value);

axis_UTILS_API void axis_waitable_number_destroy(axis_waitable_number_t *number);

axis_UTILS_API void axis_waitable_number_increase(axis_waitable_number_t *number,
                                                int64_t value);

axis_UTILS_API void axis_waitable_number_decrease(axis_waitable_number_t *number,
                                                int64_t value);

axis_UTILS_API void axis_waitable_number_multiply(axis_waitable_number_t *number,
                                                int64_t value);

axis_UTILS_API void axis_waitable_number_divide(axis_waitable_number_t *number,
                                              int64_t value);

axis_UTILS_API void axis_waitable_number_set(axis_waitable_number_t *number,
                                           int64_t value);

axis_UTILS_API int64_t axis_waitable_number_get(axis_waitable_number_t *number);

axis_UTILS_API int axis_waitable_number_wait_until(axis_waitable_number_t *number,
                                                 int64_t value, int timeout);

axis_UTILS_API int axis_waitable_number_wait_while(axis_waitable_number_t *number,
                                                 int64_t value, int timeout);
