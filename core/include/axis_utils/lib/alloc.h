//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include <stddef.h>

#include "aptima_utils/macro/memory.h"  // IWYU pragma: export

/**
 * @brief Allocate a buffer
 * @param size: Size of buffer
 * @return: Address of buffer if success, NULL otherwise
 */
aptima_UTILS_API void *aptima_malloc(size_t size);

/**
 * @brief Allocate a buffer
 * @param size: Size of buffer
 * @return: Address of buffer if success, NULL otherwise
 */
aptima_UTILS_API void *aptima_calloc(size_t cnt, size_t size);

/**
 * @brief Re-allocate a buffer with new size
 * @param p: Address of buffer
 * @param size: New size of buffer
 * @return: Address of buffer if success, NULL otherwise
 */
aptima_UTILS_API void *aptima_realloc(void *p, size_t size);

/**
 * @brief Deallocate a buffer
 * @param p: Address of buffer
 * @note It is safe to free a NULL pointer
 */
aptima_UTILS_API void aptima_free(void *p);

/**
 * @brief Duplicate a string
 * @param str: String that needs duplicate
 * @return: Address of new string
 * @note: Please free memory using |aptima_free|
 */
aptima_UTILS_API char *aptima_strdup(const char *str);

aptima_UTILS_API void *aptima_malloc_without_backtrace(size_t size);

aptima_UTILS_API void aptima_free_without_backtrace(void *p);

aptima_UTILS_API void *aptima_calloc_without_backtrace(size_t cnt, size_t size);

aptima_UTILS_API void *aptima_realloc_without_backtrace(void *p, size_t size);

aptima_UTILS_API char *aptima_strdup_without_backtrace(const char *str);
