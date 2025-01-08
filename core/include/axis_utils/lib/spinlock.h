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
#include "axis_utils/lib/task.h"
#include "axis_utils/lib/thread.h"

/**
 * This is actually a "busy loop" with "pause" instruction.
 * It's not possible to implement a "real" spin lock in userspace because
 * you have no way to disable thread schedule and interrupts.
 */

typedef struct axis_spinlock_t {
  axis_atomic_t lock;
} axis_spinlock_t;

#define axis_SPINLOCK_INIT {0}

/**
 * @brief Initialize a spinlock.
 */
axis_UTILS_API void axis_spinlock_init(axis_spinlock_t *spin);

/**
 * @brief Initialize a spinlock from address
 * @note If |addr| exists in a shared memory, this lock can be used as IPC lock
 */
axis_UTILS_API axis_spinlock_t *axis_spinlock_from_addr(axis_atomic_t *addr);

/**
 * @brief Acquire a spinlock.
 * @note This function will block if the lock is held by others. Recursively
 *       acquire the same lock will result in dead lock
 */
axis_UTILS_API void axis_spinlock_lock(axis_spinlock_t *spin);

/**
 * @brief Try to acquire a spinlock.
 * @return 0 if the lock is acquired, -1 otherwise.
 */
axis_UTILS_API int axis_spinlock_trylock(axis_spinlock_t *spin);

/**
 * @brief Release a spinlock.
 */
axis_UTILS_API void axis_spinlock_unlock(axis_spinlock_t *spin);

typedef struct axis_recursive_spinlock_t {
  axis_spinlock_t lock;
  volatile axis_pid_t pid;
  volatile axis_tid_t tid;
  volatile int64_t count;
} axis_recursive_spinlock_t;

#define axis_RECURSIVE_SPINLOCK_INIT {axis_SPINLOCK_INIT, -1, -1, 0}

/**
 * @brief Initialize a recursive spinlock
 */
axis_UTILS_API void axis_recursive_spinlock_init(axis_recursive_spinlock_t *spin);

/**
 * @brief Initialize a recursive spinlock from address
 * @note If |addr| exists in a shared memory, this lock can be used as IPC lock
 */
axis_UTILS_API axis_recursive_spinlock_t *axis_recursive_spinlock_from_addr(
    uint8_t addr[sizeof(axis_recursive_spinlock_t)]);

/**
 * @brief Acquire a recursive spinlock.
 * @note This function will block if the lock is held by another thread.
 */
axis_UTILS_API void axis_recursive_spinlock_lock(axis_recursive_spinlock_t *spin);

/**
 * @brief Try to acquire a recursive spinlock.
 * @return 0 if the lock is acquired, -1 otherwise.
 */
axis_UTILS_API int axis_recursive_spinlock_trylock(
    axis_recursive_spinlock_t *spin);

/**
 * @brief Release a recursive spinlock.
 */
axis_UTILS_API void axis_recursive_spinlock_unlock(
    axis_recursive_spinlock_t *spin);
