//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_runtime/aptima_config.h"

#include <stdbool.h>

#include "aptima_utils/lib/smart_ptr.h"

typedef struct aptima_extension_t aptima_extension_t;
typedef struct aptima_env_t aptima_env_t;
typedef struct aptima_metadata_info_t aptima_metadata_info_t;

typedef void (*aptima_extension_on_configure_func_t)(aptima_extension_t *self,
                                                  aptima_env_t *aptima_env);

typedef void (*aptima_extension_on_init_func_t)(aptima_extension_t *self,
                                             aptima_env_t *aptima_env);

typedef void (*aptima_extension_on_start_func_t)(aptima_extension_t *self,
                                              aptima_env_t *aptima_env);

typedef void (*aptima_extension_on_stop_func_t)(aptima_extension_t *self,
                                             aptima_env_t *aptima_env);

typedef void (*aptima_extension_on_deinit_func_t)(aptima_extension_t *self,
                                               aptima_env_t *aptima_env);

typedef void (*aptima_extension_on_cmd_func_t)(aptima_extension_t *self,
                                            aptima_env_t *aptima_env,
                                            aptima_shared_ptr_t *cmd);

typedef void (*aptima_extension_on_data_func_t)(aptima_extension_t *self,
                                             aptima_env_t *aptima_env,
                                             aptima_shared_ptr_t *data);

typedef void (*aptima_extension_on_audio_frame_func_t)(aptima_extension_t *self,
                                                    aptima_env_t *aptima_env,
                                                    aptima_shared_ptr_t *frame);

typedef void (*aptima_extension_on_video_frame_func_t)(aptima_extension_t *self,
                                                    aptima_env_t *aptima_env,
                                                    aptima_shared_ptr_t *frame);

aptima_RUNTIME_API bool aptima_extension_check_integrity(aptima_extension_t *self,
                                                   bool check_thread);

aptima_RUNTIME_API aptima_extension_t *aptima_extension_create(
    const char *name, aptima_extension_on_configure_func_t on_configure,
    aptima_extension_on_init_func_t on_init,
    aptima_extension_on_start_func_t on_start,
    aptima_extension_on_stop_func_t on_stop,
    aptima_extension_on_deinit_func_t on_deinit,
    aptima_extension_on_cmd_func_t on_cmd, aptima_extension_on_data_func_t on_data,
    aptima_extension_on_audio_frame_func_t on_audio_frame,
    aptima_extension_on_video_frame_func_t on_video_frame, void *user_data);

aptima_RUNTIME_API void aptima_extension_destroy(aptima_extension_t *self);

aptima_RUNTIME_API aptima_env_t *aptima_extension_get_aptima_env(aptima_extension_t *self);
