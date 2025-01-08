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

#include "axis_utils/log/log.h"

#define axis_LOG_SIGNATURE 0xC0EE0CE92149D61AU

typedef struct axis_string_t axis_string_t;

typedef void (*axis_log_output_func_t)(axis_string_t *msg, void *user_data);
typedef void (*axis_log_close_func_t)(void *user_data);

axis_UTILS_PRIVATE_API bool axis_log_check_integrity(axis_log_t *self);

axis_UTILS_API void axis_log_init(axis_log_t *self);

axis_UTILS_PRIVATE_API axis_log_t *axis_log_create(void);

axis_UTILS_API void axis_log_deinit(axis_log_t *self);

axis_UTILS_PRIVATE_API void axis_log_destroy(axis_log_t *self);

axis_UTILS_PRIVATE_API const char *filename(const char *path, size_t path_len,
                                           size_t *filename_len);

axis_UTILS_API void axis_log_log_with_size_from_va_list(
    axis_log_t *self, axis_LOG_LEVEL level, const char *func_name,
    size_t func_name_len, const char *file_name, size_t file_name_len,
    size_t line_no, const char *fmt, va_list ap);

axis_UTILS_PRIVATE_API void axis_log_log_from_va_list(
    axis_log_t *self, axis_LOG_LEVEL level, const char *func_name,
    const char *file_name, size_t line_no, const char *fmt, va_list ap);

axis_UTILS_API void axis_log_log(axis_log_t *self, axis_LOG_LEVEL level,
                               const char *func_name, const char *file_name,
                               size_t line_no, const char *msg);

axis_UTILS_API void axis_log_log_with_size(axis_log_t *self, axis_LOG_LEVEL level,
                                         const char *func_name,
                                         size_t func_name_len,
                                         const char *file_name,
                                         size_t file_name_len, size_t line_no,
                                         const char *msg, size_t msg_len);

axis_UTILS_API void axis_log_global_init(void);

axis_UTILS_API void axis_log_global_deinit(void);

axis_UTILS_API void axis_log_global_set_output_level(axis_LOG_LEVEL level);

axis_UTILS_API void axis_log_global_set_output_to_stderr(void);

axis_UTILS_API void axis_log_global_set_output_to_file(const char *log_path);
