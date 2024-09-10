//
// This file is part of the TEN Framework project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <stddef.h>

#include "core_protocols/msgpack/msg/field/field_info.h"
#include "core_protocols/msgpack/msg/msg.h"
#include "core_protocols/msgpack/msg/video_frame/field/buf.h"
#include "core_protocols/msgpack/msg/video_frame/field/height.h"
#include "core_protocols/msgpack/msg/video_frame/field/pixel_fmt.h"
#include "core_protocols/msgpack/msg/video_frame/field/timestamp.h"
#include "core_protocols/msgpack/msg/video_frame/field/width.h"
#include "include_internal/ten_runtime/msg/video_frame/field/field.h"

#ifdef __cplusplus
  #error \
      "This file contains C99 array designated initializer, and Visual Studio C++ compiler can only support up to C89 by default, so we enable this checking to prevent any wrong inclusion of this file."
#endif

static const ten_protocol_msgpack_msg_field_info_t
    ten_video_frame_fields_info[] = {
        [TEN_VIDEO_FRAME_FIELD_MSGHDR] =
            {
                .serialize = ten_msgpack_msghdr_serialize,
                .deserialize = ten_msgpack_msghdr_deserialize,
            },
        [TEN_VIDEO_FRAME_FIELD_PIXEL_FMT] =
            {
                .serialize = ten_msgpack_video_frame_pixel_fmt_serialize,
                .deserialize = ten_msgpack_video_frame_pixel_fmt_deserialize,
            },
        [TEN_VIDEO_FRAME_FIELD_TIMESTAMP] =
            {
                .serialize = ten_msgpack_video_frame_timestamp_serialize,
                .deserialize = ten_msgpack_video_frame_timestamp_deserialize,
            },
        [TEN_VIDEO_FRAME_FIELD_WIDTH] =
            {
                .serialize = ten_msgpack_video_frame_width_serialize,
                .deserialize = ten_msgpack_video_frame_width_deserialize,
            },
        [TEN_VIDEO_FRAME_FIELD_HEIGHT] =
            {
                .serialize = ten_msgpack_video_frame_height_serialize,
                .deserialize = ten_msgpack_video_frame_height_deserialize,
            },
        [TEN_VIDEO_FRAME_FIELD_BUF] =
            {
                .serialize = ten_msgpack_video_frame_buf_serialize,
                .deserialize = ten_msgpack_video_frame_buf_deserialize,
            },
        [TEN_VIDEO_FRAME_FIELD_LAST] = {0},
};

static const size_t ten_video_frame_fields_info_size =
    sizeof(ten_video_frame_fields_info) /
    sizeof(ten_video_frame_fields_info[0]);
