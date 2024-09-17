//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <stdbool.h>

#include "ten_utils/container/list.h"
#include "ten_utils/lib/json.h"

typedef struct ten_msg_t ten_msg_t;
typedef struct ten_error_t ten_error_t;

TEN_RUNTIME_PRIVATE_API bool ten_video_frame_put_width_to_json(
    ten_msg_t *self, ten_json_t *json, ten_error_t *err);

TEN_RUNTIME_PRIVATE_API bool ten_video_frame_get_width_from_json(
    ten_msg_t *self, ten_json_t *json, ten_error_t *err);

TEN_RUNTIME_PRIVATE_API void ten_video_frame_copy_width(
    ten_msg_t *self, ten_msg_t *src, ten_list_t *excluded_field_ids);
