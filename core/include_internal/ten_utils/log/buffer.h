//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_utils/ten_config.h"

#include <assert.h>

#if defined(__linux__)
  #include <linux/limits.h>
#elif defined(__MACH__)
  #include <sys/syslimits.h>
#endif

#include "include_internal/ten_utils/log/eol.h"
#include "ten_utils/log/log.h"

/**
 * @brief Size of the log line buffer. The buffer is allocated on stack. It
 * limits maximum length of a log line.
 */
#ifndef TEN_LOG_BUF_SZ
  #if defined(OS_MACOS)
    #define TEN_LOG_BUF_SZ 512
  #else
  // OPTIMIZE(Wei): Use a smaller value.
    #define TEN_LOG_BUF_SZ 4096
  #endif
#endif

static_assert(TEN_LOG_EOL_SZ < TEN_LOG_BUF_SZ, "eol_sz_less_than_buf_sz");

#if !defined(_WIN32) && !defined(_WIN64)
static_assert(TEN_LOG_BUF_SZ <= PIPE_BUF, "buf_sz_less_than_pipe_buf");
#endif

typedef void (*buffer_cb)(ten_log_message_t *msg, char *buf);

TEN_UTILS_PRIVATE_API buffer_cb g_buffer_cb;

TEN_UTILS_PRIVATE_API void buffer_callback(ten_log_message_t *msg, char *buf);
