//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/ten_config.h"

#include "include_internal/ten_utils/log/level.h"

#include <assert.h>

char ten_log_level_char(const TEN_LOG_LEVEL level) {
  switch (level) {
    case TEN_LOG_VERBOSE:
      return 'V';
    case TEN_LOG_DEBUG:
      return 'D';
    case TEN_LOG_INFO:
      return 'I';
    case TEN_LOG_WARN:
      return 'W';
    case TEN_LOG_ERROR:
      return 'E';
    case TEN_LOG_FATAL:
      return 'F';
    default:
      return '?';
  }
}

void ten_log_set_output_level(const TEN_LOG_LEVEL level) {
  ten_log_global_output_level = level;
}
