//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/ten_config.h"

#include "include_internal/ten_utils/log/buffer.h"

#include "include_internal/ten_utils/log/eol.h"

static size_t g_buf_sz = TEN_LOG_BUF_SZ - TEN_LOG_EOL_SZ;

buffer_cb g_buffer_cb = buffer_callback;

void buffer_callback(ten_log_message_t *log_msg, char *buf) {
  log_msg->buf_end =
      (log_msg->buf_content_end = log_msg->buf_start = buf) + g_buf_sz;
}
