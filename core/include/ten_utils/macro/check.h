//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include <assert.h>
#include <stdio.h>   // IWYU pragma: keep
#include <stdlib.h>  // IWYU pragma: keep

#include "ten_utils/backtrace/backtrace.h"  // IWYU pragma: keep
#include "ten_utils/log/log.h"
#include "ten_utils/ten_config.h"

#if defined(__has_feature)
  #if __has_feature(address_sanitizer)
    #define TEN_USE_ASAN
  #endif
#endif

#if defined(__SANITIZE_ADDRESS__)
  #define TEN_USE_ASAN
#endif

#if defined(TEN_PRODUCTION)

// Remove all protections in the final production release.

  #define TEN_ASSERT(expr, fmt, ...) \
    do {                             \
    } while (0)

#else  // TEN_PRODUCTION

  #ifndef NDEBUG

    #define TEN_ASSERT(expr, fmt, ...)                         \
      do {                                                     \
        /* NOLINTNEXTLINE(readability-simplify-boolean-expr)*/ \
        if (!(expr)) {                                         \
          TEN_LOGE(fmt, ##__VA_ARGS__);                        \
          ten_backtrace_dump_global(0);                        \
          /* NOLINTNEXTLINE */                                 \
          assert(0);                                           \
        }                                                      \
      } while (0)

  #else  // NDEBUG

  // Enable minimal protection if the optimization is enabled.

    #define TEN_ASSERT(expr, fmt, ...)  \
      do {                              \
        if (!(expr)) {                  \
          TEN_LOGE(fmt, ##__VA_ARGS__); \
          ten_backtrace_dump_global(0); \
          abort();                      \
        }                               \
      } while (0)

  #endif  // NDEBUG

#endif  // TEN_PRODUCTION
