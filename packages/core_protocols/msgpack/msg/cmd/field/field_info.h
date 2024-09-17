//
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <stddef.h>

#include "core_protocols/msgpack/msg/cmd/field/cmd_id.h"
#include "core_protocols/msgpack/msg/cmd/field/seq_id.h"
#include "core_protocols/msgpack/msg/field/field_info.h"
#include "core_protocols/msgpack/msg/msg.h"
#include "include_internal/ten_runtime/msg/cmd_base/field/field.h"

#ifdef __cplusplus
#error \
    "This file contains C99 array designated initializer, and Visual Studio C++ compiler can only support up to C89 by default, so we enable this checking to prevent any wrong inclusion of this file."
#endif

static const ten_protocol_msgpack_msg_field_info_t ten_cmd_base_fields_info[] =
    {
        [TEN_CMD_BASE_FIELD_MSGHDR] =
            {
                .serialize = ten_msgpack_msghdr_serialize,
                .deserialize = ten_msgpack_msghdr_deserialize,
            },
        [TEN_CMD_BASE_FIELD_CMD_ID] =
            {
                .serialize = ten_msgpack_cmd_id_serialize,
                .deserialize = ten_msgpack_cmd_id_deserialize,
            },
        [TEN_CMD_BASE_FIELD_SEQ_ID] =
            {
                .serialize = ten_msgpack_cmd_seq_id_serialize,
                .deserialize = ten_msgpack_cmd_seq_id_deserialize,
            },
        [TEN_CMD_BASE_FIELD_ORIGINAL_CONNECTION] =
            {
                .serialize = NULL,
                .deserialize = NULL,
            },
        [TEN_CMD_BASE_FIELD_LAST] = {0},
};

static const size_t ten_cmd_base_fields_info_size =
    sizeof(ten_cmd_base_fields_info) / sizeof(ten_cmd_base_fields_info[0]);
