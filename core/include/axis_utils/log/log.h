//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

#include "axis_utils/lib/signature.h"

#define axis_LOGV(...)                                                       \
  do {                                                                      \
    axis_log_log_formatted(&axis_global_log, axis_LOG_LEVEL_VERBOSE, __func__, \
                          __FILE__, __LINE__, __VA_ARGS__);                 \
  } while (0)

#define axis_LOGD(...)                                                     \
  do {                                                                    \
    axis_log_log_formatted(&axis_global_log, axis_LOG_LEVEL_DEBUG, __func__, \
                          __FILE__, __LINE__, __VA_ARGS__);               \
  } while (0)

#define axis_LOGI(...)                                                    \
  do {                                                                   \
    axis_log_log_formatted(&axis_global_log, axis_LOG_LEVEL_INFO, __func__, \
                          __FILE__, __LINE__, __VA_ARGS__);              \
  } while (0)

#define axis_LOGW(...)                                                    \
  do {                                                                   \
    axis_log_log_formatted(&axis_global_log, axis_LOG_LEVEL_WARN, __func__, \
                          __FILE__, __LINE__, __VA_ARGS__);              \
  } while (0)

#define axis_LOGE(...)                                                     \
  do {                                                                    \
    axis_log_log_formatted(&axis_global_log, axis_LOG_LEVEL_ERROR, __func__, \
                          __FILE__, __LINE__, __VA_ARGS__);               \
  } while (0)

#define axis_LOGF(...)                                                     \
  do {                                                                    \
    axis_log_log_formatted(&axis_global_log, axis_LOG_LEVEL_FATAL, __func__, \
                          __FILE__, __LINE__, __VA_ARGS__);               \
  } while (0)

#define axis_LOGV_AUX(log, ...)                                            \
  do {                                                                    \
    axis_log_log_formatted(log, axis_LOG_LEVEL_VERBOSE, __func__, __FILE__, \
                          __LINE__, __VA_ARGS__);                         \
  } while (0)

#define axis_LOGD_AUX(log, ...)                                          \
  do {                                                                  \
    axis_log_log_formatted(log, axis_LOG_LEVEL_DEBUG, __func__, __FILE__, \
                          __LINE__, __VA_ARGS__);                       \
  } while (0)

#define axis_LOGI_AUX(log, ...)                                         \
  do {                                                                 \
    axis_log_log_formatted(log, axis_LOG_LEVEL_INFO, __func__, __FILE__, \
                          __LINE__, __VA_ARGS__);                      \
  } while (0)

#define axis_LOGW_AUX(log, ...)                                         \
  do {                                                                 \
    axis_log_log_formatted(log, axis_LOG_LEVEL_WARN, __func__, __FILE__, \
                          __LINE__, __VA_ARGS__);                      \
  } while (0)

#define axis_LOGE_AUX(log, ...)                                          \
  do {                                                                  \
    axis_log_log_formatted(log, axis_LOG_LEVEL_ERROR, __func__, __FILE__, \
                          __LINE__, __VA_ARGS__);                       \
  } while (0)

#define axis_LOGF_AUX(log, ...)                                          \
  do {                                                                  \
    axis_log_log_formatted(log, axis_LOG_LEVEL_FATAL, __func__, __FILE__, \
                          __LINE__, __VA_ARGS__);                       \
  } while (0)

typedef enum axis_LOG_LEVEL {
  axis_LOG_LEVEL_INVALID,

  axis_LOG_LEVEL_VERBOSE,
  axis_LOG_LEVEL_DEBUG,
  axis_LOG_LEVEL_INFO,
  axis_LOG_LEVEL_WARN,
  axis_LOG_LEVEL_ERROR,
  axis_LOG_LEVEL_FATAL,
} axis_LOG_LEVEL;

typedef struct axis_string_t axis_string_t;

typedef void (*axis_log_output_func_t)(axis_string_t *msg, void *user_data);
typedef void (*axis_log_close_func_t)(void *user_data);
typedef void (*axis_log_formatter_func_t)(axis_string_t *buf, axis_LOG_LEVEL level,
                                         const char *func_name,
                                         size_t func_name_len,
                                         const char *file_name,
                                         size_t file_name_len, size_t line_no,
                                         const char *msg, size_t msg_len);

typedef struct axis_log_output_t {
  axis_log_output_func_t output_cb;
  axis_log_close_func_t close_cb;
  void *user_data;
} axis_log_output_t;

typedef struct axis_log_formatter_t {
  axis_log_formatter_func_t format_cb;
  void *user_data;  // In case the formatter needs any user data
} axis_log_formatter_t;

typedef struct axis_log_t {
  axis_signature_t signature;

  axis_LOG_LEVEL output_level;
  axis_log_output_t output;

  axis_log_formatter_t formatter;
} axis_log_t;

axis_UTILS_API axis_log_t axis_global_log;

axis_UTILS_API void axis_log_log_formatted(axis_log_t *self, axis_LOG_LEVEL level,
                                         const char *func_name,
                                         const char *file_name, size_t line_no,
                                         const char *fmt, ...);
