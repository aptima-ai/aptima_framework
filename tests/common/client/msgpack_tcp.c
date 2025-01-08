//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "tests/common/client/msgpack_tcp.h"

#include <string.h>

#include "core_protocols/msgpack/common/parser.h"
#include "core_protocols/msgpack/msg/msg.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "axis_utils/container/list.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "tests/common/client/tcp.h"

static axis_buf_t axis_test_msgpack_tcp_client_msgs_to_buf(axis_list_t *msgs) {
  axis_ASSERT(msgs && axis_list_check_integrity(msgs), "Invalid argument.");
  return axis_msgpack_serialize_msgs(msgs, NULL);
}

static void axis_test_msgpack_tcp_client_buf_to_msgs(
    axis_test_tcp_client_t *client, void *data, size_t data_size,
    axis_list_t *msgs) {
  axis_ASSERT(client, "Invalid argument.");

  axis_test_msgpack_tcp_client_t *msgpack_client =
      (axis_test_msgpack_tcp_client_t *)client;
  axis_msgpack_deserialize_msgs(
      &msgpack_client->parser,
      axis_BUF_STATIC_INIT_WITH_DATA_UNOWNED(data, data_size), msgs);
}

axis_test_msgpack_tcp_client_t *axis_test_msgpack_tcp_client_create(
    const char *app_id) {
  axis_ASSERT(app_id, "Invalid argument.");

  axis_test_msgpack_tcp_client_t *client =
      (axis_test_msgpack_tcp_client_t *)axis_MALLOC(
          sizeof(axis_test_msgpack_tcp_client_t));
  axis_ASSERT(client, "Failed to allocate memory.");

  if (axis_test_tcp_client_init(&client->base, app_id,
                               axis_test_msgpack_tcp_client_msgs_to_buf,
                               axis_test_msgpack_tcp_client_buf_to_msgs)) {
    axis_msgpack_parser_init(&client->parser);
  } else {
    axis_FREE(client);
    client = NULL;
  }

  return client;
}

void axis_test_msgpack_tcp_client_destroy(axis_test_msgpack_tcp_client_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_test_tcp_client_deinit(&self->base);
  axis_msgpack_parser_deinit(&self->parser);

  axis_FREE(self);
}

bool axis_test_msgpack_tcp_client_send_msg(axis_test_msgpack_tcp_client_t *self,
                                          axis_shared_ptr_t *msg) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(msg && axis_msg_check_integrity(msg), "Invalid argument.");

  return axis_test_tcp_client_send_msg(&self->base, msg);
}

axis_shared_ptr_t *axis_test_msgpack_tcp_client_recv_msg(
    axis_test_msgpack_tcp_client_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  return axis_test_tcp_client_recv_msg(&self->base);
}

void axis_test_msgpack_tcp_client_recv_msgs_batch(
    axis_test_msgpack_tcp_client_t *self, axis_list_t *msgs) {
  axis_ASSERT(self && msgs, "Invalid argument.");

  return axis_test_tcp_client_recv_msgs_batch(&self->base, msgs);
}

axis_shared_ptr_t *axis_test_msgpack_tcp_client_send_and_recv_msg(
    axis_test_msgpack_tcp_client_t *self, axis_shared_ptr_t *msg) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(msg && axis_msg_check_integrity(msg), "Invalid argument.");

  return axis_test_tcp_client_send_and_recv_msg(&self->base, msg);
}

bool axis_test_msgpack_tcp_client_send_data(axis_test_msgpack_tcp_client_t *self,
                                           const char *graph_id,
                                           const char *extension_group_name,
                                           const char *extension_name,
                                           void *data, size_t size) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(graph_id && extension_group_name && extension_name && data,
             "Invalid argument.");

  return axis_test_tcp_client_send_data(
      &self->base, graph_id, extension_group_name, extension_name, data, size);
}

bool axis_test_msgpack_tcp_client_close_app(
    axis_test_msgpack_tcp_client_t *self) {
  return axis_test_tcp_client_close_app(&self->base);
}

void axis_test_msgpack_tcp_client_get_info(axis_test_msgpack_tcp_client_t *self,
                                          axis_string_t *ip, uint16_t *port) {
  axis_test_tcp_client_get_info(&self->base, ip, port);
}
