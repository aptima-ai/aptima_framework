//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "ten_utils/log/log.h"

typedef struct ten_env_t ten_env_t;

TEN_RUNTIME_API void ten_env_log_with_size_formatted_without_check_thread(
    ten_env_t *self, TEN_LOG_LEVEL level, const char *func_name,
    size_t func_name_len, const char *file_name, size_t file_name_len,
    size_t line_no, const char *fmt, ...);

TEN_RUNTIME_API void ten_env_log_with_size_formatted(
    ten_env_t *self, TEN_LOG_LEVEL level, const char *func_name,
    size_t func_name_len, const char *file_name, size_t file_name_len,
    size_t line_no, const char *fmt, ...);

TEN_RUNTIME_API void ten_env_log_without_check_thread(
    ten_env_t *self, TEN_LOG_LEVEL level, const char *func_name,
    const char *file_name, size_t line_no, const char *msg);
