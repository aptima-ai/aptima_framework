//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

typedef struct aptima_waitable_object_t aptima_waitable_object_t;

aptima_UTILS_API aptima_waitable_object_t *aptima_waitable_object_create(
    void *init_value);

aptima_UTILS_API void aptima_waitable_object_destroy(aptima_waitable_object_t *obj);

aptima_UTILS_API void aptima_waitable_object_set(aptima_waitable_object_t *obj,
                                           void *value);

aptima_UTILS_API void *aptima_waitable_object_get(aptima_waitable_object_t *obj);

aptima_UTILS_API void aptima_waitable_object_update(aptima_waitable_object_t *obj);

aptima_UTILS_API int aptima_waitable_object_wait_until(aptima_waitable_object_t *obj,
                                                 int (*compare)(const void *l,
                                                                const void *r),
                                                 int timeout);

aptima_UTILS_API int aptima_waitable_object_wait_while(aptima_waitable_object_t *obj,
                                                 int (*compare)(const void *l,
                                                                const void *r),
                                                 int timeout);
