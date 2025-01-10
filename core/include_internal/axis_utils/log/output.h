//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stdbool.h>

/**
 * @brief String to put in the end of each log line (can be empty).
 */
#ifndef axis_LOG_EOL
#define axis_LOG_EOL "\n"
#endif

typedef struct axis_log_t axis_log_t;
typedef struct axis_string_t axis_string_t;

axis_UTILS_API void axis_log_set_output_to_stderr(axis_log_t *self);

axis_UTILS_PRIVATE_API void axis_log_output_to_file_cb(axis_string_t *msg,
                                                     void *user_data);

axis_UTILS_PRIVATE_API void axis_log_output_to_stderr_cb(axis_string_t *msg,
                                                       void *user_data);

axis_UTILS_PRIVATE_API void axis_log_set_output_to_file(axis_log_t *self,
                                                      const char *log_path);

axis_UTILS_PRIVATE_API void axis_log_output_to_file_deinit(axis_log_t *self);

axis_UTILS_PRIVATE_API bool axis_log_is_output_to_file(axis_log_t *self);
