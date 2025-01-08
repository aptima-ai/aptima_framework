//
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "msgpack.h"
#include "axis_utils/lib/smart_ptr.h"

typedef struct axis_msgpack_parser_t {
  msgpack_unpacker unpacker;
  msgpack_unpacked unpacked;
} axis_msgpack_parser_t;

axis_RUNTIME_PRIVATE_API void axis_msgpack_parser_init(
    axis_msgpack_parser_t *self);

axis_RUNTIME_PRIVATE_API void axis_msgpack_parser_deinit(
    axis_msgpack_parser_t *self);

axis_RUNTIME_PRIVATE_API void axis_msgpack_parser_feed_data(
    axis_msgpack_parser_t *self, const char *data, size_t data_size);

axis_RUNTIME_PRIVATE_API axis_shared_ptr_t *axis_msgpack_parser_parse_data(
    axis_msgpack_parser_t *self);
