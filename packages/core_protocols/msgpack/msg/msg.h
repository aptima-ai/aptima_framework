//
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdint.h>

#include "core_protocols/msgpack/common/parser.h"
#include "include_internal/axis_runtime/msg/msg.h"

typedef struct axis_msg_deserialize_info_t {
  msgpack_unpacker *unpacker;
  msgpack_unpacked *unpacked;
} axis_msg_deserialize_info_t;

axis_RUNTIME_PRIVATE_API axis_msg_deserialize_info_t *
axis_msg_deserialize_info_create(msgpack_unpacker *unpacker,
                                msgpack_unpacked *unpacked);

axis_RUNTIME_PRIVATE_API void axis_msg_deserialize_info_destroy(
    axis_msg_deserialize_info_t *self);

axis_RUNTIME_PRIVATE_API bool axis_msgpack_serialize_msg(axis_shared_ptr_t *self,
                                                       msgpack_packer *pck,
                                                       axis_error_t *err);

axis_RUNTIME_PRIVATE_API axis_shared_ptr_t *axis_msgpack_deserialize_msg(
    msgpack_unpacker *unpacker, msgpack_unpacked *unpacked);

axis_RUNTIME_API axis_buf_t axis_msgpack_serialize_msgs(axis_list_t *msgs,
                                                     axis_error_t *err);

axis_RUNTIME_API void axis_msgpack_deserialize_msgs(axis_msgpack_parser_t *parser,
                                                  axis_buf_t input_buf,
                                                  axis_list_t *result_msgs);
