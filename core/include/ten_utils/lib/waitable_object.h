//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_utils/ten_config.h"

typedef struct ten_waitable_object_t ten_waitable_object_t;

TEN_UTILS_API ten_waitable_object_t *ten_waitable_object_create(
    void *init_value);

TEN_UTILS_API void ten_waitable_object_destroy(ten_waitable_object_t *obj);

TEN_UTILS_API void ten_waitable_object_set(ten_waitable_object_t *obj,
                                           void *value);

TEN_UTILS_API void *ten_waitable_object_get(ten_waitable_object_t *obj);

TEN_UTILS_API void ten_waitable_object_update(ten_waitable_object_t *obj);

TEN_UTILS_API int ten_waitable_object_wait_until(ten_waitable_object_t *obj,
                                                 int (*compare)(const void *l,
                                                                const void *r),
                                                 int timeout);

TEN_UTILS_API int ten_waitable_object_wait_while(ten_waitable_object_t *obj,
                                                 int (*compare)(const void *l,
                                                                const void *r),
                                                 int timeout);
