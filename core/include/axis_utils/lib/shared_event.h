//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stdint.h>

#include "axis_utils/lib/atomic.h"
#include "axis_utils/lib/waitable_addr.h"

typedef struct axis_shared_event_t axis_shared_event_t;

axis_UTILS_API axis_shared_event_t *axis_shared_event_create(
    uint32_t *addr_for_sig, axis_atomic_t *addr_for_lock, int init_state,
    int auto_reset);

axis_UTILS_API int axis_shared_event_wait(axis_shared_event_t *event, int wait_ms);

axis_UTILS_API void axis_shared_event_set(axis_shared_event_t *event);

axis_UTILS_API void axis_shared_event_reset(axis_shared_event_t *event);

axis_UTILS_API void axis_shared_event_destroy(axis_shared_event_t *event);
