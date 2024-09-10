//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_utils/ten_config.h"

/**
 * @brief When defined, OutputDebugString() will be used instead of stderr
 * (ignored on non-Windows platforms). Uses OutputDebugStringA() variant and
 * feeds it with UTF-8 data.
 */
#ifdef TEN_LOG_USE_DEBUGSTRING
  #undef TEN_LOG_USE_DEBUGSTRING

  #if defined(_WIN32) || defined(_WIN64)
    #define TEN_LOG_USE_DEBUGSTRING 1
  #else
    #define TEN_LOG_USE_DEBUGSTRING 0
  #endif

#else
  #define TEN_LOG_USE_DEBUGSTRING 0
#endif

#if TEN_LOG_USE_DEBUGSTRING

enum { OUT_DEBUGSTRING_MASK = TEN_LOG_PUT_STD };

TEN_UTILS_PRIVATE_API void out_debugstring_cb(const ten_log_message_t *msg,
                                              void *arg);

  #define OUT_DEBUGSTRING \
    false, OUT_DEBUGSTRING_MASK, out_debugstring_cb, NULL, NULL

#endif
