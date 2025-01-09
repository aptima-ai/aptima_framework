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
#define aptima_thread_once_t INIT_ONCE
#define aptima_THREAD_ONCE_INIT INIT_ONCE_STATIC_INIT
#else
#include <pthread.h>
#define aptima_thread_once_t pthread_once_t
#define aptima_THREAD_ONCE_INIT PTHREAD_ONCE_INIT
#endif

/**
 * @brief Initialize a thread-once object.
 * @param once Pointer to the thread-once object.
 * @param init_routine Pointer to the initialization routine.
 * @return 0 on success, or a negative error code.
 * @note This function is will guarantee that the initialization routine will be
 *       called only once.
 */
aptima_UTILS_API int aptima_thread_once(aptima_thread_once_t *once,
                                  void (*init_routine)(void));
