//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#if defined(_WIN32)
#include <Windows.h>
typedef DWORD aptima_thread_key_t;
#else
#include <pthread.h>
typedef pthread_key_t aptima_thread_key_t;
#endif

#define kInvalidTlsKey ((aptima_thread_key_t) - 1)

/**
 * @brief Create a thread local storage key.
 * @return The key.
 */
aptima_UTILS_API aptima_thread_key_t aptima_thread_key_create(void);

/**
 * @brief Delete a thread local storage key.
 * @param key The key.
 */
aptima_UTILS_API void aptima_thread_key_destroy(aptima_thread_key_t key);

/**
 * @brief Set the value of a thread local storage key.
 * @param key The key.
 * @param value The value.
 */
aptima_UTILS_API int aptima_thread_set_key(aptima_thread_key_t key, void *value);

/**
 * @brief Get the value of a thread local storage key.
 * @param key The key.
 * @return The value.
 */
aptima_UTILS_API void *aptima_thread_get_key(aptima_thread_key_t key);
