//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include <stdint.h>

typedef struct aptima_spinlock_t aptima_spinlock_t;
typedef struct aptima_waitable_t {
  uint32_t sig;
} aptima_waitable_t;

#define aptima_WAITABLE_INIT {0}

aptima_UTILS_API void aptima_waitable_init(aptima_waitable_t *wb);

aptima_UTILS_API aptima_waitable_t *aptima_waitable_from_addr(uint32_t *address);

aptima_UTILS_API int aptima_waitable_wait(aptima_waitable_t *wb, uint32_t expect,
                                    aptima_spinlock_t *lock, int timeout);

aptima_UTILS_API void aptima_waitable_notify(aptima_waitable_t *wb);

aptima_UTILS_API void aptima_waitable_notify_all(aptima_waitable_t *wb);

aptima_UTILS_API uint32_t aptima_waitable_get(aptima_waitable_t *wb);

aptima_UTILS_API void aptima_waitable_set(aptima_waitable_t *wb, uint32_t val);
