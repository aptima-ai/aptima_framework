//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/axis_config.h"

#include "include_internal/axis_utils/log/level.h"

#include <assert.h>

char axis_log_level_char(const axis_LOG_LEVEL level) {
  switch (level) {
    case axis_LOG_LEVEL_VERBOSE:
      return 'V';
    case axis_LOG_LEVEL_DEBUG:
      return 'D';
    case axis_LOG_LEVEL_INFO:
      return 'I';
    case axis_LOG_LEVEL_WARN:
      return 'W';
    case axis_LOG_LEVEL_ERROR:
      return 'E';
    case axis_LOG_LEVEL_FATAL:
      return 'F';
    default:
      return '?';
  }
}

void axis_log_set_output_level(axis_log_t *self, axis_LOG_LEVEL level) {
  assert(self && "Invalid argument.");

  self->output_level = level;
}
