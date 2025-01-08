//
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <msgpack/pack.h>
#include <msgpack/unpack.h>

#include "axis_utils/value/value_kv.h"

axis_RUNTIME_PRIVATE_API bool axis_msgpack_value_deserialize(
    axis_value_t *value, msgpack_unpacker *unpacker, msgpack_unpacked *unpacked);

axis_RUNTIME_PRIVATE_API bool axis_msgpack_value_deserialize_inplace(
    axis_value_t *value, msgpack_unpacker *unpacker, msgpack_unpacked *unpacked);

axis_RUNTIME_PRIVATE_API axis_value_t *
axis_msgpack_create_value_through_deserialization(msgpack_unpacker *unpacker,
                                                 msgpack_unpacked *unpacked);

axis_RUNTIME_PRIVATE_API axis_value_kv_t *
axis_msgpack_create_value_kv_through_deserialization(msgpack_unpacker *unpacker,
                                                    msgpack_unpacked *unpacked);

axis_RUNTIME_PRIVATE_API void axis_msgpack_value_serialize(axis_value_t *value,
                                                         msgpack_packer *pck);

axis_RUNTIME_PRIVATE_API void axis_msgpack_value_kv_serialize(
    axis_value_kv_t *kv, msgpack_packer *pck);
