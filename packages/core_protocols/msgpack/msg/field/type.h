//
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <msgpack/pack.h>

typedef struct ten_msg_t ten_msg_t;

TEN_RUNTIME_PRIVATE_API void ten_msgpack_msg_type_serialize(
    ten_msg_t *self, msgpack_packer *pck);
