//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_runtime/aptima_config.h"

#include <stdbool.h>

#include "aptima_utils/lib/error.h"
#include "aptima_utils/lib/smart_ptr.h"

typedef struct aptima_env_t aptima_env_t;
typedef struct aptima_extension_t aptima_extension_t;

typedef void (*aptima_env_msg_result_handler_func_t)(
    aptima_env_t *aptima_env, aptima_shared_ptr_t *cmd_result,
    void *cmd_result_handler_user_data, aptima_error_t *err);

typedef bool (*aptima_env_send_cmd_func_t)(
    aptima_env_t *self, aptima_shared_ptr_t *cmd,
    aptima_env_msg_result_handler_func_t result_handler,
    void *result_handler_user_data, aptima_error_t *err);

aptima_RUNTIME_API bool aptima_env_send_cmd(
    aptima_env_t *self, aptima_shared_ptr_t *cmd,
    aptima_env_msg_result_handler_func_t result_handler,
    void *result_handler_user_data, aptima_error_t *err);

aptima_RUNTIME_API bool aptima_env_send_cmd_ex(
    aptima_env_t *self, aptima_shared_ptr_t *cmd,
    aptima_env_msg_result_handler_func_t result_handler,
    void *result_handler_user_data, aptima_error_t *err);

aptima_RUNTIME_API bool aptima_env_send_data(
    aptima_env_t *self, aptima_shared_ptr_t *data,
    aptima_env_msg_result_handler_func_t result_handler,
    void *result_handler_user_data, aptima_error_t *err);

aptima_RUNTIME_API bool aptima_env_send_video_frame(
    aptima_env_t *self, aptima_shared_ptr_t *frame,
    aptima_env_msg_result_handler_func_t result_handler,
    void *result_handler_user_data, aptima_error_t *err);

aptima_RUNTIME_API bool aptima_env_send_audio_frame(
    aptima_env_t *self, aptima_shared_ptr_t *frame,
    aptima_env_msg_result_handler_func_t result_handler,
    void *result_handler_user_data, aptima_error_t *err);
