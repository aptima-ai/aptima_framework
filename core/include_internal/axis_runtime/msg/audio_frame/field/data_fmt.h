//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

#include "include_internal/axis_runtime/msg/loop_fields.h"
#include "axis_runtime/msg/audio_frame/audio_frame.h"
#include "axis_utils/container/list.h"

typedef struct axis_msg_t axis_msg_t;
typedef struct axis_error_t axis_error_t;

axis_RUNTIME_PRIVATE_API void axis_audio_frame_copy_data_fmt(
    axis_msg_t *self, axis_msg_t *src, axis_list_t *excluded_field_ids);

axis_RUNTIME_PRIVATE_API const char *axis_audio_frame_data_fmt_to_string(
    axis_AUDIO_FRAME_DATA_FMT data_fmt);

axis_RUNTIME_PRIVATE_API axis_AUDIO_FRAME_DATA_FMT
axis_audio_frame_data_fmt_from_string(const char *data_fmt_str);

axis_RUNTIME_PRIVATE_API bool axis_audio_frame_process_data_fmt(
    axis_msg_t *self, axis_raw_msg_process_one_field_func_t cb, void *user_data,
    axis_error_t *err);
