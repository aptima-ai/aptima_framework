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
#include "include_internal/aptima_runtime/msg/msg.h"
#include "aptima_utils/container/list.h"
#include "aptima_utils/lib/alloc.h"
#include "aptima_utils/lib/smart_ptr.h"
#include "aptima_utils/lib/string.h"
#include "aptima_utils/macro/check.h"
#include "tests/common/client/tcp.h"

static aptima_buf_t aptima_test_msgpack_tcp_client_msgs_to_buf(aptima_list_t *msgs) {
  aptima_ASSERT(msgs && aptima_list_check_integrity(msgs), "Invalid argument.");
  return aptima_msgpack_serialize_msgs(msgs, NULL);
}

static void aptima_test_msgpack_tcp_client_buf_to_msgs(
    aptima_test_tcp_client_t *client, void *data, size_t data_size,
    aptima_list_t *msgs) {
  aptima_ASSERT(client, "Invalid argument.");

  aptima_test_msgpack_tcp_client_t *msgpack_client =
      (aptima_test_msgpack_tcp_client_t *)client;
  aptima_msgpack_deserialize_msgs(
      &msgpack_client->parser,
      aptima_BUF_STATIC_INIT_WITH_DATA_UNOWNED(data, data_size), msgs);
}

aptima_test_msgpack_tcp_client_t *aptima_test_msgpack_tcp_client_create(
    const char *app_id) {
  aptima_ASSERT(app_id, "Invalid argument.");

  aptima_test_msgpack_tcp_client_t *client =
      (aptima_test_msgpack_tcp_client_t *)aptima_MALLOC(
          sizeof(aptima_test_msgpack_tcp_client_t));
  aptima_ASSERT(client, "Failed to allocate memory.");

  if (aptima_test_tcp_client_init(&client->base, app_id,
                               aptima_test_msgpack_tcp_client_msgs_to_buf,
                               aptima_test_msgpack_tcp_client_buf_to_msgs)) {
    aptima_msgpack_parser_init(&client->parser);
  } else {
    aptima_FREE(client);
    client = NULL;
  }

  return client;
}

void aptima_test_msgpack_tcp_client_destroy(aptima_test_msgpack_tcp_client_t *self) {
  aptima_ASSERT(self, "Invalid argument.");

  aptima_test_tcp_client_deinit(&self->base);
  aptima_msgpack_parser_deinit(&self->parser);

  aptima_FREE(self);
}

bool aptima_test_msgpack_tcp_client_send_msg(aptima_test_msgpack_tcp_client_t *self,
                                          aptima_shared_ptr_t *msg) {
  aptima_ASSERT(self, "Invalid argument.");
  aptima_ASSERT(msg && aptima_msg_check_integrity(msg), "Invalid argument.");

  return aptima_test_tcp_client_send_msg(&self->base, msg);
}

aptima_shared_ptr_t *aptima_test_msgpack_tcp_client_recv_msg(
    aptima_test_msgpack_tcp_client_t *self) {
  aptima_ASSERT(self, "Invalid argument.");

  return aptima_test_tcp_client_recv_msg(&self->base);
}

void aptima_test_msgpack_tcp_client_recv_msgs_batch(
    aptima_test_msgpack_tcp_client_t *self, aptima_list_t *msgs) {
  aptima_ASSERT(self && msgs, "Invalid argument.");

  return aptima_test_tcp_client_recv_msgs_batch(&self->base, msgs);
}

aptima_shared_ptr_t *aptima_test_msgpack_tcp_client_send_and_recv_msg(
    aptima_test_msgpack_tcp_client_t *self, aptima_shared_ptr_t *msg) {
  aptima_ASSERT(self, "Invalid argument.");
  aptima_ASSERT(msg && aptima_msg_check_integrity(msg), "Invalid argument.");

  return aptima_test_tcp_client_send_and_recv_msg(&self->base, msg);
}

bool aptima_test_msgpack_tcp_client_send_data(aptima_test_msgpack_tcp_client_t *self,
                                           const char *graph_id,
                                           const char *extension_group_name,
                                           const char *extension_name,
                                           void *data, size_t size) {
  aptima_ASSERT(self, "Invalid argument.");
  aptima_ASSERT(graph_id && extension_group_name && extension_name && data,
             "Invalid argument.");

  return aptima_test_tcp_client_send_data(
      &self->base, graph_id, extension_group_name, extension_name, data, size);
}

bool aptima_test_msgpack_tcp_client_close_app(
    aptima_test_msgpack_tcp_client_t *self) {
  return aptima_test_tcp_client_close_app(&self->base);
}

void aptima_test_msgpack_tcp_client_get_info(aptima_test_msgpack_tcp_client_t *self,
                                          aptima_string_t *ip, uint16_t *port) {
  aptima_test_tcp_client_get_info(&self->base, ip, port);
}
