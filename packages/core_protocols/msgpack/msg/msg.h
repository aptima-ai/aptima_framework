//
// This file is part of the TEN Framework project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <stdint.h>

#include "core_protocols/msgpack/common/parser.h"
#include "include_internal/ten_runtime/msg/msg.h"

TEN_RUNTIME_PRIVATE_API void ten_msgpack_msghdr_serialize(ten_msg_t *self,
                                                          msgpack_packer *pck);

TEN_RUNTIME_PRIVATE_API bool ten_msgpack_msghdr_deserialize(
    ten_msg_t *self, msgpack_unpacker *unpacker, msgpack_unpacked *unpacked);

TEN_RUNTIME_PRIVATE_API bool ten_msgpack_msg_serialize(ten_shared_ptr_t *self,
                                                       msgpack_packer *pck,
                                                       ten_error_t *err);

TEN_RUNTIME_PRIVATE_API bool ten_msgpack_msg_deserialize(
    ten_shared_ptr_t *self, msgpack_unpacker *unpacker,
    msgpack_unpacked *unpacked);

TEN_RUNTIME_PRIVATE_API TEN_MSG_TYPE ten_msgpack_deserialize_msg_type(
    msgpack_unpacker *unpacker, msgpack_unpacked *unpacked);

TEN_RUNTIME_PRIVATE_API ten_shared_ptr_t *ten_msgpack_deserialize_msg_internal(
    msgpack_unpacker *unpacker, msgpack_unpacked *unpacked);

TEN_RUNTIME_API ten_buf_t ten_msgpack_serialize_msg(ten_list_t *msgs,
                                                    ten_error_t *err);

TEN_RUNTIME_API void ten_msgpack_deserialize_msg(ten_msgpack_parser_t *parser,
                                                 ten_buf_t input_buf,
                                                 ten_list_t *result_msgs);
