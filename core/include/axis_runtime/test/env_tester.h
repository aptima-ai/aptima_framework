//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_runtime/aptima_config.h"

#include "aptima_utils/lib/error.h"
#include "aptima_utils/lib/smart_ptr.h"

typedef struct aptima_env_tester_t aptima_env_tester_t;

aptima_RUNTIME_API bool aptima_env_tester_on_start_done(aptima_env_tester_t *self,
                                                  aptima_error_t *err);

typedef void (*aptima_env_tester_cmd_result_handler_func_t)(
    aptima_env_tester_t *self, aptima_shared_ptr_t *cmd_result, void *user_data,
    aptima_error_t *error);

typedef void (*aptima_env_tester_error_handler_func_t)(aptima_env_tester_t *self,
                                                    void *user_data,
                                                    aptima_error_t *error);

aptima_RUNTIME_API bool aptima_env_tester_send_cmd(
    aptima_env_tester_t *self, aptima_shared_ptr_t *cmd,
    aptima_env_tester_cmd_result_handler_func_t handler, void *user_data,
    aptima_error_t *error);

aptima_RUNTIME_API bool aptima_env_tester_send_data(
    aptima_env_tester_t *self, aptima_shared_ptr_t *data,
    aptima_env_tester_error_handler_func_t handler, void *user_data,
    aptima_error_t *error);

aptima_RUNTIME_API bool aptima_env_tester_send_audio_frame(
    aptima_env_tester_t *self, aptima_shared_ptr_t *audio_frame,
    aptima_env_tester_error_handler_func_t handler, void *user_data,
    aptima_error_t *error);

aptima_RUNTIME_API bool aptima_env_tester_send_video_frame(
    aptima_env_tester_t *self, aptima_shared_ptr_t *video_frame,
    aptima_env_tester_error_handler_func_t handler, void *user_data,
    aptima_error_t *error);

aptima_RUNTIME_API bool aptima_env_tester_return_result(
    aptima_env_tester_t *self, aptima_shared_ptr_t *result,
    aptima_shared_ptr_t *target_cmd,
    aptima_env_tester_error_handler_func_t error_handler, void *user_data,
    aptima_error_t *error);

aptima_RUNTIME_API bool aptima_env_tester_stop_test(aptima_env_tester_t *self,
                                              aptima_error_t *error);
