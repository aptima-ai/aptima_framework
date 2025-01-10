//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include <stdint.h>

typedef struct aptima_cond_t aptima_cond_t;
typedef struct aptima_mutex_t aptima_mutex_t;

/**
 * @brief Create a condition variable.
 */
aptima_UTILS_API aptima_cond_t *aptima_cond_create(void);

/**
 * @brief Destroy a condition variable.
 */
aptima_UTILS_API void aptima_cond_destroy(aptima_cond_t *cond);

/**
 * @brief Wait on a condition variable.
 * @param cond The condition variable to wait on.
 * @param mutex The mutex to unlock while waiting.
 * @param wait_ms The maximum time to wait in milliseconds.
 * @return 0 on success, -1 on error.
 *
 * @note This function will unlock the mutex before waiting and lock it again
 *       when it is signaled. Surprise wakeup still happens just like pthread
 *       version of condition variable.
 */
aptima_UTILS_API int aptima_cond_wait(aptima_cond_t *cond, aptima_mutex_t *mutex,
                                int64_t wait_ms);

/**
 * @brief Wait on a condition variable while predicate() is true.
 * @param cond The condition variable to wait on.
 * @param mutex The mutex to unlock while waiting.
 * @param predicate The predicate to check.
 * @param arg The argument to pass to predicate().
 * @param wait_ms The maximum time to wait in milliseconds.
 * @return 0 on success, -1 on error.
 *
 * @note This function will unlock the mutex before waiting and lock it again
 *       when it is signaled. Surprise wakeup does _not_ happen because we
 *       instantly check predicate() before leaving.
 */
aptima_UTILS_API int aptima_cond_wait_while(aptima_cond_t *cond, aptima_mutex_t *mutex,
                                      int (*predicate)(void *), void *arg,
                                      int64_t wait_ms);

/**
 * @brief Signal a condition variable.
 * @param cond The condition variable to signal.
 * @return 0 on success, -1 on error.
 */
aptima_UTILS_API int aptima_cond_signal(aptima_cond_t *cond);

/**
 * @brief Broadcast a condition variable.
 * @param cond The condition variable to broadcast.
 * @return 0 on success, -1 on error.
 */
aptima_UTILS_API int aptima_cond_broadcast(aptima_cond_t *cond);
