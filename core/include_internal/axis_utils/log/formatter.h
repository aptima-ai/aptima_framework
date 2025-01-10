//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include "axis_utils/log/log.h"

axis_UTILS_PRIVATE_API void axis_log_default_formatter(
    axis_string_t *buf, axis_LOG_LEVEL level, const char *func_name,
    size_t func_name_len, const char *file_name, size_t file_name_len,
    size_t line_no, const char *msg, size_t msg_len);

axis_UTILS_PRIVATE_API void axis_log_colored_formatter(
    axis_string_t *buf, axis_LOG_LEVEL level, const char *func_name,
    size_t func_name_len, const char *file_name, size_t file_name_len,
    size_t line_no, const char *msg, size_t msg_len);

axis_UTILS_PRIVATE_API void axis_log_set_formatter(
    axis_log_t *self, axis_log_formatter_func_t format_cb, void *user_data);
