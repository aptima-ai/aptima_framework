//
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#pragma once

#include "aptima_runtime/aptima_config.h"

#include <msgpack/pack.h>
#include <msgpack/unpack.h>

#include "aptima_utils/value/value_kv.h"

aptima_RUNTIME_PRIVATE_API bool aptima_msgpack_value_deserialize(
    aptima_value_t *value, msgpack_unpacker *unpacker, msgpack_unpacked *unpacked);

aptima_RUNTIME_PRIVATE_API bool aptima_msgpack_value_deserialize_inplace(
    aptima_value_t *value, msgpack_unpacker *unpacker, msgpack_unpacked *unpacked);

aptima_RUNTIME_PRIVATE_API aptima_value_t *
aptima_msgpack_create_value_through_deserialization(msgpack_unpacker *unpacker,
                                                 msgpack_unpacked *unpacked);

aptima_RUNTIME_PRIVATE_API aptima_value_kv_t *
aptima_msgpack_create_value_kv_through_deserialization(msgpack_unpacker *unpacker,
                                                    msgpack_unpacked *unpacked);

aptima_RUNTIME_PRIVATE_API void aptima_msgpack_value_serialize(aptima_value_t *value,
                                                         msgpack_packer *pck);

aptima_RUNTIME_PRIVATE_API void aptima_msgpack_value_kv_serialize(
    aptima_value_kv_t *kv, msgpack_packer *pck);
