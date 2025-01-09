//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "tests/common/client/tcp.h"

#include <stddef.h>

#include "include_internal/aptima_runtime/msg/msg.h"
#include "aptima_runtime/msg/cmd/close_app/cmd.h"
#include "aptima_runtime/ten.h"
#include "aptima_utils/container/list.h"
#include "aptima_utils/container/list_node_str.h"
#include "aptima_utils/io/network.h"
#include "aptima_utils/io/socket.h"
#include "aptima_utils/lib/alloc.h"
#include "aptima_utils/lib/buf.h"
#include "aptima_utils/lib/smart_ptr.h"
#include "aptima_utils/lib/string.h"
#include "aptima_utils/lib/time.h"
#include "aptima_utils/macro/check.h"

static void aptima_test_tcp_client_dump_socket_info(aptima_test_tcp_client_t *self,
                                                 const char *fmt, ...) {
  aptima_ASSERT(self, "Invalid argument.");

  aptima_string_t ip;
  aptima_string_init(&ip);
  uint16_t port = 0;
  aptima_socket_get_info(self->socket, &ip, &port);

  aptima_string_t new_fmt;
  aptima_string_init(&new_fmt);

  const char *p = fmt;

  while (*p) {
    if ('^' != *p) {
      aptima_string_append_formatted(&new_fmt, "%c", *p++);
      continue;
    }

    switch (*++p) {
      // The IP.
      case '1':
        aptima_string_append_formatted(&new_fmt, "%s",
                                    aptima_string_get_raw_str(&ip));
        p++;
        break;

      // The port.
      case '2':
        aptima_string_append_formatted(&new_fmt, "%d", port);
        p++;
        break;

      default:
        aptima_string_append_formatted(&new_fmt, "%c", *p++);
        break;
    }
  }

  va_list ap;
  va_start(ap, fmt);

  aptima_string_t description;
  aptima_string_init(&description);
  aptima_string_append_from_va_list(&description, aptima_string_get_raw_str(&new_fmt),
                                 ap);
  aptima_string_deinit(&new_fmt);

  va_end(ap);

  aptima_LOGD("%s", aptima_string_get_raw_str(&description));

  aptima_string_deinit(&ip);
  aptima_string_deinit(&description);
}

bool aptima_test_tcp_client_init(
    aptima_test_tcp_client_t *self, const char *app_id,
    aptima_test_tcp_client_msgs_to_buf_func_t msgs_to_buf,
    aptima_test_tcp_client_buf_to_msgs_func_t buf_to_msgs) {
  aptima_ASSERT(self && app_id, "Invalid argument.");

  aptima_string_init_formatted(&self->app_id, "%s", app_id);
  self->msgs_to_buf = msgs_to_buf;
  self->buf_to_msgs = buf_to_msgs;
  self->socket = NULL;

  aptima_list_t splitted_strs = aptima_LIST_INIT_VAL;
  aptima_c_string_split(app_id, "/", &splitted_strs);
  aptima_ASSERT(aptima_list_size(&splitted_strs) == 2, "Invalid argument.");

  char ip[256] = {0};
  int32_t port = 0;
  aptima_list_foreach (&splitted_strs, iter) {
    if (iter.index == 1) {
      aptima_string_t *splitted_str = aptima_str_listnode_get(iter.node);
      aptima_ASSERT(splitted_str, "Invalid argument.");

      aptima_host_split(aptima_string_get_raw_str(splitted_str), ip, 256, &port);
      break;
    }
  }
  aptima_list_clear(&splitted_strs);

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
        aptima_socket_create(aptima_SOCKET_FAMILY_INET, aptima_SOCKET_TYPE_STREAM,
                          aptima_SOCKET_PROTOCOL_TCP);
    aptima_ASSERT(self->socket, "Failed to create socket.");

    aptima_socket_addr_t *addr = aptima_socket_addr_create(ip, port);
    if (aptima_socket_connect(self->socket, addr)) {
      aptima_socket_addr_destroy(addr);
      break;
    }
    aptima_socket_addr_destroy(addr);
    aptima_socket_destroy(self->socket);
    self->socket = NULL;

    // Wait 100 ms between retry.
    aptima_sleep(100);
  }

  if (!self->socket) {
    aptima_LOGW("Failed to connect to %s:%d after retry %d times.", ip, port,
             TCP_CLIENT_CONNECT_RETRY_TIMES);
    return false;
  }

  return true;
}

aptima_test_tcp_client_t *aptima_test_tcp_client_create(
    const char *app_id, aptima_test_tcp_client_msgs_to_buf_func_t msgs_to_buf,
    aptima_test_tcp_client_buf_to_msgs_func_t buf_to_msgs) {
  aptima_ASSERT(app_id, "Invalid argument.");

  aptima_test_tcp_client_t *client =
      (aptima_test_tcp_client_t *)aptima_MALLOC(sizeof(aptima_test_tcp_client_t));
  aptima_ASSERT(client, "Failed to allocate memory.");
  if (!client) {
    return NULL;
  }

  if (!aptima_test_tcp_client_init(client, app_id, msgs_to_buf, buf_to_msgs)) {
    aptima_FREE(client);
    client = NULL;
  }

  return client;
}

void aptima_test_tcp_client_deinit(aptima_test_tcp_client_t *self) {
  aptima_ASSERT(self, "Invalid argument.");

  aptima_test_tcp_client_dump_socket_info(self, "Close tcp client: ^1:^2");

  aptima_string_deinit(&self->app_id);
  aptima_socket_destroy(self->socket);
}

void aptima_test_tcp_client_destroy(aptima_test_tcp_client_t *self) {
  aptima_ASSERT(self, "Invalid argument.");

  aptima_test_tcp_client_deinit(self);
  aptima_FREE(self);
}

bool aptima_test_tcp_client_send_msg(aptima_test_tcp_client_t *self,
                                  aptima_shared_ptr_t *msg) {
  aptima_ASSERT(self, "Invalid argument.");
  aptima_ASSERT(msg && aptima_msg_check_integrity(msg), "Invalid argument.");

  if (aptima_msg_get_dest_cnt(msg) == 0) {
    aptima_msg_clear_and_set_dest(msg, aptima_string_get_raw_str(&self->app_id), NULL,
                               NULL, NULL, NULL);
  }

  aptima_list_t msgs = aptima_LIST_INIT_VAL;
  aptima_list_push_smart_ptr_back(&msgs, msg);
  aptima_buf_t buf = self->msgs_to_buf(&msgs);

  size_t sent_size = 0;
  while (true) {
    ssize_t rc = aptima_socket_send(self->socket, buf.data + sent_size,
                                 buf.content_size - sent_size);
    if (rc < 0) {
      aptima_test_tcp_client_dump_socket_info(
          self, "aptima_socket_send (^1:^2) failed: %ld", rc);
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

void aptima_test_tcp_client_recv_msgs_batch(aptima_test_tcp_client_t *self,
                                         aptima_list_t *msgs) {
  aptima_ASSERT(self && msgs, "Should not happen.");

  while (true) {
    char recv_buf[8192] = {0};
    ssize_t recv_size = aptima_socket_recv(self->socket, recv_buf, 8192);
    if (recv_size > 0) {
      self->buf_to_msgs(self, recv_buf, recv_size, msgs);

      if (aptima_list_size(msgs) > 0) {
        break;
      }
    } else {
      aptima_test_tcp_client_dump_socket_info(
          self, "aptima_socket_recv (^1:^2) failed: %ld", recv_size);

      break;
    }
  }
}

aptima_shared_ptr_t *aptima_test_tcp_client_recv_msg(aptima_test_tcp_client_t *self) {
  aptima_ASSERT(self, "Invalid argument.");

  aptima_list_t msgs = aptima_LIST_INIT_VAL;
  aptima_test_tcp_client_recv_msgs_batch(self, &msgs);

  aptima_ASSERT(aptima_list_size(&msgs) <= 1, "Should not happen.");

  if (aptima_list_size(&msgs) == 1) {
    aptima_shared_ptr_t *received_msg =
        aptima_smart_ptr_listnode_get(aptima_list_front(&msgs));
    aptima_shared_ptr_t *received_msg_ = aptima_shared_ptr_clone(received_msg);

    aptima_list_clear(&msgs);

    return received_msg_;
  }

  return NULL;
}

aptima_shared_ptr_t *aptima_test_tcp_client_send_and_recv_msg(
    aptima_test_tcp_client_t *self, aptima_shared_ptr_t *msg) {
  aptima_ASSERT(self, "Invalid argument.");
  aptima_ASSERT(msg && aptima_msg_check_integrity(msg), "Invalid argument.");

  if (aptima_test_tcp_client_send_msg(self, msg)) {
    return aptima_test_tcp_client_recv_msg(self);
  } else {
    return NULL;
  }
}

bool aptima_test_tcp_client_send_data(aptima_test_tcp_client_t *self,
                                   const char *graph_id,
                                   const char *extension_group_name,
                                   const char *extension_name, void *data,
                                   size_t size) {
  aptima_ASSERT(self, "Invalid argument.");
  aptima_ASSERT(graph_id && extension_group_name && extension_name && data,
             "Invalid argument.");

  aptima_shared_ptr_t *msg = aptima_data_create("test", NULL);

  aptima_buf_t buf;
  aptima_buf_init_with_copying_data(&buf, data, size);
  aptima_data_set_buf_with_move(msg, &buf);

  aptima_msg_clear_and_set_dest(msg, aptima_string_get_raw_str(&self->app_id),
                             graph_id, extension_group_name, extension_name,
                             NULL);

  bool rc = aptima_test_tcp_client_send_msg(self, msg);
  aptima_shared_ptr_destroy(msg);

  return rc;
}

bool aptima_test_tcp_client_close_app(aptima_test_tcp_client_t *self) {
  aptima_shared_ptr_t *close_app_cmd = aptima_cmd_close_app_create();
  bool rc = aptima_test_tcp_client_send_msg(self, close_app_cmd);
  aptima_shared_ptr_destroy(close_app_cmd);

  return rc;
}

void aptima_test_tcp_client_get_info(aptima_test_tcp_client_t *self, aptima_string_t *ip,
                                  uint16_t *port) {
  aptima_ASSERT(self, "Invalid argument.");
  aptima_socket_get_info(self->socket, ip, port);
}
