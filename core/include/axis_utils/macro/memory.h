//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include "aptima_utils/lib/alloc.h"
#include "aptima_utils/sanitizer/memory_check.h"  // IWYU pragma: keep

#if defined(aptima_ENABLE_MEMORY_CHECK)

#define aptima_MALLOC(size) \
  aptima_sanitizer_memory_malloc((size), __FILE__, __LINE__, __FUNCTION__)

#define aptima_CALLOC(cnt, size) \
  aptima_sanitizer_memory_calloc((cnt), (size), __FILE__, __LINE__, __FUNCTION__)

#define aptima_FREE(address)                         \
  do {                                            \
    aptima_sanitizer_memory_free((void *)(address)); \
    address = NULL;                               \
  } while (0)

#define aptima_FREE_(address)                        \
  do {                                            \
    aptima_sanitizer_memory_free((void *)(address)); \
  } while (0)

#define aptima_REALLOC(address, size)                                    \
  aptima_sanitizer_memory_realloc((address), (size), __FILE__, __LINE__, \
                               __FUNCTION__)

#define aptima_STRDUP(str) \
  aptima_sanitizer_memory_strdup((str), __FILE__, __LINE__, __FUNCTION__)

#else

#define aptima_MALLOC(size) aptima_malloc((size))

#define aptima_CALLOC(cnt, size) aptima_calloc((cnt), (size))

#define aptima_FREE(address)        \
  do {                           \
    aptima_free((void *)(address)); \
    address = NULL;              \
  } while (0)

#define aptima_FREE_(address)       \
  do {                           \
    aptima_free((void *)(address)); \
  } while (0)

#define aptima_REALLOC(address, size) aptima_realloc((address), (size))

#define aptima_STRDUP(str) aptima_strdup((str))

#endif
