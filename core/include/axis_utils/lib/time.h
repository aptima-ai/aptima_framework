//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include <stdint.h>

/**
 * @brief Get the current time in milliseconds.
 * @return The current time in milliseconds.
 * @note The returned value is not steady and is not guaranteed to be monotonic.
 */
aptima_UTILS_API int64_t aptima_current_time(void);

/**
 * @brief Get the current time in microseconds.
 * @return The current time in milliseconds.
 * @note The returned value is not steady and is not guaranteed to be monotonic.
 */
aptima_UTILS_API int64_t aptima_current_time_us(void);

/**
 * @brief Sleep for a specified time in milliseconds.
 * @param msec The time to sleep in milliseconds.
 */
aptima_UTILS_API void aptima_sleep(int64_t msec);

/**
 * @brief Sleep for a random time in milliseconds.
 * @param msec The maximum time to sleep in milliseconds.
 */
aptima_UTILS_API void aptima_random_sleep(int64_t max_msec);

/**
 * @brief Sleep for a specified time in microseconds.
 * @param msec The time to sleep in microseconds.
 */
aptima_UTILS_API void aptima_usleep(int64_t usec);
