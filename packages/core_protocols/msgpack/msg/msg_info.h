//
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "core_protocols/msgpack/msg/audio_frame/audio_frame.h"
#include "core_protocols/msgpack/msg/cmd/close_app/cmd.h"
#include "core_protocols/msgpack/msg/cmd/cmd.h"
#include "core_protocols/msgpack/msg/cmd/custom/cmd.h"
#include "core_protocols/msgpack/msg/cmd_result/cmd.h"
#include "core_protocols/msgpack/msg/data/data.h"
#include "core_protocols/msgpack/msg/video_frame/video_frame.h"
#include "ten_runtime/ten.h"
#include "ten_utils/lib/smart_ptr.h"

typedef bool (*ten_msg_serialize_func_t)(ten_shared_ptr_t *msg,
                                         msgpack_packer *packer,
                                         ten_error_t *err);

typedef bool (*ten_msg_deserialize_func_t)(ten_shared_ptr_t *msg,
                                           msgpack_unpacker *unpacker,
                                           msgpack_unpacked *unpacked);

typedef struct ten_protocol_msgpack_msg_info_t {
  ten_msg_serialize_func_t serialize;
  ten_msg_deserialize_func_t deserialize;
} ten_protocol_msgpack_msg_info_t;

#ifdef __cplusplus
#error \
    "This file contains C99 array designated initializer, and Visual Studio C++ compiler can only support up to C89 by default, so we enable this checking to prevent any wrong inclusion of this file."
#endif

static const ten_protocol_msgpack_msg_info_t ten_msg_info[] = {
    [TEN_MSG_TYPE_INVALID] =
        {
            .serialize = NULL,
            .deserialize = NULL,
        },
    [TEN_MSG_TYPE_CMD_START_GRAPH] =
        {
            .serialize = ten_msgpack_cmd_serialize_through_json,
            .deserialize = NULL,
        },
    [TEN_MSG_TYPE_CMD_RESULT] =
        {
            .serialize = ten_msgpack_cmd_result_serialize,
            .deserialize = ten_msgpack_cmd_result_deserialize,
        },
    [TEN_MSG_TYPE_CMD_TIMEOUT] =
        {
            .serialize = NULL,
            .deserialize = NULL,
        },
    [TEN_MSG_TYPE_CMD_TIMER] =
        {
            .serialize = NULL,
            .deserialize = NULL,
        },
    [TEN_MSG_TYPE_CMD_CLOSE_APP] =
        {
            .serialize = ten_msgpack_cmd_close_app_serialize,
            .deserialize = ten_msgpack_cmd_close_app_deserialize,
        },
    [TEN_MSG_TYPE_CMD_STOP_GRAPH] =
        {
            .serialize = NULL,
            .deserialize = NULL,
        },
    [TEN_MSG_TYPE_CMD] =
        {
            .serialize = ten_msgpack_cmd_custom_serialize,
            .deserialize = ten_msgpack_cmd_custom_deserialize,
        },
    [TEN_MSG_TYPE_DATA] =
        {
            .serialize = ten_msgpack_data_serialize,
            .deserialize = ten_msgpack_data_deserialize,
        },
    [TEN_MSG_TYPE_AUDIO_FRAME] =
        {
            .serialize = ten_msgpack_audio_frame_serialize,
            .deserialize = ten_msgpack_audio_frame_deserialize,
        },
    [TEN_MSG_TYPE_VIDEO_FRAME] =
        {
            .serialize = ten_msgpack_video_frame_serialize,
            .deserialize = ten_msgpack_video_frame_deserialize,
        },
    [TEN_MSG_TYPE_LAST] = {0},
};

static const size_t ten_msg_info_size =
    sizeof(ten_msg_info) / sizeof(ten_msg_info[0]);
