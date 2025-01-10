//
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_runtime/msg/msg.h"

typedef struct axis_msg_t axis_msg_t;
typedef struct msgpack_packer msgpack_packer;
typedef struct msgpack_unpacker msgpack_unpacker;
typedef struct msgpack_unpacked msgpack_unpacked;

axis_RUNTIME_PRIVATE_API void axis_msgpack_msg_type_serialize(
    axis_msg_t *self, msgpack_packer *pck);

axis_RUNTIME_PRIVATE_API axis_MSG_TYPE axis_msgpack_deserialize_msg_type(
    msgpack_unpacker *unpacker, msgpack_unpacked *unpacked);
