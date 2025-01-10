//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/axis_config.h"

#include <inttypes.h>
#include <time.h>

#include "include_internal/axis_utils/log/level.h"
#include "include_internal/axis_utils/log/log.h"
#include "include_internal/axis_utils/log/pid.h"
#include "include_internal/axis_utils/log/termcolor.h"
#include "include_internal/axis_utils/log/time.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/log/log.h"

void axis_log_set_formatter(axis_log_t *self, axis_log_formatter_func_t format_cb,
                           void *user_data) {
  axis_ASSERT(self, "Invalid argument.");

  self->formatter.format_cb = format_cb;
  self->formatter.user_data = user_data;
}

// Default formatter (no colors).
void axis_log_default_formatter(axis_string_t *buf, axis_LOG_LEVEL level,
                               const char *func_name, size_t func_name_len,
                               const char *file_name, size_t file_name_len,
                               size_t line_no, const char *msg,
                               size_t msg_len) {
  struct tm time_info;
  size_t msec = 0;

  axis_log_get_time(&time_info, &msec);
  axis_log_add_time_string(buf, &time_info, msec);

  int64_t pid = 0;
  int64_t tid = 0;
  axis_log_get_pid_tid(&pid, &tid);

  axis_string_append_formatted(buf, " %d(%d) %c", pid, tid,
                              axis_log_level_char(level));

  if (func_name_len) {
    axis_string_append_formatted(buf, " %.*s", (int)func_name_len, func_name);
  }

  size_t actual_file_name_len = 0;
  const char *actual_file_name =
      filename(file_name, file_name_len, &actual_file_name_len);
  if (actual_file_name_len) {
    axis_string_append_formatted(buf, "@%.*s:%d", (int)actual_file_name_len,
                                actual_file_name, (int)line_no);
  }

  axis_string_append_formatted(buf, " %.*s", (int)msg_len, msg);
}

// Colored formatter.
void axis_log_colored_formatter(axis_string_t *buf, axis_LOG_LEVEL level,
                               const char *func_name, size_t func_name_len,
                               const char *file_name, size_t file_name_len,
                               size_t line_no, const char *msg,
                               size_t msg_len) {
  struct tm time_info;
  size_t msec = 0;
  axis_log_get_time(&time_info, &msec);
  axis_log_add_time_string(buf, &time_info, msec);

  int64_t pid = 0;
  int64_t tid = 0;
  axis_log_get_pid_tid(&pid, &tid);

  // Determine color based on log level.
  const char *level_color = NULL;
  switch (level) {
    case axis_LOG_LEVEL_FATAL:
    case axis_LOG_LEVEL_ERROR:
      level_color = axis_LOG_COLOR_RED;
      break;
    case axis_LOG_LEVEL_WARN:
      level_color = axis_LOG_COLOR_YELLOW;
      break;
    case axis_LOG_LEVEL_INFO:
      level_color = axis_LOG_COLOR_GREEN;
      break;
    case axis_LOG_LEVEL_DEBUG:
    case axis_LOG_LEVEL_VERBOSE:
      level_color = axis_LOG_COLOR_CYAN;
      break;
    default:
      level_color = axis_LOG_COLOR_WHITE;
      break;
  }

  axis_string_append_formatted(buf, " %" PRId64 "(%" PRId64 ") %s%c%s", pid, tid,
                              level_color, axis_log_level_char(level),
                              axis_LOG_COLOR_RESET);

  // Add color to function name.
  if (func_name_len) {
    axis_string_append_formatted(buf, " %s%.*s%s", axis_LOG_COLOR_MAGENTA,
                                (int)func_name_len, func_name,
                                axis_LOG_COLOR_RESET);
  }

  // Add color to file name and line number.
  size_t actual_file_name_len = 0;
  const char *actual_file_name =
      filename(file_name, file_name_len, &actual_file_name_len);
  if (actual_file_name_len) {
    axis_string_append_formatted(buf, "%s@%.*s:%zu%s", axis_LOG_COLOR_BLUE,
                                (int)actual_file_name_len, actual_file_name,
                                line_no, axis_LOG_COLOR_RESET);
  }

  // Add color to message.
  axis_string_append_formatted(buf, " %s%.*s%s", axis_LOG_COLOR_WHITE,
                              (int)msg_len, msg, axis_LOG_COLOR_RESET);
}
