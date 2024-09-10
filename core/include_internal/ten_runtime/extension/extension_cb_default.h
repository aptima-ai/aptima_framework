//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "ten_runtime/extension/extension.h"

typedef struct ten_env_t ten_env_t;

TEN_RUNTIME_PRIVATE_API void ten_extension_on_start_default(
    ten_extension_t *self, ten_env_t *ten_env);

TEN_RUNTIME_PRIVATE_API void ten_extension_on_stop_default(
    ten_extension_t *self, ten_env_t *ten_env);

TEN_RUNTIME_PRIVATE_API void ten_extension_on_deinit_default(
    ten_extension_t *self, ten_env_t *ten_env);

TEN_RUNTIME_PRIVATE_API void ten_extension_on_cmd_default(
    ten_extension_t *self, ten_env_t *ten_env, ten_shared_ptr_t *cmd);

TEN_RUNTIME_PRIVATE_API void ten_extension_on_data_default(
    ten_extension_t *self, ten_env_t *ten_env, ten_shared_ptr_t *data);

TEN_RUNTIME_PRIVATE_API void ten_extension_on_audio_frame_default(
    ten_extension_t *self, ten_env_t *ten_env, ten_shared_ptr_t *frame);

TEN_RUNTIME_PRIVATE_API void ten_extension_on_video_frame_default(
    ten_extension_t *self, ten_env_t *ten_env, ten_shared_ptr_t *frame);