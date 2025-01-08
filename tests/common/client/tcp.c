//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "tests/common/client/tcp.h"

#include <stddef.h>

#include "include_internal/axis_runtime/msg/msg.h"
#include "axis_runtime/msg/cmd/close_app/cmd.h"
#include "axis_runtime/ten.h"
#include "axis_utils/container/list.h"
#include "axis_utils/container/list_node_str.h"
#include "axis_utils/io/network.h"
#include "axis_utils/io/socket.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/buf.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/lib/time.h"
#include "axis_utils/macro/check.h"

static void axis_test_tcp_client_dump_socket_info(axis_test_tcp_client_t *self,
                                                 const char *fmt, ...) {
  axis_ASSERT(self, "Invalid argument.");

  axis_string_t ip;
  axis_string_init(&ip);
  uint16_t port = 0;
  axis_socket_get_info(self->socket, &ip, &port);

  axis_string_t new_fmt;
  axis_string_init(&new_fmt);

  const char *p = fmt;

  while (*p) {
    if ('^' != *p) {
      axis_string_append_formatted(&new_fmt, "%c", *p++);
      continue;
    }

    switch (*++p) {
      // The IP.
      case '1':
        axis_string_append_formatted(&new_fmt, "%s",
                                    axis_string_get_raw_str(&ip));
        p++;
        break;

      // The port.
      case '2':
        axis_string_append_formatted(&new_fmt, "%d", port);
        p++;
        break;

      default:
        axis_string_append_formatted(&new_fmt, "%c", *p++);
        break;
    }
  }

  va_list ap;
  va_start(ap, fmt);

  axis_string_t description;
  axis_string_init(&description);
  axis_string_append_from_va_list(&description, axis_string_get_raw_str(&new_fmt),
                                 ap);
  axis_string_deinit(&new_fmt);

  va_end(ap);

  axis_LOGD("%s", axis_string_get_raw_str(&description));

  axis_string_deinit(&ip);
  axis_string_deinit(&description);
}

bool axis_test_tcp_client_init(
    axis_test_tcp_client_t *self, const char *app_id,
    axis_test_tcp_client_msgs_to_buf_func_t msgs_to_buf,
    axis_test_tcp_client_buf_to_msgs_func_t buf_to_msgs) {
  axis_ASSERT(self && app_id, "Invalid argument.");

  axis_string_init_formatted(&self->app_id, "%s", app_id);
  self->msgs_to_buf = msgs_to_buf;
  self->buf_to_msgs = buf_to_msgs;
  self->socket = NULL;

  axis_list_t splitted_strs = axis_LIST_INIT_VAL;
  axis_c_string_split(app_id, "/", &splitted_strs);
  axis_ASSERT(axis_list_size(&splitted_strs) == 2, "Invalid argument.");

  char ip[256] = {0};
  int32_t port = 0;
  axis_list_foreach (&splitted_strs, iter) {
    if (iter.index == 1) {
      axis_string_t *splitted_str = axis_str_listnode_get(iter.node);
      axis_ASSERT(splitted_str, "Invalid argument.");

      axis_host_split(axis_string_get_raw_str(splitted_str), ip, 256, &port);
      break;
    }
  }
  axis_list_clear(&splitted_strs);

  // According to Linux 'connect' manpage, it says:
  //
  // If connect() fails, consider the state of the socket as unspecified.
  // Portable applications should close the socket and create a new one for
  // reconnecting.
  //
  // In Linux, we can reuse the same socket, and calling connect() several
  // times. But in Mac, if we do this, after the first connect() fails (it
  // might be correct, probably the peer endpoint is not ready), the
  // afterwards connect() would still fail with errno 22 (invalid argument).
  // So we have to close the original socket, and create a new socket when
  // retrying.

  for (size_t i = 0; i < TCP_CLIENT_CONNECT_RETRY_TIMES; ++i) {
    self->socket =
        axis_socket_create(axis_SOCKET_FAMILY_INET, axis_SOCKET_TYPE_STREAM,
                          axis_SOCKET_PROTOCOL_TCP);
    axis_ASSERT(self->socket, "Failed to create socket.");

    axis_socket_addr_t *addr = axis_socket_addr_create(ip, port);
    if (axis_socket_connect(self->socket, addr)) {
      axis_socket_addr_destroy(addr);
      break;
    }
    axis_socket_addr_destroy(addr);
    axis_socket_destroy(self->socket);
    self->socket = NULL;

    // Wait 100 ms between retry.
    axis_sleep(100);
  }

  if (!self->socket) {
    axis_LOGW("Failed to connect to %s:%d after retry %d times.", ip, port,
             TCP_CLIENT_CONNECT_RETRY_TIMES);
    return false;
  }

  return true;
}

axis_test_tcp_client_t *axis_test_tcp_client_create(
    const char *app_id, axis_test_tcp_client_msgs_to_buf_func_t msgs_to_buf,
    axis_test_tcp_client_buf_to_msgs_func_t buf_to_msgs) {
  axis_ASSERT(app_id, "Invalid argument.");

  axis_test_tcp_client_t *client =
      (axis_test_tcp_client_t *)axis_MALLOC(sizeof(axis_test_tcp_client_t));
  axis_ASSERT(client, "Failed to allocate memory.");
  if (!client) {
    return NULL;
  }

  if (!axis_test_tcp_client_init(client, app_id, msgs_to_buf, buf_to_msgs)) {
    axis_FREE(client);
    client = NULL;
  }

  return client;
}

void axis_test_tcp_client_deinit(axis_test_tcp_client_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_test_tcp_client_dump_socket_info(self, "Close tcp client: ^1:^2");

  axis_string_deinit(&self->app_id);
  axis_socket_destroy(self->socket);
}

void axis_test_tcp_client_destroy(axis_test_tcp_client_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_test_tcp_client_deinit(self);
  axis_FREE(self);
}

bool axis_test_tcp_client_send_msg(axis_test_tcp_client_t *self,
                                  axis_shared_ptr_t *msg) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(msg && axis_msg_check_integrity(msg), "Invalid argument.");

  if (axis_msg_get_dest_cnt(msg) == 0) {
    axis_msg_clear_and_set_dest(msg, axis_string_get_raw_str(&self->app_id), NULL,
                               NULL, NULL, NULL);
  }

  axis_list_t msgs = axis_LIST_INIT_VAL;
  axis_list_push_smart_ptr_back(&msgs, msg);
  axis_buf_t buf = self->msgs_to_buf(&msgs);

  size_t sent_size = 0;
  while (true) {
    ssize_t rc = axis_socket_send(self->socket, buf.data + sent_size,
                                 buf.content_size - sent_size);
    if (rc < 0) {
      axis_test_tcp_client_dump_socket_info(
          self, "axis_socket_send (^1:^2) failed: %ld", rc);
      return false;
    }

    sent_size += rc;
    if (sent_size == buf.content_size) {
      break;
    }
  }

  free(buf.data);

  return true;
}

void axis_test_tcp_client_recv_msgs_batch(axis_test_tcp_client_t *self,
                                         axis_list_t *msgs) {
  axis_ASSERT(self && msgs, "Should not happen.");

  while (true) {
    char recv_buf[8192] = {0};
    ssize_t recv_size = axis_socket_recv(self->socket, recv_buf, 8192);
    if (recv_size > 0) {
      self->buf_to_msgs(self, recv_buf, recv_size, msgs);

      if (axis_list_size(msgs) > 0) {
        break;
      }
    } else {
      axis_test_tcp_client_dump_socket_info(
          self, "axis_socket_recv (^1:^2) failed: %ld", recv_size);

      break;
    }
  }
}

axis_shared_ptr_t *axis_test_tcp_client_recv_msg(axis_test_tcp_client_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_list_t msgs = axis_LIST_INIT_VAL;
  axis_test_tcp_client_recv_msgs_batch(self, &msgs);

  axis_ASSERT(axis_list_size(&msgs) <= 1, "Should not happen.");

  if (axis_list_size(&msgs) == 1) {
    axis_shared_ptr_t *received_msg =
        axis_smart_ptr_listnode_get(axis_list_front(&msgs));
    axis_shared_ptr_t *received_msg_ = axis_shared_ptr_clone(received_msg);

    axis_list_clear(&msgs);

    return received_msg_;
  }

  return NULL;
}

axis_shared_ptr_t *axis_test_tcp_client_send_and_recv_msg(
    axis_test_tcp_client_t *self, axis_shared_ptr_t *msg) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(msg && axis_msg_check_integrity(msg), "Invalid argument.");

  if (axis_test_tcp_client_send_msg(self, msg)) {
    return axis_test_tcp_client_recv_msg(self);
  } else {
    return NULL;
  }
}

bool axis_test_tcp_client_send_data(axis_test_tcp_client_t *self,
                                   const char *graph_id,
                                   const char *extension_group_name,
                                   const char *extension_name, void *data,
                                   size_t size) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(graph_id && extension_group_name && extension_name && data,
             "Invalid argument.");

  axis_shared_ptr_t *msg = axis_data_create("test", NULL);

  axis_buf_t buf;
  axis_buf_init_with_copying_data(&buf, data, size);
  axis_data_set_buf_with_move(msg, &buf);

  axis_msg_clear_and_set_dest(msg, axis_string_get_raw_str(&self->app_id),
                             graph_id, extension_group_name, extension_name,
                             NULL);

  bool rc = axis_test_tcp_client_send_msg(self, msg);
  axis_shared_ptr_destroy(msg);

  return rc;
}

bool axis_test_tcp_client_close_app(axis_test_tcp_client_t *self) {
  axis_shared_ptr_t *close_app_cmd = axis_cmd_close_app_create();
  bool rc = axis_test_tcp_client_send_msg(self, close_app_cmd);
  axis_shared_ptr_destroy(close_app_cmd);

  return rc;
}

void axis_test_tcp_client_get_info(axis_test_tcp_client_t *self, axis_string_t *ip,
                                  uint16_t *port) {
  axis_ASSERT(self, "Invalid argument.");
  axis_socket_get_info(self->socket, ip, port);
}
