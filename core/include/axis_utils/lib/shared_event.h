//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include <stdint.h>

#include "aptima_utils/lib/atomic.h"
#include "aptima_utils/lib/waitable_addr.h"

typedef struct aptima_shared_event_t aptima_shared_event_t;

aptima_UTILS_API aptima_shared_event_t *aptima_shared_event_create(
    uint32_t *addr_for_sig, aptima_atomic_t *addr_for_lock, int init_state,
    int auto_reset);

aptima_UTILS_API int aptima_shared_event_wait(aptima_shared_event_t *event, int wait_ms);

aptima_UTILS_API void aptima_shared_event_set(aptima_shared_event_t *event);

aptima_UTILS_API void aptima_shared_event_reset(aptima_shared_event_t *event);

aptima_UTILS_API void aptima_shared_event_destroy(aptima_shared_event_t *event);
