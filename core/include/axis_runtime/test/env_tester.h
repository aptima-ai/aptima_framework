//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_utils/lib/error.h"
#include "axis_utils/lib/smart_ptr.h"

typedef struct axis_env_tester_t axis_env_tester_t;

axis_RUNTIME_API bool axis_env_tester_on_start_done(axis_env_tester_t *self,
                                                  axis_error_t *err);

typedef void (*axis_env_tester_cmd_result_handler_func_t)(
    axis_env_tester_t *self, axis_shared_ptr_t *cmd_result, void *user_data,
    axis_error_t *error);

typedef void (*axis_env_tester_error_handler_func_t)(axis_env_tester_t *self,
                                                    void *user_data,
                                                    axis_error_t *error);

axis_RUNTIME_API bool axis_env_tester_send_cmd(
    axis_env_tester_t *self, axis_shared_ptr_t *cmd,
    axis_env_tester_cmd_result_handler_func_t handler, void *user_data,
    axis_error_t *error);

axis_RUNTIME_API bool axis_env_tester_send_data(
    axis_env_tester_t *self, axis_shared_ptr_t *data,
    axis_env_tester_error_handler_func_t handler, void *user_data,
    axis_error_t *error);

axis_RUNTIME_API bool axis_env_tester_send_audio_frame(
    axis_env_tester_t *self, axis_shared_ptr_t *audio_frame,
    axis_env_tester_error_handler_func_t handler, void *user_data,
    axis_error_t *error);

axis_RUNTIME_API bool axis_env_tester_send_video_frame(
    axis_env_tester_t *self, axis_shared_ptr_t *video_frame,
    axis_env_tester_error_handler_func_t handler, void *user_data,
    axis_error_t *error);

axis_RUNTIME_API bool axis_env_tester_return_result(
    axis_env_tester_t *self, axis_shared_ptr_t *result,
    axis_shared_ptr_t *target_cmd,
    axis_env_tester_error_handler_func_t error_handler, void *user_data,
    axis_error_t *error);

axis_RUNTIME_API bool axis_env_tester_stop_test(axis_env_tester_t *self,
                                              axis_error_t *error);
