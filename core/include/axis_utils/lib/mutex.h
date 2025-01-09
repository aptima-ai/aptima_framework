//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include "aptima_utils/macro/check.h"

#define aptima_DO_WITH_MUTEX_LOCK(lock, blocks)                  \
  do {                                                        \
    int rc = aptima_mutex_lock(lock);                            \
    aptima_ASSERT(!rc, "Unable to lock, error code: %d.", rc);   \
                                                              \
    {blocks}                                                  \
                                                              \
    rc = aptima_mutex_unlock(lock);                              \
    aptima_ASSERT(!rc, "Unable to unlock, error code: %d.", rc); \
  } while (0)

typedef struct aptima_mutex_t aptima_mutex_t;

/**
 * @brief Create a mutex.
 * @return The mutex handle.
 */
aptima_UTILS_API aptima_mutex_t *aptima_mutex_create(void);

/**
 * @brief Lock a mutex.
 * @param mutex The mutex handle.
 * @return 0 if success, otherwise failed.
 *
 * @note This function will block until the mutex is unlocked.
 */
aptima_UTILS_API int aptima_mutex_lock(aptima_mutex_t *mutex);

/**
 * @brief Unlock a mutex.
 * @param mutex The mutex handle.
 * @return 0 if success, otherwise failed.
 */
aptima_UTILS_API int aptima_mutex_unlock(aptima_mutex_t *mutex);

/**
 * @brief Destroy a mutex.
 * @param mutex The mutex handle.
 */
aptima_UTILS_API void aptima_mutex_destroy(aptima_mutex_t *mutex);

/**
 * @brief Get system mutex handle.
 * @param mutex The mutex handle.
 * @return The system mutex handle.
 */
aptima_UTILS_API void *aptima_mutex_get_native_handle(aptima_mutex_t *mutex);
