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
#include "aptima_utils/lib/event.h"

typedef enum REFLOCK_FLAG {
  aptima_REFLOCK_REF = (int64_t)0x00000001,
  aptima_REFLOCK_DESTROY = (int64_t)0x10000000,
  aptima_REFLOCK_POISON = (int64_t)0x300dead0,
  aptima_REFLOCK_DESTROY_MASK = (int64_t)0xf0000000,
  aptima_REFLOCK_REF_MASK = (int64_t)0x0fffffff,
} REFLOCK_FLAG;

typedef struct aptima_reflock_t {
  aptima_atomic_t state;
  aptima_event_t *event;
} aptima_reflock_t;

/**
 * @brief Initialize a reflock.
 * @param reflock The reflock to initialize.
 */
aptima_UTILS_API void aptima_reflock_init(aptima_reflock_t *lock);

/**
 * @brief Increase the reference count of a reflock.
 * @param reflock The reflock to increase the reference count.
 */
aptima_UTILS_API void aptima_reflock_ref(aptima_reflock_t *lock);

/**
 * @brief Decrease the reference count of a reflock.
 * @param reflock The reflock to decrease the reference count.
 */
aptima_UTILS_API void aptima_reflock_unref(aptima_reflock_t *lock);

/**
 * @brief Decrease reference count and destroy after it's zero.
 * @param reflock The reflock to decrease the reference count.
 * @note This function will wait until the reflock is zero
 */
aptima_UTILS_API void aptima_reflock_unref_destroy(aptima_reflock_t *lock);
