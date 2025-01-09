//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

#include "aptima_utils/lib/signature.h"

#define aptima_LOGV(...)                                                       \
  do {                                                                      \
    aptima_log_log_formatted(&aptima_global_log, aptima_LOG_LEVEL_VERBOSE, __func__, \
                          __FILE__, __LINE__, __VA_ARGS__);                 \
  } while (0)

#define aptima_LOGD(...)                                                     \
  do {                                                                    \
    aptima_log_log_formatted(&aptima_global_log, aptima_LOG_LEVEL_DEBUG, __func__, \
                          __FILE__, __LINE__, __VA_ARGS__);               \
  } while (0)

#define aptima_LOGI(...)                                                    \
  do {                                                                   \
    aptima_log_log_formatted(&aptima_global_log, aptima_LOG_LEVEL_INFO, __func__, \
                          __FILE__, __LINE__, __VA_ARGS__);              \
  } while (0)

#define aptima_LOGW(...)                                                    \
  do {                                                                   \
    aptima_log_log_formatted(&aptima_global_log, aptima_LOG_LEVEL_WARN, __func__, \
                          __FILE__, __LINE__, __VA_ARGS__);              \
  } while (0)

#define aptima_LOGE(...)                                                     \
  do {                                                                    \
    aptima_log_log_formatted(&aptima_global_log, aptima_LOG_LEVEL_ERROR, __func__, \
                          __FILE__, __LINE__, __VA_ARGS__);               \
  } while (0)

#define aptima_LOGF(...)                                                     \
  do {                                                                    \
    aptima_log_log_formatted(&aptima_global_log, aptima_LOG_LEVEL_FATAL, __func__, \
                          __FILE__, __LINE__, __VA_ARGS__);               \
  } while (0)

#define aptima_LOGV_AUX(log, ...)                                            \
  do {                                                                    \
    aptima_log_log_formatted(log, aptima_LOG_LEVEL_VERBOSE, __func__, __FILE__, \
                          __LINE__, __VA_ARGS__);                         \
  } while (0)

#define aptima_LOGD_AUX(log, ...)                                          \
  do {                                                                  \
    aptima_log_log_formatted(log, aptima_LOG_LEVEL_DEBUG, __func__, __FILE__, \
                          __LINE__, __VA_ARGS__);                       \
  } while (0)

#define aptima_LOGI_AUX(log, ...)                                         \
  do {                                                                 \
    aptima_log_log_formatted(log, aptima_LOG_LEVEL_INFO, __func__, __FILE__, \
                          __LINE__, __VA_ARGS__);                      \
  } while (0)

#define aptima_LOGW_AUX(log, ...)                                         \
  do {                                                                 \
    aptima_log_log_formatted(log, aptima_LOG_LEVEL_WARN, __func__, __FILE__, \
                          __LINE__, __VA_ARGS__);                      \
  } while (0)

#define aptima_LOGE_AUX(log, ...)                                          \
  do {                                                                  \
    aptima_log_log_formatted(log, aptima_LOG_LEVEL_ERROR, __func__, __FILE__, \
                          __LINE__, __VA_ARGS__);                       \
  } while (0)

#define aptima_LOGF_AUX(log, ...)                                          \
  do {                                                                  \
    aptima_log_log_formatted(log, aptima_LOG_LEVEL_FATAL, __func__, __FILE__, \
                          __LINE__, __VA_ARGS__);                       \
  } while (0)

typedef enum aptima_LOG_LEVEL {
  aptima_LOG_LEVEL_INVALID,

  aptima_LOG_LEVEL_VERBOSE,
  aptima_LOG_LEVEL_DEBUG,
  aptima_LOG_LEVEL_INFO,
  aptima_LOG_LEVEL_WARN,
  aptima_LOG_LEVEL_ERROR,
  aptima_LOG_LEVEL_FATAL,
} aptima_LOG_LEVEL;

typedef struct aptima_string_t aptima_string_t;

typedef void (*aptima_log_output_func_t)(aptima_string_t *msg, void *user_data);
typedef void (*aptima_log_close_func_t)(void *user_data);
typedef void (*aptima_log_formatter_func_t)(aptima_string_t *buf, aptima_LOG_LEVEL level,
                                         const char *func_name,
                                         size_t func_name_len,
                                         const char *file_name,
                                         size_t file_name_len, size_t line_no,
                                         const char *msg, size_t msg_len);

typedef struct aptima_log_output_t {
  aptima_log_output_func_t output_cb;
  aptima_log_close_func_t close_cb;
  void *user_data;
} aptima_log_output_t;

typedef struct aptima_log_formatter_t {
  aptima_log_formatter_func_t format_cb;
  void *user_data;  // In case the formatter needs any user data
} aptima_log_formatter_t;

typedef struct aptima_log_t {
  aptima_signature_t signature;

  aptima_LOG_LEVEL output_level;
  aptima_log_output_t output;

  aptima_log_formatter_t formatter;
} aptima_log_t;

aptima_UTILS_API aptima_log_t aptima_global_log;

aptima_UTILS_API void aptima_log_log_formatted(aptima_log_t *self, aptima_LOG_LEVEL level,
                                         const char *func_name,
                                         const char *file_name, size_t line_no,
                                         const char *fmt, ...);
