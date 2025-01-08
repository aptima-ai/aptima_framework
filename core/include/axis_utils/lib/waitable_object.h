//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

typedef struct axis_waitable_object_t axis_waitable_object_t;

axis_UTILS_API axis_waitable_object_t *axis_waitable_object_create(
    void *init_value);

axis_UTILS_API void axis_waitable_object_destroy(axis_waitable_object_t *obj);

axis_UTILS_API void axis_waitable_object_set(axis_waitable_object_t *obj,
                                           void *value);

axis_UTILS_API void *axis_waitable_object_get(axis_waitable_object_t *obj);

axis_UTILS_API void axis_waitable_object_update(axis_waitable_object_t *obj);

axis_UTILS_API int axis_waitable_object_wait_until(axis_waitable_object_t *obj,
                                                 int (*compare)(const void *l,
                                                                const void *r),
                                                 int timeout);

axis_UTILS_API int axis_waitable_object_wait_while(axis_waitable_object_t *obj,
                                                 int (*compare)(const void *l,
                                                                const void *r),
                                                 int timeout);
