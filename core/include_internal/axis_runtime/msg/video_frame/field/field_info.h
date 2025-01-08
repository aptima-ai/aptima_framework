//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include <stddef.h>

#include "include_internal/axis_runtime/msg/field/field_info.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/msg/video_frame/field/buf.h"
#include "include_internal/axis_runtime/msg/video_frame/field/field.h"
#include "include_internal/axis_runtime/msg/video_frame/field/height.h"
#include "include_internal/axis_runtime/msg/video_frame/field/pixel_fmt.h"
#include "include_internal/axis_runtime/msg/video_frame/field/timestamp.h"
#include "include_internal/axis_runtime/msg/video_frame/field/width.h"

#ifdef __cplusplus
#error \
    "This file contains C99 array designated initializer, and Visual Studio C++ compiler can only support up to C89 by default, so we enable this checking to prevent any wrong inclusion of this file."
#endif

static const axis_msg_field_info_t axis_video_frame_fields_info[] = {
    [axis_VIDEO_FRAME_FIELD_MSGHDR] =
        {
            .field_name = NULL,
            .field_id = -1,
            .copy_field = axis_raw_msg_copy_field,
            .process_field = axis_raw_msg_process_field,
        },
    [axis_VIDEO_FRAME_FIELD_PIXEL_FMT] =
        {
            .field_name = NULL,
            .field_id = axis_MSG_FIELD_LAST + axis_VIDEO_FRAME_FIELD_PIXEL_FMT,
            .copy_field = axis_video_frame_copy_pixel_fmt,
            .process_field = axis_video_frame_process_pixel_fmt,
        },
    [axis_VIDEO_FRAME_FIELD_TIMESTAMP] =
        {
            .field_name = NULL,
            .field_id = axis_MSG_FIELD_LAST + axis_VIDEO_FRAME_FIELD_TIMESTAMP,
            .copy_field = axis_video_frame_copy_timestamp,
            .process_field = axis_video_frame_process_timestamp,
        },
    [axis_VIDEO_FRAME_FIELD_WIDTH] =
        {
            .field_name = NULL,
            .field_id = axis_MSG_FIELD_LAST + axis_VIDEO_FRAME_FIELD_WIDTH,
            .copy_field = axis_video_frame_copy_width,
            .process_field = axis_video_frame_process_width,
        },
    [axis_VIDEO_FRAME_FIELD_HEIGHT] =
        {
            .field_name = NULL,
            .field_id = axis_MSG_FIELD_LAST + axis_VIDEO_FRAME_FIELD_HEIGHT,
            .copy_field = axis_video_frame_copy_height,
            .process_field = axis_video_frame_process_height,
        },
    [axis_VIDEO_FRAME_FIELD_BUF] =
        {
            .field_name = NULL,
            .field_id = axis_MSG_FIELD_LAST + axis_VIDEO_FRAME_FIELD_BUF,
            .copy_field = NULL,
            .process_field = axis_video_frame_process_buf,
        },
    [axis_VIDEO_FRAME_FIELD_LAST] = {0},
};

static const size_t axis_video_frame_fields_info_size =
    sizeof(axis_video_frame_fields_info) /
    sizeof(axis_video_frame_fields_info[0]);
