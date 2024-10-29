//
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <stddef.h>

#include "core_protocols/msgpack/msg/audio_frame/field/bytes_per_sample.h"
#include "core_protocols/msgpack/msg/audio_frame/field/data.h"
#include "core_protocols/msgpack/msg/audio_frame/field/data_fmt.h"
#include "core_protocols/msgpack/msg/audio_frame/field/line_size.h"
#include "core_protocols/msgpack/msg/audio_frame/field/number_of_channel.h"
#include "core_protocols/msgpack/msg/audio_frame/field/sample_rate.h"
#include "core_protocols/msgpack/msg/audio_frame/field/samples_per_channel.h"
#include "core_protocols/msgpack/msg/audio_frame/field/timestamp.h"
#include "core_protocols/msgpack/msg/field/field_info.h"
#include "core_protocols/msgpack/msg/msg.h"
#include "include_internal/ten_runtime/msg/audio_frame/field/field.h"

#ifdef __cplusplus
#error \
    "This file contains C99 array designated initializer, and Visual Studio C++ compiler can only support up to C89 by default, so we enable this checking to prevent any wrong inclusion of this file."
#endif

static const ten_protocol_msgpack_msg_field_info_t
    ten_audio_frame_fields_info[] = {
        [TEN_AUDIO_FRAME_FIELD_MSGHDR] =
            {
                .serialize = ten_msgpack_msghdr_serialize,
                .deserialize = ten_msgpack_msghdr_deserialize,
            },
        [TEN_AUDIO_FRAME_FIELD_TIMESTAMP] =
            {
                .serialize = ten_msgpack_audio_frame_timestamp_serialize,
                .deserialize = ten_msgpack_audio_frame_timestamp_deserialize,
            },
        [TEN_AUDIO_FRAME_FIELD_SAMPLE_RATE] =
            {
                .serialize = ten_msgpack_audio_frame_sample_rate_serialize,
                .deserialize = ten_msgpack_audio_frame_sample_rate_deserialize,
            },
        [TEN_AUDIO_FRAME_FIELD_BYTES_PER_SAMPLE] =
            {
                .serialize = ten_msgpack_audio_frame_bytes_per_sample_serialize,
                .deserialize =
                    ten_msgpack_audio_frame_bytes_per_sample_deserialize,
            },
        [TEN_AUDIO_FRAME_FIELD_SAMPLES_PER_CHANNEL] =
            {
                .serialize =
                    ten_msgpack_audio_frame_samples_per_channel_serialize,
                .deserialize =
                    ten_msgpack_audio_frame_samples_per_channel_deserialize,
            },
        [TEN_AUDIO_FRAME_FIELD_NUMBER_OF_CHANNEL] =
            {
                .serialize =
                    ten_msgpack_audio_frame_number_of_channel_serialize,
                .deserialize =
                    ten_msgpack_audio_frame_number_of_channel_deserialize,
            },
        [TEN_AUDIO_FRAME_FIELD_DATA_FMT] =
            {
                .serialize = ten_msgpack_audio_frame_data_fmt_serialize,
                .deserialize = ten_msgpack_audio_frame_data_fmt_deserialize,
            },
        [TEN_AUDIO_FRAME_FIELD_BUF] =
            {
                .serialize = ten_msgpack_audio_frame_data_serialize,
                .deserialize = ten_msgpack_audio_frame_data_deserialize,
            },
        [TEN_AUDIO_FRAME_FIELD_LINE_SIZE] =
            {
                .serialize = ten_msgpack_audio_frame_line_size_serialize,
                .deserialize = ten_msgpack_audio_frame_line_size_deserialize,
            },
        [TEN_AUDIO_FRAME_FIELD_LAST] = {0},
};

static const size_t ten_audio_frame_fields_info_size =
    sizeof(ten_audio_frame_fields_info) /
    sizeof(ten_audio_frame_fields_info[0]);
