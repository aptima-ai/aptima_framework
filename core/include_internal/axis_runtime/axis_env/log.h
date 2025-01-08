//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_utils/log/log.h"

#define axis_ENV_LOG_VERBOSE_INTERNAL(axis_env, ...)                            \
  do {                                                                        \
    axis_env_log_formatted(axis_env, axis_LOG_LEVEL_VERBOSE, __func__, __FILE__, \
                          __LINE__, __VA_ARGS__);                             \
  } while (0)

#define axis_ENV_LOG_DEBUG_INTERNAL(axis_env, ...)                            \
  do {                                                                      \
    axis_env_log_formatted(axis_env, axis_LOG_LEVEL_DEBUG, __func__, __FILE__, \
                          __LINE__, __VA_ARGS__);                           \
  } while (0)

#define axis_ENV_LOG_INFO_INTERNAL(axis_env, ...)                            \
  do {                                                                     \
    axis_env_log_formatted(axis_env, axis_LOG_LEVEL_INFO, __func__, __FILE__, \
                          __LINE__, __VA_ARGS__);                          \
  } while (0)

#define axis_ENV_LOG_WARN_INTERNAL(axis_env, ...)                            \
  do {                                                                     \
    axis_env_log_formatted(axis_env, axis_LOG_LEVEL_WARN, __func__, __FILE__, \
                          __LINE__, __VA_ARGS__);                          \
  } while (0)

#define axis_ENV_LOG_ERROR_INTERNAL(axis_env, ...)                            \
  do {                                                                      \
    axis_env_log_formatted(axis_env, axis_LOG_LEVEL_ERROR, __func__, __FILE__, \
                          __LINE__, __VA_ARGS__);                           \
  } while (0)

#define axis_ENV_LOG_FATAL_INTERNAL(axis_env, ...)                            \
  do {                                                                      \
    axis_env_log_formatted(axis_env, axis_LOG_LEVEL_FATAL, __func__, __FILE__, \
                          __LINE__, __VA_ARGS__);                           \
  } while (0)

typedef struct axis_env_t axis_env_t;

axis_RUNTIME_API void axis_env_log_with_size_formatted_without_check_thread(
    axis_env_t *self, axis_LOG_LEVEL level, const char *func_name,
    size_t func_name_len, const char *file_name, size_t file_name_len,
    size_t line_no, const char *fmt, ...);

axis_RUNTIME_API void axis_env_log_with_size_formatted(
    axis_env_t *self, axis_LOG_LEVEL level, const char *func_name,
    size_t func_name_len, const char *file_name, size_t file_name_len,
    size_t line_no, const char *fmt, ...);

axis_RUNTIME_PRIVATE_API void axis_env_log_formatted(
    axis_env_t *self, axis_LOG_LEVEL level, const char *func_name,
    const char *file_name, size_t line_no, const char *fmt, ...);

axis_RUNTIME_API void axis_env_log_without_check_thread(
    axis_env_t *self, axis_LOG_LEVEL level, const char *func_name,
    const char *file_name, size_t line_no, const char *msg);
