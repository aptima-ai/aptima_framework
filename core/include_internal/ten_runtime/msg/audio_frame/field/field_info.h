//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include <stddef.h>

#include "include_internal/ten_runtime/msg/audio_frame/field/bytes_per_sample.h"
#include "include_internal/ten_runtime/msg/audio_frame/field/data_fmt.h"
#include "include_internal/ten_runtime/msg/audio_frame/field/field.h"
#include "include_internal/ten_runtime/msg/audio_frame/field/line_size.h"
#include "include_internal/ten_runtime/msg/audio_frame/field/number_of_channel.h"
#include "include_internal/ten_runtime/msg/audio_frame/field/sample_rate.h"
#include "include_internal/ten_runtime/msg/audio_frame/field/samples_per_channel.h"
#include "include_internal/ten_runtime/msg/audio_frame/field/timestamp.h"
#include "include_internal/ten_runtime/msg/field/field_info.h"
#include "include_internal/ten_runtime/msg/msg.h"

#ifdef __cplusplus
  #error \
      "This file contains C99 array designated initializer, and Visual Studio C++ compiler can only support up to C89 by default, so we enable this checking to prevent any wrong inclusion of this file."
#endif

static const ten_msg_field_info_t ten_audio_frame_fields_info[] = {
    [TEN_AUDIO_FRAME_FIELD_MSGHDR] =
        {
            .field_name = NULL,
            .field_id = -1,
            .put_field_to_json = ten_raw_msg_put_field_to_json,
            .get_field_from_json = ten_raw_msg_get_field_from_json,
            .copy_field = ten_raw_msg_copy_field,
        },
    [TEN_AUDIO_FRAME_FIELD_TIMESTAMP] =
        {
            .field_name = NULL,
            .field_id = TEN_MSG_FIELD_LAST + TEN_AUDIO_FRAME_FIELD_TIMESTAMP,
            .put_field_to_json = ten_audio_frame_put_timestamp_to_json,
            .get_field_from_json = ten_audio_frame_get_timestamp_from_json,
            .copy_field = ten_audio_frame_copy_timestamp,
        },
    [TEN_AUDIO_FRAME_FIELD_SAMPLE_RATE] =
        {
            .field_name = NULL,
            .field_id = TEN_MSG_FIELD_LAST + TEN_AUDIO_FRAME_FIELD_SAMPLE_RATE,
            .put_field_to_json = ten_audio_frame_put_sample_rate_to_json,
            .get_field_from_json = ten_audio_frame_get_sample_rate_from_json,
            .copy_field = ten_audio_frame_copy_sample_rate,
        },
    [TEN_AUDIO_FRAME_FIELD_BYTES_PER_SAMPLE] =
        {
            .field_name = NULL,
            .field_id =
                TEN_MSG_FIELD_LAST + TEN_AUDIO_FRAME_FIELD_BYTES_PER_SAMPLE,
            .put_field_to_json = ten_audio_frame_put_bytes_per_sample_to_json,
            .get_field_from_json =
                ten_audio_frame_get_bytes_per_sample_from_json,
            .copy_field = ten_audio_frame_copy_bytes_per_sample,
        },
    [TEN_AUDIO_FRAME_FIELD_SAMPLES_PER_CHANNEL] =
        {
            .field_name = NULL,
            .field_id =
                TEN_MSG_FIELD_LAST + TEN_AUDIO_FRAME_FIELD_SAMPLES_PER_CHANNEL,
            .put_field_to_json =
                ten_audio_frame_put_samples_per_channel_to_json,
            .get_field_from_json =
                ten_audio_frame_get_samples_per_channel_from_json,
            .copy_field = ten_audio_frame_copy_samples_per_channel,
        },
    [TEN_AUDIO_FRAME_FIELD_NUMBER_OF_CHANNEL] =
        {
            .field_name = NULL,
            .field_id =
                TEN_MSG_FIELD_LAST + TEN_AUDIO_FRAME_FIELD_NUMBER_OF_CHANNEL,
            .put_field_to_json = ten_audio_frame_put_number_of_channel_to_json,
            .get_field_from_json =
                ten_audio_frame_get_number_of_channel_from_json,
            .copy_field = ten_audio_frame_copy_number_of_channel,
        },
    [TEN_AUDIO_FRAME_FIELD_DATA_FMT] =
        {
            .field_name = NULL,
            .field_id = TEN_MSG_FIELD_LAST + TEN_AUDIO_FRAME_FIELD_DATA_FMT,
            .put_field_to_json = ten_audio_frame_put_data_fmt_to_json,
            .get_field_from_json = ten_audio_frame_get_data_fmt_from_json,
            .copy_field = ten_audio_frame_copy_data_fmt,
        },
    [TEN_AUDIO_FRAME_FIELD_DATA] =
        {
            .field_name = NULL,
            .field_id = TEN_MSG_FIELD_LAST + TEN_AUDIO_FRAME_FIELD_DATA,

            // It is not possible to get/put the binary content of a memory
            // buffer from/into JSON (unless you use base64). If needed, clients
            // must use explicit get/set properties from/to the buffer; clients
            // cannot use JSON for this.
            .put_field_to_json = NULL,
            .get_field_from_json = NULL,

            .copy_field = NULL,
        },
    [TEN_AUDIO_FRAME_FIELD_LINE_SIZE] =
        {
            .field_name = NULL,
            .field_id = TEN_MSG_FIELD_LAST + TEN_AUDIO_FRAME_FIELD_LINE_SIZE,
            .put_field_to_json = ten_audio_frame_put_line_size_to_json,
            .get_field_from_json = ten_audio_frame_get_line_size_from_json,
            .copy_field = ten_audio_frame_copy_line_size,
        },
    [TEN_AUDIO_FRAME_FIELD_LAST] = {0},
};

static const size_t ten_audio_frame_fields_info_size =
    sizeof(ten_audio_frame_fields_info) /
    sizeof(ten_audio_frame_fields_info[0]);
