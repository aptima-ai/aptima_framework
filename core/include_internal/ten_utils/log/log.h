//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_utils/ten_config.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

#include "ten_utils/log/log.h"

#define TEN_LOG_SIGNATURE 0xC0EE0CE92149D61AU

typedef struct ten_string_t ten_string_t;

typedef void (*ten_log_output_func_t)(ten_string_t *msg, void *user_data);
typedef void (*ten_log_close_func_t)(void *user_data);

TEN_UTILS_PRIVATE_API bool ten_log_check_integrity(ten_log_t *self);

TEN_UTILS_API void ten_log_init(ten_log_t *self);

TEN_UTILS_PRIVATE_API ten_log_t *ten_log_create(void);

TEN_UTILS_API void ten_log_deinit(ten_log_t *self);

TEN_UTILS_PRIVATE_API void ten_log_destroy(ten_log_t *self);

TEN_UTILS_PRIVATE_API const char *filename(const char *path, size_t path_len,
                                           size_t *filename_len);

TEN_UTILS_API void ten_log_log_with_size_from_va_list(
    ten_log_t *self, TEN_LOG_LEVEL level, const char *func_name,
    size_t func_name_len, const char *file_name, size_t file_name_len,
    size_t line_no, const char *fmt, va_list ap);

TEN_UTILS_PRIVATE_API void ten_log_log_from_va_list(
    ten_log_t *self, TEN_LOG_LEVEL level, const char *func_name,
    const char *file_name, size_t line_no, const char *fmt, va_list ap);

TEN_UTILS_API void ten_log_log(ten_log_t *self, TEN_LOG_LEVEL level,
                               const char *func_name, const char *file_name,
                               size_t line_no, const char *msg);

TEN_UTILS_API void ten_log_log_with_size(ten_log_t *self, TEN_LOG_LEVEL level,
                                         const char *func_name,
                                         size_t func_name_len,
                                         const char *file_name,
                                         size_t file_name_len, size_t line_no,
                                         const char *msg, size_t msg_len);

TEN_UTILS_API void ten_log_global_init(void);

TEN_UTILS_API void ten_log_global_deinit(void);

TEN_UTILS_API void ten_log_global_set_output_level(TEN_LOG_LEVEL level);

TEN_UTILS_API void ten_log_global_set_output_to_file(const char *log_path);
