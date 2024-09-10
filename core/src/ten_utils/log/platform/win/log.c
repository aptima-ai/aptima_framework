//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/ten_config.h"

#include "include_internal/ten_utils/log/platform/win/log.h"

#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"

#if TEN_LOG_USE_DEBUGSTRING

  #include <windows.h>

void out_debugstring_cb(const ten_log_message_t *msg, void *arg) {
  TEN_ASSERT(msg, "Invalid argument.");

  VAR_UNUSED(arg);

  msg->buf_content_end[0] = '\n';
  msg->buf_content_end[1] = '\0';

  OutputDebugStringA(msg->buf_start);
}

#endif
