//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <stdbool.h>

#include "ten_utils/lib/error.h"
#include "ten_utils/lib/smart_ptr.h"

typedef struct ten_env_t ten_env_t;
typedef struct ten_extension_t ten_extension_t;

typedef void (*ten_env_msg_result_handler_func_t)(
    ten_env_t *ten_env, ten_shared_ptr_t *cmd_result,
    void *cmd_result_handler_user_data, ten_error_t *err);

typedef bool (*ten_env_send_cmd_func_t)(
    ten_env_t *self, ten_shared_ptr_t *cmd,
    ten_env_msg_result_handler_func_t result_handler,
    void *result_handler_user_data, ten_error_t *err);

TEN_RUNTIME_API bool ten_env_send_cmd(
    ten_env_t *self, ten_shared_ptr_t *cmd,
    ten_env_msg_result_handler_func_t result_handler,
    void *result_handler_user_data, ten_error_t *err);

TEN_RUNTIME_API bool ten_env_send_cmd_ex(
    ten_env_t *self, ten_shared_ptr_t *cmd,
    ten_env_msg_result_handler_func_t result_handler,
    void *result_handler_user_data, ten_error_t *err);

TEN_RUNTIME_API bool ten_env_send_data(
    ten_env_t *self, ten_shared_ptr_t *data,
    ten_env_msg_result_handler_func_t result_handler,
    void *result_handler_user_data, ten_error_t *err);

TEN_RUNTIME_API bool ten_env_send_video_frame(
    ten_env_t *self, ten_shared_ptr_t *frame,
    ten_env_msg_result_handler_func_t result_handler,
    void *result_handler_user_data, ten_error_t *err);

TEN_RUNTIME_API bool ten_env_send_audio_frame(
    ten_env_t *self, ten_shared_ptr_t *frame,
    ten_env_msg_result_handler_func_t result_handler,
    void *result_handler_user_data, ten_error_t *err);
