//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stdint.h>

/**
 * @brief Get the current time in milliseconds.
 * @return The current time in milliseconds.
 * @note The returned value is not steady and is not guaranteed to be monotonic.
 */
axis_UTILS_API int64_t axis_current_time(void);

/**
 * @brief Get the current time in microseconds.
 * @return The current time in milliseconds.
 * @note The returned value is not steady and is not guaranteed to be monotonic.
 */
axis_UTILS_API int64_t axis_current_time_us(void);

/**
 * @brief Sleep for a specified time in milliseconds.
 * @param msec The time to sleep in milliseconds.
 */
axis_UTILS_API void axis_sleep(int64_t msec);

/**
 * @brief Sleep for a random time in milliseconds.
 * @param msec The maximum time to sleep in milliseconds.
 */
axis_UTILS_API void axis_random_sleep(int64_t max_msec);

/**
 * @brief Sleep for a specified time in microseconds.
 * @param msec The time to sleep in microseconds.
 */
axis_UTILS_API void axis_usleep(int64_t usec);
