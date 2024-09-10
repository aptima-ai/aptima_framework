//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/ten_config.h"

#include "include_internal/ten_utils/log/platform/mac/log.h"

#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"

#if TEN_LOG_USE_NSLOG

  #include <CoreFoundation/CoreFoundation.h>

CF_EXPORT void CFLog(int32_t level, CFStringRef format, ...);

static int apple_level(const int level) {
  switch (level) {
    case TEN_LOG_VERBOSE:
      return 7; /* ASL_LEVEL_DEBUG / kCFLogLevelDebug */

    case TEN_LOG_DEBUG:
      return 7; /* ASL_LEVEL_DEBUG / kCFLogLevelDebug */

    case TEN_LOG_INFO:
      return 6; /* ASL_LEVEL_INFO / kCFLogLevelInfo */

    case TEN_LOG_WARN:
      return 4; /* ASL_LEVEL_WARNING / kCFLogLevelWarning */

    case TEN_LOG_ERROR:
      return 3; /* ASL_LEVEL_ERR / kCFLogLevelError */

    case TEN_LOG_FATAL:
      return 0; /* ASL_LEVEL_EMERG / kCFLogLevelEmergency */

    default:
      TEN_ASSERT(0, "Bad log level");
      return 0; /* ASL_LEVEL_EMERG / kCFLogLevelEmergency */
  }
}

void out_nslog_cb(const ten_log_message_t *msg, void *arg) {
  VAR_UNUSED(arg);

  *msg->buf_content_end = 0;
  CFLog(apple_lvl(msg->level), CFSTR("%s"), msg->tag_start);
}

#endif
