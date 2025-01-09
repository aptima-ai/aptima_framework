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
#include "aptima_utils/lib/task.h"
#include "aptima_utils/lib/thread.h"

/**
 * This is actually a "busy loop" with "pause" instruction.
 * It's not possible to implement a "real" spin lock in userspace because
 * you have no way to disable thread schedule and interrupts.
 */

typedef struct aptima_spinlock_t {
  aptima_atomic_t lock;
} aptima_spinlock_t;

#define aptima_SPINLOCK_INIT {0}

/**
 * @brief Initialize a spinlock.
 */
aptima_UTILS_API void aptima_spinlock_init(aptima_spinlock_t *spin);

/**
 * @brief Initialize a spinlock from address
 * @note If |addr| exists in a shared memory, this lock can be used as IPC lock
 */
aptima_UTILS_API aptima_spinlock_t *aptima_spinlock_from_addr(aptima_atomic_t *addr);

/**
 * @brief Acquire a spinlock.
 * @note This function will block if the lock is held by others. Recursively
 *       acquire the same lock will result in dead lock
 */
aptima_UTILS_API void aptima_spinlock_lock(aptima_spinlock_t *spin);

/**
 * @brief Try to acquire a spinlock.
 * @return 0 if the lock is acquired, -1 otherwise.
 */
aptima_UTILS_API int aptima_spinlock_trylock(aptima_spinlock_t *spin);

/**
 * @brief Release a spinlock.
 */
aptima_UTILS_API void aptima_spinlock_unlock(aptima_spinlock_t *spin);

typedef struct aptima_recursive_spinlock_t {
  aptima_spinlock_t lock;
  volatile aptima_pid_t pid;
  volatile aptima_tid_t tid;
  volatile int64_t count;
} aptima_recursive_spinlock_t;

#define aptima_RECURSIVE_SPINLOCK_INIT {aptima_SPINLOCK_INIT, -1, -1, 0}

/**
 * @brief Initialize a recursive spinlock
 */
aptima_UTILS_API void aptima_recursive_spinlock_init(aptima_recursive_spinlock_t *spin);

/**
 * @brief Initialize a recursive spinlock from address
 * @note If |addr| exists in a shared memory, this lock can be used as IPC lock
 */
aptima_UTILS_API aptima_recursive_spinlock_t *aptima_recursive_spinlock_from_addr(
    uint8_t addr[sizeof(aptima_recursive_spinlock_t)]);

/**
 * @brief Acquire a recursive spinlock.
 * @note This function will block if the lock is held by another thread.
 */
aptima_UTILS_API void aptima_recursive_spinlock_lock(aptima_recursive_spinlock_t *spin);

/**
 * @brief Try to acquire a recursive spinlock.
 * @return 0 if the lock is acquired, -1 otherwise.
 */
aptima_UTILS_API int aptima_recursive_spinlock_trylock(
    aptima_recursive_spinlock_t *spin);

/**
 * @brief Release a recursive spinlock.
 */
aptima_UTILS_API void aptima_recursive_spinlock_unlock(
    aptima_recursive_spinlock_t *spin);
