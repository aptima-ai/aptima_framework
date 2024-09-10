//
// This file is part of the TEN Framework project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <assert.h>
#include <stdbool.h>

#include "core_protocols/msgpack/common/common.h"
#include "include_internal/ten_runtime/common/loc.h"
#include "ten_utils/lib/string.h"

TEN_RUNTIME_PRIVATE_API void ten_msgpack_loc_serialize(ten_loc_t *self,
                                                       msgpack_packer *pck);

TEN_RUNTIME_PRIVATE_API void ten_msgpack_loc_deserialize(
    ten_loc_t *self, msgpack_unpacker *unpacker, msgpack_unpacked *unpacked);

TEN_RUNTIME_PRIVATE_API void ten_msgpack_loc_list_serialize(
    ten_list_t *self, msgpack_packer *pck);

TEN_RUNTIME_PRIVATE_API ten_list_t ten_msgpack_loc_list_deserialize(
    msgpack_unpacker *unpacker, msgpack_unpacked *unpacked);
