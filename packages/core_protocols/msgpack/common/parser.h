//
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#pragma once

#include "aptima_runtime/aptima_config.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "msgpack.h"
#include "aptima_utils/lib/smart_ptr.h"

typedef struct aptima_msgpack_parser_t {
  msgpack_unpacker unpacker;
  msgpack_unpacked unpacked;
} aptima_msgpack_parser_t;

aptima_RUNTIME_PRIVATE_API void aptima_msgpack_parser_init(
    aptima_msgpack_parser_t *self);

aptima_RUNTIME_PRIVATE_API void aptima_msgpack_parser_deinit(
    aptima_msgpack_parser_t *self);

aptima_RUNTIME_PRIVATE_API void aptima_msgpack_parser_feed_data(
    aptima_msgpack_parser_t *self, const char *data, size_t data_size);

aptima_RUNTIME_PRIVATE_API aptima_shared_ptr_t *aptima_msgpack_parser_parse_data(
    aptima_msgpack_parser_t *self);
