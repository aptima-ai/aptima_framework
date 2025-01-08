//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include "axis_utils/lib/alloc.h"
#include "axis_utils/sanitizer/memory_check.h"  // IWYU pragma: keep

#if defined(axis_ENABLE_MEMORY_CHECK)

#define axis_MALLOC(size) \
  axis_sanitizer_memory_malloc((size), __FILE__, __LINE__, __FUNCTION__)

#define axis_CALLOC(cnt, size) \
  axis_sanitizer_memory_calloc((cnt), (size), __FILE__, __LINE__, __FUNCTION__)

#define axis_FREE(address)                         \
  do {                                            \
    axis_sanitizer_memory_free((void *)(address)); \
    address = NULL;                               \
  } while (0)

#define axis_FREE_(address)                        \
  do {                                            \
    axis_sanitizer_memory_free((void *)(address)); \
  } while (0)

#define axis_REALLOC(address, size)                                    \
  axis_sanitizer_memory_realloc((address), (size), __FILE__, __LINE__, \
                               __FUNCTION__)

#define axis_STRDUP(str) \
  axis_sanitizer_memory_strdup((str), __FILE__, __LINE__, __FUNCTION__)

#else

#define axis_MALLOC(size) axis_malloc((size))

#define axis_CALLOC(cnt, size) axis_calloc((cnt), (size))

#define axis_FREE(address)        \
  do {                           \
    axis_free((void *)(address)); \
    address = NULL;              \
  } while (0)

#define axis_FREE_(address)       \
  do {                           \
    axis_free((void *)(address)); \
  } while (0)

#define axis_REALLOC(address, size) axis_realloc((address), (size))

#define axis_STRDUP(str) axis_strdup((str))

#endif
