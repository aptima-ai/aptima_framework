//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include "stdint.h"

typedef struct aptima_waitable_object_t aptima_waitable_number_t;

aptima_UTILS_API aptima_waitable_number_t *aptima_waitable_number_create(
    int64_t init_value);

aptima_UTILS_API void aptima_waitable_number_destroy(aptima_waitable_number_t *number);

aptima_UTILS_API void aptima_waitable_number_increase(aptima_waitable_number_t *number,
                                                int64_t value);

aptima_UTILS_API void aptima_waitable_number_decrease(aptima_waitable_number_t *number,
                                                int64_t value);

aptima_UTILS_API void aptima_waitable_number_multiply(aptima_waitable_number_t *number,
                                                int64_t value);

aptima_UTILS_API void aptima_waitable_number_divide(aptima_waitable_number_t *number,
                                              int64_t value);

aptima_UTILS_API void aptima_waitable_number_set(aptima_waitable_number_t *number,
                                           int64_t value);

aptima_UTILS_API int64_t aptima_waitable_number_get(aptima_waitable_number_t *number);

aptima_UTILS_API int aptima_waitable_number_wait_until(aptima_waitable_number_t *number,
                                                 int64_t value, int timeout);

aptima_UTILS_API int aptima_waitable_number_wait_while(aptima_waitable_number_t *number,
                                                 int64_t value, int timeout);
