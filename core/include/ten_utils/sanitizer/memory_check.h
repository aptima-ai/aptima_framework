//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_utils/ten_config.h"

#include "ten_utils/container/list.h"
#include "ten_utils/lib/mutex.h"
#include "ten_utils/lib/string.h"

// As the source files are compiled in `out/<os>/<cpu>`, the `__FILE__`
// will be a relative path starts with '../../../'.
#define TEN_FILE_PATH_RELATIVE_PREFIX_LENGTH 9

typedef struct ten_sanitizer_memory_record_t {
  void *addr;
  size_t size;

  // Do not use `ten_string_t` here to avoid a circular dependency between
  // `ten_string_t` and `ten_malloc`.
  char *func_name;
  char *file_name;

  uint32_t lineno;
} ten_sanitizer_memory_record_t;

typedef struct ten_sanitizer_memory_records_t {
  ten_mutex_t *lock;
  ten_list_t records;  // ten_sanitizer_memory_record_t
  size_t total_size;
} ten_sanitizer_memory_records_t;

TEN_UTILS_API void ten_sanitizer_memory_record_init(void);

TEN_UTILS_API void ten_sanitizer_memory_record_deinit(void);

TEN_UTILS_API void ten_sanitizer_memory_record_dump(void);

/**
 * @brief Malloc and record memory info.
 * @see TEN_MALLOC
 * @note Please free memory using ten_sanitizer_memory_free().
 */
TEN_UTILS_API void *ten_sanitizer_memory_malloc(size_t size,
                                                const char *file_name,
                                                uint32_t lineno,
                                                const char *func_name);

TEN_UTILS_API void *ten_sanitizer_memory_calloc(size_t cnt, size_t size,
                                                const char *file_name,
                                                uint32_t lineno,
                                                const char *func_name);

/**
 * @brief Free memory and remove the record.
 * @see ten_free
 */
TEN_UTILS_API void ten_sanitizer_memory_free(void *address);

/**
 * @brief Realloc memory and record memory info.
 * @see ten_realloc
 * @note Please free memory using ten_sanitizer_memory_free().
 */
TEN_UTILS_API void *ten_sanitizer_memory_realloc(void *addr, size_t size,
                                                 const char *file_name,
                                                 uint32_t lineno,
                                                 const char *func_name);

/**
 * @brief Duplicate string and record memory info.
 * @see ten_strdup
 * @note Please free memory using ten_sanitizer_memory_free().
 */
TEN_UTILS_API char *ten_sanitizer_memory_strdup(const char *str,
                                                const char *file_name,
                                                uint32_t lineno,
                                                const char *func_name);
