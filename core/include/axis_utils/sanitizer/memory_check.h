//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include "axis_utils/container/list.h"
#include "axis_utils/lib/mutex.h"
#include "axis_utils/lib/string.h"

// As the source files are compiled in `out/<os>/<cpu>`, the `__FILE__`
// will be a relative path starts with '../../../'.
#define axis_FILE_PATH_RELATIVE_PREFIX_LENGTH 9

typedef struct axis_sanitizer_memory_record_t {
  void *addr;
  size_t size;

  // Do not use `axis_string_t` here to avoid a circular dependency between
  // `axis_string_t` and `axis_malloc`.
  char *func_name;
  char *file_name;

  uint32_t lineno;
} axis_sanitizer_memory_record_t;

typedef struct axis_sanitizer_memory_records_t {
  axis_mutex_t *lock;
  axis_list_t records;  // axis_sanitizer_memory_record_t
  size_t total_size;
} axis_sanitizer_memory_records_t;

axis_UTILS_API void axis_sanitizer_memory_record_init(void);

axis_UTILS_API void axis_sanitizer_memory_record_deinit(void);

axis_UTILS_API void axis_sanitizer_memory_record_dump(void);

/**
 * @brief Malloc and record memory info.
 * @see axis_MALLOC
 * @note Please free memory using axis_sanitizer_memory_free().
 */
axis_UTILS_API void *axis_sanitizer_memory_malloc(size_t size,
                                                const char *file_name,
                                                uint32_t lineno,
                                                const char *func_name);

axis_UTILS_API void *axis_sanitizer_memory_calloc(size_t cnt, size_t size,
                                                const char *file_name,
                                                uint32_t lineno,
                                                const char *func_name);

/**
 * @brief Free memory and remove the record.
 * @see axis_free
 */
axis_UTILS_API void axis_sanitizer_memory_free(void *address);

/**
 * @brief Realloc memory and record memory info.
 * @see axis_realloc
 * @note Please free memory using axis_sanitizer_memory_free().
 */
axis_UTILS_API void *axis_sanitizer_memory_realloc(void *addr, size_t size,
                                                 const char *file_name,
                                                 uint32_t lineno,
                                                 const char *func_name);

/**
 * @brief Duplicate string and record memory info.
 * @see axis_strdup
 * @note Please free memory using axis_sanitizer_memory_free().
 */
axis_UTILS_API char *axis_sanitizer_memory_strdup(const char *str,
                                                const char *file_name,
                                                uint32_t lineno,
                                                const char *func_name);
