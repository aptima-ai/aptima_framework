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
    case TEN_LOG_LEVEL_VERBOSE:
      return 'V';
    case TEN_LOG_LEVEL_DEBUG:
      return 'D';
    case TEN_LOG_LEVEL_INFO:
      return 'I';
    case TEN_LOG_LEVEL_WARN:
      return 'W';
    case TEN_LOG_LEVEL_ERROR:
      return 'E';
    case TEN_LOG_LEVEL_FATAL:
      return 'F';
    default:
      return '?';
  }
}

void ten_log_set_output_level(ten_log_t *self, TEN_LOG_LEVEL level) {
  assert(self && "Invalid argument.");

  self->output_level = level;
}
