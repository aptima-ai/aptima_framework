//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

#include "axis_utils/lib/error.h"
#include "axis_utils/lib/smart_ptr.h"

typedef struct axis_env_t axis_env_t;
typedef struct axis_extension_t axis_extension_t;

typedef void (*axis_env_msg_result_handler_func_t)(
    axis_env_t *axis_env, axis_shared_ptr_t *cmd_result,
    void *cmd_result_handler_user_data, axis_error_t *err);

typedef bool (*axis_env_send_cmd_func_t)(
    axis_env_t *self, axis_shared_ptr_t *cmd,
    axis_env_msg_result_handler_func_t result_handler,
    void *result_handler_user_data, axis_error_t *err);

axis_RUNTIME_API bool axis_env_send_cmd(
    axis_env_t *self, axis_shared_ptr_t *cmd,
    axis_env_msg_result_handler_func_t result_handler,
    void *result_handler_user_data, axis_error_t *err);

axis_RUNTIME_API bool axis_env_send_cmd_ex(
    axis_env_t *self, axis_shared_ptr_t *cmd,
    axis_env_msg_result_handler_func_t result_handler,
    void *result_handler_user_data, axis_error_t *err);

axis_RUNTIME_API bool axis_env_send_data(
    axis_env_t *self, axis_shared_ptr_t *data,
    axis_env_msg_result_handler_func_t result_handler,
    void *result_handler_user_data, axis_error_t *err);

axis_RUNTIME_API bool axis_env_send_video_frame(
    axis_env_t *self, axis_shared_ptr_t *frame,
    axis_env_msg_result_handler_func_t result_handler,
    void *result_handler_user_data, axis_error_t *err);

axis_RUNTIME_API bool axis_env_send_audio_frame(
    axis_env_t *self, axis_shared_ptr_t *frame,
    axis_env_msg_result_handler_func_t result_handler,
    void *result_handler_user_data, axis_error_t *err);
