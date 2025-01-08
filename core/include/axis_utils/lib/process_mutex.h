//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

typedef struct axis_process_mutex_t axis_process_mutex_t;

/**
 * @brief Create a mutex.
 * @return The mutex handle.
 */
axis_UTILS_API axis_process_mutex_t *axis_process_mutex_create(const char *name);

/**
 * @brief Lock a mutex.
 * @param mutex The mutex handle.
 * @return 0 if success, otherwise failed.
 *
 * @note This function will block until the mutex is unlocked.
 */
axis_UTILS_API int axis_process_mutex_lock(axis_process_mutex_t *mutex);

/**
 * @brief Unlock a mutex.
 * @param mutex The mutex handle.
 * @return 0 if success, otherwise failed.
 */
axis_UTILS_API int axis_process_mutex_unlock(axis_process_mutex_t *mutex);

/**
 * @brief Destroy a mutex.
 * @param mutex The mutex handle.
 */
axis_UTILS_API void axis_process_mutex_destroy(axis_process_mutex_t *mutex);
