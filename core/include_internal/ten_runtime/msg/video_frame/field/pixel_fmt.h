//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <stdbool.h>

#include "ten_runtime/msg/video_frame/video_frame.h"
#include "ten_utils/container/list.h"
#include "ten_utils/lib/json.h"

typedef struct ten_msg_t ten_msg_t;
typedef struct ten_error_t ten_error_t;

TEN_RUNTIME_PRIVATE_API bool ten_video_frame_put_pixel_fmt_to_json(
    ten_msg_t *self, ten_json_t *json, ten_error_t *err);

TEN_RUNTIME_PRIVATE_API bool ten_video_frame_get_pixel_fmt_from_json(
    ten_msg_t *self, ten_json_t *json, ten_error_t *err);

TEN_RUNTIME_PRIVATE_API void ten_video_frame_copy_pixel_fmt(
    ten_msg_t *self, ten_msg_t *src, ten_list_t *excluded_field_ids);

TEN_RUNTIME_PRIVATE_API const char *ten_video_frame_pixel_fmt_to_string(
    TEN_PIXEL_FMT pixel_fmt);

TEN_RUNTIME_PRIVATE_API TEN_PIXEL_FMT
ten_video_frame_pixel_fmt_from_string(const char *pixel_fmt_str);
