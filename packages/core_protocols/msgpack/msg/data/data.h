//
// This file is part of the TEN Framework project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <msgpack/pack.h>
#include <msgpack/unpack.h>

#include "ten_utils/lib/error.h"
#include "ten_utils/lib/smart_ptr.h"

TEN_RUNTIME_PRIVATE_API bool ten_msgpack_data_serialize(ten_shared_ptr_t *self,
                                                        msgpack_packer *pck,
                                                        ten_error_t *err);

TEN_RUNTIME_PRIVATE_API bool ten_msgpack_data_deserialize(
    ten_shared_ptr_t *self, msgpack_unpacker *unpacker,
    msgpack_unpacked *unpacked);
