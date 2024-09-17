//
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <stdbool.h>

#include "include_internal/ten_runtime/msg/msg.h"
#include "msgpack.h"
#include "ten_utils/lib/smart_ptr.h"

TEN_RUNTIME_PRIVATE_API bool ten_msgpack_cmd_serialize_through_json(
    ten_shared_ptr_t *self, msgpack_packer *pck, ten_error_t *err);

TEN_RUNTIME_PRIVATE_API ten_shared_ptr_t *
ten_msgpack_cmd_deserialize_through_json(ten_shared_ptr_t *msg);

TEN_RUNTIME_PRIVATE_API void ten_msgpack_cmd_base_hdr_serialize(
    ten_msg_t *self, msgpack_packer *pck);

TEN_RUNTIME_PRIVATE_API bool ten_msgpack_cmd_base_hdr_deserialize(
    ten_msg_t *self, msgpack_unpacker *unpacker, msgpack_unpacked *unpacked);
