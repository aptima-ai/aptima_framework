//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_runtime/aptima_config.h"

#include "core_protocols/msgpack/common/parser.h"
#include "aptima_utils/lib/smart_ptr.h"
#include "tests/common/client/tcp.h"

typedef struct aptima_test_msgpack_tcp_client_t {
  aptima_test_tcp_client_t base;
  aptima_msgpack_parser_t parser;
} aptima_test_msgpack_tcp_client_t;

aptima_RUNTIME_API aptima_test_msgpack_tcp_client_t *
aptima_test_msgpack_tcp_client_create(const char *app_id);

aptima_RUNTIME_API void aptima_test_msgpack_tcp_client_destroy(
    aptima_test_msgpack_tcp_client_t *self);

aptima_RUNTIME_API bool aptima_test_msgpack_tcp_client_send_msg(
    aptima_test_msgpack_tcp_client_t *self, aptima_shared_ptr_t *msg);

aptima_RUNTIME_API aptima_shared_ptr_t *aptima_test_msgpack_tcp_client_recv_msg(
    aptima_test_msgpack_tcp_client_t *self);

aptima_RUNTIME_API void aptima_test_msgpack_tcp_client_recv_msgs_batch(
    aptima_test_msgpack_tcp_client_t *self, aptima_list_t *msgs);

aptima_RUNTIME_PRIVATE_API aptima_shared_ptr_t *
aptima_test_msgpack_tcp_client_send_and_recv_msg(
    aptima_test_msgpack_tcp_client_t *self, aptima_shared_ptr_t *msg);

aptima_RUNTIME_PRIVATE_API bool aptima_test_msgpack_tcp_client_send_data(
    aptima_test_msgpack_tcp_client_t *self, const char *graph_id,
    const char *extension_group_name, const char *extension_name, void *data,
    size_t size);

aptima_RUNTIME_API void aptima_test_msgpack_tcp_client_get_info(
    aptima_test_msgpack_tcp_client_t *self, aptima_string_t *ip, uint16_t *port);

aptima_RUNTIME_PRIVATE_API bool aptima_test_msgpack_tcp_client_close_app(
    aptima_test_msgpack_tcp_client_t *self);
