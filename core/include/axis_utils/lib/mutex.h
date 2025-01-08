//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include "axis_utils/macro/check.h"

#define axis_DO_WITH_MUTEX_LOCK(lock, blocks)                  \
  do {                                                        \
    int rc = axis_mutex_lock(lock);                            \
    axis_ASSERT(!rc, "Unable to lock, error code: %d.", rc);   \
                                                              \
    {blocks}                                                  \
                                                              \
    rc = axis_mutex_unlock(lock);                              \
    axis_ASSERT(!rc, "Unable to unlock, error code: %d.", rc); \
  } while (0)

typedef struct axis_mutex_t axis_mutex_t;

/**
 * @brief Create a mutex.
 * @return The mutex handle.
 */
axis_UTILS_API axis_mutex_t *axis_mutex_create(void);

/**
 * @brief Lock a mutex.
 * @param mutex The mutex handle.
 * @return 0 if success, otherwise failed.
 *
 * @note This function will block until the mutex is unlocked.
 */
axis_UTILS_API int axis_mutex_lock(axis_mutex_t *mutex);

/**
 * @brief Unlock a mutex.
 * @param mutex The mutex handle.
 * @return 0 if success, otherwise failed.
 */
axis_UTILS_API int axis_mutex_unlock(axis_mutex_t *mutex);

/**
 * @brief Destroy a mutex.
 * @param mutex The mutex handle.
 */
axis_UTILS_API void axis_mutex_destroy(axis_mutex_t *mutex);

/**
 * @brief Get system mutex handle.
 * @param mutex The mutex handle.
 * @return The system mutex handle.
 */
axis_UTILS_API void *axis_mutex_get_native_handle(axis_mutex_t *mutex);
