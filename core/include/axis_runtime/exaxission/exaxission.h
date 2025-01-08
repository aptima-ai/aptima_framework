//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

#include "axis_utils/lib/smart_ptr.h"

typedef struct axis_extension_t axis_extension_t;
typedef struct axis_env_t axis_env_t;
typedef struct axis_metadata_info_t axis_metadata_info_t;

typedef void (*axis_extension_on_configure_func_t)(axis_extension_t *self,
                                                  axis_env_t *axis_env);

typedef void (*axis_extension_on_init_func_t)(axis_extension_t *self,
                                             axis_env_t *axis_env);

typedef void (*axis_extension_on_start_func_t)(axis_extension_t *self,
                                              axis_env_t *axis_env);

typedef void (*axis_extension_on_stop_func_t)(axis_extension_t *self,
                                             axis_env_t *axis_env);

typedef void (*axis_extension_on_deinit_func_t)(axis_extension_t *self,
                                               axis_env_t *axis_env);

typedef void (*axis_extension_on_cmd_func_t)(axis_extension_t *self,
                                            axis_env_t *axis_env,
                                            axis_shared_ptr_t *cmd);

typedef void (*axis_extension_on_data_func_t)(axis_extension_t *self,
                                             axis_env_t *axis_env,
                                             axis_shared_ptr_t *data);

typedef void (*axis_extension_on_audio_frame_func_t)(axis_extension_t *self,
                                                    axis_env_t *axis_env,
                                                    axis_shared_ptr_t *frame);

typedef void (*axis_extension_on_video_frame_func_t)(axis_extension_t *self,
                                                    axis_env_t *axis_env,
                                                    axis_shared_ptr_t *frame);

axis_RUNTIME_API bool axis_extension_check_integrity(axis_extension_t *self,
                                                   bool check_thread);

axis_RUNTIME_API axis_extension_t *axis_extension_create(
    const char *name, axis_extension_on_configure_func_t on_configure,
    axis_extension_on_init_func_t on_init,
    axis_extension_on_start_func_t on_start,
    axis_extension_on_stop_func_t on_stop,
    axis_extension_on_deinit_func_t on_deinit,
    axis_extension_on_cmd_func_t on_cmd, axis_extension_on_data_func_t on_data,
    axis_extension_on_audio_frame_func_t on_audio_frame,
    axis_extension_on_video_frame_func_t on_video_frame, void *user_data);

axis_RUNTIME_API void axis_extension_destroy(axis_extension_t *self);

axis_RUNTIME_API axis_env_t *axis_extension_get_axis_env(axis_extension_t *self);
