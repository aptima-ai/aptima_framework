//
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <stddef.h>

#include "core_protocols/msgpack/msg/field/dest.h"
#include "core_protocols/msgpack/msg/field/name.h"
#include "core_protocols/msgpack/msg/field/properties.h"
#include "core_protocols/msgpack/msg/field/src.h"
#include "core_protocols/msgpack/msg/field/type.h"
#include "include_internal/ten_runtime/msg/field/field.h"

#ifdef __cplusplus
#error \
    "This file contains C99 array designated initializer, and Visual Studio C++ compiler can only support up to C89 by default, so we enable this checking to prevent any wrong inclusion of this file."
#endif

typedef void (*ten_msg_field_serialize_func_t)(ten_msg_t *self,
                                               msgpack_packer *pck);

typedef bool (*ten_msg_field_deserialize_func_t)(ten_msg_t *self,
                                                 msgpack_unpacker *unpacker,
                                                 msgpack_unpacked *unpacked);

typedef struct ten_protocol_msgpack_msg_field_info_t {
  ten_msg_field_serialize_func_t serialize;
  ten_msg_field_deserialize_func_t deserialize;
} ten_protocol_msgpack_msg_field_info_t;

static const ten_protocol_msgpack_msg_field_info_t
    ten_protocol_msgpack_msg_fields_info[] = {
        [TEN_MSG_FIELD_TYPE] =
            {
                .serialize = ten_msgpack_msg_type_serialize,
                .deserialize = NULL,
            },
        [TEN_MSG_FIELD_NAME] =
            {
                .serialize = ten_msgpack_msg_name_serialize,
                .deserialize = ten_msgpack_msg_name_deserialize,
            },
        [TEN_MSG_FIELD_SRC] =
            {
                .serialize = ten_msgpack_msg_src_serialize,
                .deserialize = ten_msgpack_msg_src_deserialize,
            },
        [TEN_MSG_FIELD_DEST] =
            {
                .serialize = ten_msgpack_msg_dest_serialize,
                .deserialize = ten_msgpack_msg_dest_deserialize,
            },
        [TEN_MSG_FIELD_PROPERTIES] =
            {
                .serialize = ten_msgpack_msg_properties_serialize,
                .deserialize = ten_msgpack_msg_properties_deserialize,
            },
        [TEN_MSG_FIELD_LAST] = {0},
};

static const size_t ten_protocol_msgpack_msg_fields_info_size =
    sizeof(ten_protocol_msgpack_msg_fields_info) /
    sizeof(ten_protocol_msgpack_msg_fields_info[0]);
