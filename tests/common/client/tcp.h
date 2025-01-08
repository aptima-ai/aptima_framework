//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_utils/io/socket.h"
#include "axis_utils/lib/smart_ptr.h"

#define TCP_CLIENT_CONNECT_RETRY_TIMES 200

typedef struct axis_test_tcp_client_t axis_test_tcp_client_t;

typedef axis_buf_t (*axis_test_tcp_client_msgs_to_buf_func_t)(axis_list_t *msgs);
typedef void (*axis_test_tcp_client_buf_to_msgs_func_t)(
    axis_test_tcp_client_t *client, void *data, size_t data_size,
    axis_list_t *msgs);

typedef struct axis_test_tcp_client_t {
  axis_string_t app_id;
  axis_socket_t *socket;

  axis_test_tcp_client_msgs_to_buf_func_t msgs_to_buf;
  axis_test_tcp_client_buf_to_msgs_func_t buf_to_msgs;
} axis_test_tcp_client_t;

axis_RUNTIME_API bool axis_test_tcp_client_init(
    axis_test_tcp_client_t *self, const char *app_id,
    axis_test_tcp_client_msgs_to_buf_func_t msgs_to_buf,
    axis_test_tcp_client_buf_to_msgs_func_t buf_to_msgs);

axis_RUNTIME_API void axis_test_tcp_client_deinit(axis_test_tcp_client_t *self);

axis_RUNTIME_API axis_test_tcp_client_t *axis_test_tcp_client_create(
    const char *app_id, axis_test_tcp_client_msgs_to_buf_func_t msgs_to_buf,
    axis_test_tcp_client_buf_to_msgs_func_t buf_to_msgs);

axis_RUNTIME_API void axis_test_tcp_client_destroy(axis_test_tcp_client_t *self);

axis_RUNTIME_API bool axis_test_tcp_client_send_msg(axis_test_tcp_client_t *self,
                                                  axis_shared_ptr_t *msg);

axis_RUNTIME_API axis_shared_ptr_t *axis_test_tcp_client_recv_msg(
    axis_test_tcp_client_t *self);

axis_RUNTIME_API void axis_test_tcp_client_recv_msgs_batch(
    axis_test_tcp_client_t *self, axis_list_t *msgs);

axis_RUNTIME_API axis_shared_ptr_t *axis_test_tcp_client_send_and_recv_msg(
    axis_test_tcp_client_t *self, axis_shared_ptr_t *msg);

axis_RUNTIME_API bool axis_test_tcp_client_send_data(
    axis_test_tcp_client_t *self, const char *graph_id,
    const char *extension_group_name, const char *extension_name, void *data,
    size_t size);

axis_RUNTIME_API void axis_test_tcp_client_get_info(axis_test_tcp_client_t *self,
                                                  axis_string_t *ip,
                                                  uint16_t *port);

axis_RUNTIME_API bool axis_test_tcp_client_close_app(axis_test_tcp_client_t *self);
