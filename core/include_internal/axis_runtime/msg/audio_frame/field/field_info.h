//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include <stddef.h>

#include "include_internal/axis_runtime/msg/audio_frame/field/buf.h"
#include "include_internal/axis_runtime/msg/audio_frame/field/bytes_per_sample.h"
#include "include_internal/axis_runtime/msg/audio_frame/field/data_fmt.h"
#include "include_internal/axis_runtime/msg/audio_frame/field/field.h"
#include "include_internal/axis_runtime/msg/audio_frame/field/line_size.h"
#include "include_internal/axis_runtime/msg/audio_frame/field/number_of_channel.h"
#include "include_internal/axis_runtime/msg/audio_frame/field/sample_rate.h"
#include "include_internal/axis_runtime/msg/audio_frame/field/samples_per_channel.h"
#include "include_internal/axis_runtime/msg/audio_frame/field/timestamp.h"
#include "include_internal/axis_runtime/msg/field/field_info.h"
#include "include_internal/axis_runtime/msg/msg.h"

#ifdef __cplusplus
#error \
    "This file contains C99 array designated initializer, and Visual Studio C++ compiler can only support up to C89 by default, so we enable this checking to prevent any wrong inclusion of this file."
#endif

static const axis_msg_field_info_t axis_audio_frame_fields_info[] = {
    [axis_AUDIO_FRAME_FIELD_MSGHDR] =
        {
            .field_name = NULL,
            .field_id = -1,
            .copy_field = axis_raw_msg_copy_field,
            .process_field = axis_raw_msg_process_field,
        },
    [axis_AUDIO_FRAME_FIELD_TIMESTAMP] =
        {
            .field_name = NULL,
            .field_id = axis_MSG_FIELD_LAST + axis_AUDIO_FRAME_FIELD_TIMESTAMP,
            .copy_field = axis_audio_frame_copy_timestamp,
            .process_field = axis_audio_frame_process_timestamp,
        },
    [axis_AUDIO_FRAME_FIELD_SAMPLE_RATE] =
        {
            .field_name = NULL,
            .field_id = axis_MSG_FIELD_LAST + axis_AUDIO_FRAME_FIELD_SAMPLE_RATE,
            .copy_field = axis_audio_frame_copy_sample_rate,
            .process_field = axis_audio_frame_process_sample_rate,
        },
    [axis_AUDIO_FRAME_FIELD_BYTES_PER_SAMPLE] =
        {
            .field_name = NULL,
            .field_id =
                axis_MSG_FIELD_LAST + axis_AUDIO_FRAME_FIELD_BYTES_PER_SAMPLE,
            .copy_field = axis_audio_frame_copy_bytes_per_sample,
            .process_field = axis_audio_frame_process_bytes_per_sample,
        },
    [axis_AUDIO_FRAME_FIELD_SAMPLES_PER_CHANNEL] =
        {
            .field_name = NULL,
            .field_id =
                axis_MSG_FIELD_LAST + axis_AUDIO_FRAME_FIELD_SAMPLES_PER_CHANNEL,
            .copy_field = axis_audio_frame_copy_samples_per_channel,
            .process_field = axis_audio_frame_process_samples_per_channel,
        },
    [axis_AUDIO_FRAME_FIELD_NUMBER_OF_CHANNEL] =
        {
            .field_name = NULL,
            .field_id =
                axis_MSG_FIELD_LAST + axis_AUDIO_FRAME_FIELD_NUMBER_OF_CHANNEL,
            .copy_field = axis_audio_frame_copy_number_of_channel,
            .process_field = axis_audio_frame_process_number_of_channel,
        },
    [axis_AUDIO_FRAME_FIELD_DATA_FMT] =
        {
            .field_name = NULL,
            .field_id = axis_MSG_FIELD_LAST + axis_AUDIO_FRAME_FIELD_DATA_FMT,
            .copy_field = axis_audio_frame_copy_data_fmt,
            .process_field = axis_audio_frame_process_data_fmt,
        },
    [axis_AUDIO_FRAME_FIELD_BUF] =
        {
            .field_name = NULL,
            .field_id = axis_MSG_FIELD_LAST + axis_AUDIO_FRAME_FIELD_BUF,
            .copy_field = NULL,
            .process_field = axis_audio_frame_process_buf,
        },
    [axis_AUDIO_FRAME_FIELD_LINE_SIZE] =
        {
            .field_name = NULL,
            .field_id = axis_MSG_FIELD_LAST + axis_AUDIO_FRAME_FIELD_LINE_SIZE,
            .copy_field = axis_audio_frame_copy_line_size,
            .process_field = axis_audio_frame_process_line_size,
        },
    [axis_AUDIO_FRAME_FIELD_LAST] = {0},
};

static const size_t axis_audio_frame_fields_info_size =
    sizeof(axis_audio_frame_fields_info) /
    sizeof(axis_audio_frame_fields_info[0]);
