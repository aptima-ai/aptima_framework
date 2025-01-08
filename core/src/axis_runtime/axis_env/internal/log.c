//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_runtime/axis_env/internal/log.h"

#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "include_internal/axis_utils/log/log.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_utils/lib/string.h"

static void axis_env_log_internal(axis_env_t *self, axis_LOG_LEVEL level,
                                 const char *func_name, const char *file_name,
                                 size_t line_no, const char *msg,
                                 bool check_thread) {
  axis_ASSERT(self && axis_env_check_integrity(self, check_thread),
             "Should not happen.");

  axis_string_t final_msg;
  axis_string_init_formatted(
      &final_msg, "[%s] %s",
      axis_env_get_attached_instance_name(self, check_thread), msg);

  axis_log_log(&axis_global_log, level, func_name, file_name, line_no,
              axis_string_get_raw_str(&final_msg));

  axis_string_deinit(&final_msg);
}

// TODO(Wei): This function is currently specifically designed for the addon
// because the addon currently does not have a main thread, so it's unable to
// check thread safety. Once the main thread for the addon is determined in the
// future, these hacks made specifically for the addon can be completely
// removed, and comprehensive thread safety checking can be implemented.
void axis_env_log_without_check_thread(axis_env_t *self, axis_LOG_LEVEL level,
                                      const char *func_name,
                                      const char *file_name, size_t line_no,
                                      const char *msg) {
  axis_env_log_internal(self, level, func_name, file_name, line_no, msg, false);
}

void axis_env_log(axis_env_t *self, axis_LOG_LEVEL level, const char *func_name,
                 const char *file_name, size_t line_no, const char *msg) {
  axis_env_log_internal(self, level, func_name, file_name, line_no, msg, true);
}

static void axis_env_log_with_size_formatted_internal(
    axis_env_t *self, axis_LOG_LEVEL level, const char *func_name,
    size_t func_name_len, const char *file_name, size_t file_name_len,
    size_t line_no, bool check_thread, const char *fmt, va_list ap) {
  axis_ASSERT(self && axis_env_check_integrity(self, check_thread),
             "Should not happen.");

  axis_string_t final_msg;
  axis_string_init_formatted(
      &final_msg, "[%s] ",
      axis_env_get_attached_instance_name(self, check_thread));

  axis_string_append_from_va_list(&final_msg, fmt, ap);

  axis_log_log_with_size(&axis_global_log, level, func_name, func_name_len,
                        file_name, file_name_len, line_no,
                        axis_string_get_raw_str(&final_msg),
                        axis_string_len(&final_msg));

  axis_string_deinit(&final_msg);
}

// TODO(Wei): This function is currently specifically designed for the addon
// because the addon currently does not have a main thread, so it's unable to
// check thread safety. Once the main thread for the addon is determined in the
// future, these hacks made specifically for the addon can be completely
// removed, and comprehensive thread safety checking can be implemented.
void axis_env_log_with_size_formatted_without_check_thread(
    axis_env_t *self, axis_LOG_LEVEL level, const char *func_name,
    size_t func_name_len, const char *file_name, size_t file_name_len,
    size_t line_no, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  axis_env_log_with_size_formatted_internal(
      self, level, func_name, func_name_len, file_name, file_name_len, line_no,
      false, fmt, ap);

  va_end(ap);
}

void axis_env_log_with_size_formatted(axis_env_t *self, axis_LOG_LEVEL level,
                                     const char *func_name,
                                     size_t func_name_len,
                                     const char *file_name,
                                     size_t file_name_len, size_t line_no,
                                     const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  axis_env_log_with_size_formatted_internal(
      self, level, func_name, func_name_len, file_name, file_name_len, line_no,
      true, fmt, ap);

  va_end(ap);
}

void axis_env_log_formatted(axis_env_t *self, axis_LOG_LEVEL level,
                           const char *func_name, const char *file_name,
                           size_t line_no, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  axis_env_log_with_size_formatted_internal(
      self, level, func_name, strlen(func_name), file_name, strlen(file_name),
      line_no, true, fmt, ap);

  va_end(ap);
}
