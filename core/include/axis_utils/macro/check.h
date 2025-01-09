//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include <assert.h>
#include <stdio.h>   // IWYU pragma: keep
#include <stdlib.h>  // IWYU pragma: keep

#include "aptima_utils/backtrace/backtrace.h"  // IWYU pragma: keep

#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define aptima_USE_ASAN
#endif
#endif

#if defined(__SANITIZE_ADDRESS__)
#define aptima_USE_ASAN
#endif

#if defined(aptima_PRODUCTION)

// Remove all protections in the final production release.

#define aptima_ASSERT(expr, fmt, ...) \
  do {                             \
  } while (0)

#else  // aptima_PRODUCTION

// aptima_ASSERT is used within `aptima_string_t`, so do not use `aptima_string_t` inside
// `aptima_ASSERT` to avoid circular dependencies.
//
// This size should not be too large; otherwise, it may cause a stack overflow
// in the deep call chain.
#define ASSERT_ERR_MSG_MAX_LENGTH 128

#ifndef NDEBUG

#define aptima_ASSERT(expr, fmt, ...)                                        \
  do {                                                                    \
    /* NOLINTNEXTLINE */                                                  \
    if (!(expr)) {                                                        \
      /* NOLINTNEXTLINE */                                                \
      char ____err_msg[ASSERT_ERR_MSG_MAX_LENGTH];                        \
      int written =                                                       \
          snprintf(____err_msg, sizeof(____err_msg), fmt, ##__VA_ARGS__); \
      assert(written > 0);                                                \
      written = fprintf(stderr, "%s\n", ____err_msg);                     \
      assert(written > 0);                                                \
      aptima_backtrace_dump_global(0);                                       \
      /* NOLINTNEXTLINE */                                                \
      assert(0);                                                          \
    }                                                                     \
  } while (0)

#else  // NDEBUG

// Enable minimal protection if the optimization is enabled.

#define aptima_ASSERT(expr, fmt, ...)                                        \
  do {                                                                    \
    /* NOLINTNEXTLINE */                                                  \
    if (!(expr)) {                                                        \
      /* NOLINTNEXTLINE */                                                \
      char ____err_msg[ASSERT_ERR_MSG_MAX_LENGTH];                        \
      int written =                                                       \
          snprintf(____err_msg, sizeof(____err_msg), fmt, ##__VA_ARGS__); \
      if (written <= 0) {                                                 \
        abort();                                                          \
      }                                                                   \
      written = fprintf(stderr, "%s\n", ____err_msg);                     \
      if (written <= 0) {                                                 \
        abort();                                                          \
      }                                                                   \
      aptima_backtrace_dump_global(0);                                       \
      /* NOLINTNEXTLINE */                                                \
      abort();                                                            \
    }                                                                     \
  } while (0)

#endif  // NDEBUG

#endif  // aptima_PRODUCTION
