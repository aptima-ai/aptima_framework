//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

#include "axis_utils/container/list.h"

typedef struct axis_msg_t axis_msg_t;
typedef struct axis_error_t axis_error_t;

axis_RUNTIME_PRIVATE_API void axis_audio_frame_copy_channel_layout(
    axis_msg_t *self, axis_msg_t *src, axis_list_t *excluded_field_ids);
