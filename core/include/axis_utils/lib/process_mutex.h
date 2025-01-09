//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

typedef struct aptima_process_mutex_t aptima_process_mutex_t;

/**
 * @brief Create a mutex.
 * @return The mutex handle.
 */
aptima_UTILS_API aptima_process_mutex_t *aptima_process_mutex_create(const char *name);

/**
 * @brief Lock a mutex.
 * @param mutex The mutex handle.
 * @return 0 if success, otherwise failed.
 *
 * @note This function will block until the mutex is unlocked.
 */
aptima_UTILS_API int aptima_process_mutex_lock(aptima_process_mutex_t *mutex);

/**
 * @brief Unlock a mutex.
 * @param mutex The mutex handle.
 * @return 0 if success, otherwise failed.
 */
aptima_UTILS_API int aptima_process_mutex_unlock(aptima_process_mutex_t *mutex);

/**
 * @brief Destroy a mutex.
 * @param mutex The mutex handle.
 */
aptima_UTILS_API void aptima_process_mutex_destroy(aptima_process_mutex_t *mutex);
