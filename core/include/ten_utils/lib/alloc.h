//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_utils/ten_config.h"

#include <stddef.h>

#include "ten_utils/macro/memory.h"  // IWYU pragma: export

/**
 * @brief Allocate a buffer
 * @param size: Size of buffer
 * @return: Address of buffer if success, NULL otherwise
 */
TEN_UTILS_API void *ten_malloc(size_t size);

/**
 * @brief Allocate a buffer
 * @param size: Size of buffer
 * @return: Address of buffer if success, NULL otherwise
 */
TEN_UTILS_API void *ten_calloc(size_t cnt, size_t size);

/**
 * @brief Re-allocate a buffer with new size
 * @param p: Address of buffer
 * @param size: New size of buffer
 * @return: Address of buffer if success, NULL otherwise
 */
TEN_UTILS_API void *ten_realloc(void *p, size_t size);

/**
 * @brief Deallocate a buffer
 * @param p: Address of buffer
 * @note It is safe to free a NULL pointer
 */
TEN_UTILS_API void ten_free(void *p);

/**
 * @brief Duplicate a string
 * @param str: String that needs duplicate
 * @return: Address of new string
 * @note: Please free memory using |ten_free|
 */
TEN_UTILS_API char *ten_strdup(const char *str);

TEN_UTILS_API void *ten_malloc_without_backtrace(size_t size);

TEN_UTILS_API void ten_free_without_backtrace(void *p);

TEN_UTILS_API void *ten_calloc_without_backtrace(size_t cnt, size_t size);

TEN_UTILS_API void *ten_realloc_without_backtrace(void *p, size_t size);

TEN_UTILS_API char *ten_strdup_without_backtrace(const char *str);
