//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_utils/ten_config.h"

/**
 * @brief When defined, NSLog (uses Apple System Log) will be used instead of
 * stderr (ignored on non-Apple platforms). Date, time, pid and tid (context)
 * will be provided by NSLog. Curiously, doesn't use NSLog() directly, but
 * piggybacks on non-public CFLog() function. Both use Apple System Log
 * internally, but it's easier to call CFLog() from C than NSLog(). Current
 * implementation doesn't support "%@" format specifier.
 */
#ifdef TEN_LOG_USE_NSLOG
  #undef TEN_LOG_USE_NSLOG

  #if defined(__APPLE__) && defined(__MACH__)
    #define TEN_LOG_USE_NSLOG 1
  #else
    #define TEN_LOG_USE_NSLOG 0
  #endif

#else
  #define TEN_LOG_USE_NSLOG 0
#endif

#if TEN_LOG_USE_NSLOG

enum { OUT_NSLOG_MASK = TEN_LOG_PUT_STD & ~TEN_LOG_PUT_CTX };

TEN_UTILS_PRIVATE_API void out_nslog_cb(const ten_log_message_t *msg,
                                        void *arg);

  #define OUT_NSLOG false, OUT_NSLOG_MASK, out_nslog_cb, NULL, NULL

#endif
