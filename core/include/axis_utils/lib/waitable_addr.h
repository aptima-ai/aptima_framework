//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stdint.h>

typedef struct axis_spinlock_t axis_spinlock_t;
typedef struct axis_waitable_t {
  uint32_t sig;
} axis_waitable_t;

#define axis_WAITABLE_INIT {0}

axis_UTILS_API void axis_waitable_init(axis_waitable_t *wb);

axis_UTILS_API axis_waitable_t *axis_waitable_from_addr(uint32_t *address);

axis_UTILS_API int axis_waitable_wait(axis_waitable_t *wb, uint32_t expect,
                                    axis_spinlock_t *lock, int timeout);

axis_UTILS_API void axis_waitable_notify(axis_waitable_t *wb);

axis_UTILS_API void axis_waitable_notify_all(axis_waitable_t *wb);

axis_UTILS_API uint32_t axis_waitable_get(axis_waitable_t *wb);

axis_UTILS_API void axis_waitable_set(axis_waitable_t *wb, uint32_t val);
