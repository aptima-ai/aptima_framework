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
#include "axis_utils/lib/event.h"

typedef enum REFLOCK_FLAG {
  axis_REFLOCK_REF = (int64_t)0x00000001,
  axis_REFLOCK_DESTROY = (int64_t)0x10000000,
  axis_REFLOCK_POISON = (int64_t)0x300dead0,
  axis_REFLOCK_DESTROY_MASK = (int64_t)0xf0000000,
  axis_REFLOCK_REF_MASK = (int64_t)0x0fffffff,
} REFLOCK_FLAG;

typedef struct axis_reflock_t {
  axis_atomic_t state;
  axis_event_t *event;
} axis_reflock_t;

/**
 * @brief Initialize a reflock.
 * @param reflock The reflock to initialize.
 */
axis_UTILS_API void axis_reflock_init(axis_reflock_t *lock);

/**
 * @brief Increase the reference count of a reflock.
 * @param reflock The reflock to increase the reference count.
 */
axis_UTILS_API void axis_reflock_ref(axis_reflock_t *lock);

/**
 * @brief Decrease the reference count of a reflock.
 * @param reflock The reflock to decrease the reference count.
 */
axis_UTILS_API void axis_reflock_unref(axis_reflock_t *lock);

/**
 * @brief Decrease reference count and destroy after it's zero.
 * @param reflock The reflock to decrease the reference count.
 * @note This function will wait until the reflock is zero
 */
axis_UTILS_API void axis_reflock_unref_destroy(axis_reflock_t *lock);
