//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/remote/remote.h"

#include <stdlib.h>

#include "include_internal/ten_runtime/app/app.h"
#include "include_internal/ten_runtime/connection/connection.h"
#include "include_internal/ten_runtime/engine/engine.h"
#include "include_internal/ten_runtime/engine/internal/remote_interface.h"
#include "include_internal/ten_runtime/engine/internal/thread.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "include_internal/ten_runtime/protocol/protocol.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/smart_ptr.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"
#include "ten_utils/sanitizer/thread_check.h"

bool ten_remote_check_integrity(ten_remote_t *self, bool check_thread) {
  TEN_ASSERT(self, "Should not happen.");

  if (ten_signature_get(&self->signature) !=
      (ten_signature_t)TEN_REMOTE_SIGNATURE) {
    return false;
  }

  if (check_thread) {
    return ten_sanitizer_thread_check_do_check(&self->thread_check);
  }

  return true;
}

static bool ten_remote_could_be_close(ten_remote_t *self) {
  TEN_ASSERT(self && ten_remote_check_integrity(self, true),
             "Should not happen.");

  if (!self->connection || self->connection->is_closed) {
    // The 'connection' has already been closed, so this 'remote' could be
    // closed, too.
    return true;
  }
  return false;
}

void ten_remote_destroy(ten_remote_t *self) {
  TEN_ASSERT(
      self &&
          // TEN_NOLINTNEXTLINE(thread-check)
          // thread-check: The belonging thread of the 'remote' is ended when
          // this function is called, so we can not check thread integrity here.
          ten_remote_check_integrity(self, false),
      "Should not happen.");
  TEN_ASSERT(self->is_closed == true,
             "Remote should be closed first before been destroyed.");

  ten_signature_set(&self->signature, 0);

  if (self->connection) {
    ten_connection_destroy(self->connection);
  }

  ten_string_deinit(&self->uri);
  ten_loc_deinit(&self->explicit_dest_loc);
  ten_sanitizer_thread_check_deinit(&self->thread_check);

  if (self->on_server_connected_cmd) {
    ten_shared_ptr_destroy(self->on_server_connected_cmd);
  }

  TEN_FREE(self);
}

static void ten_remote_do_close(ten_remote_t *self) {
  TEN_ASSERT(self && ten_remote_check_integrity(self, true),
             "Should not happen.");

  // Mark this 'remote' as having been closed, so that other modules could know
  // this fact.
  self->is_closed = true;

  if (self->on_closed) {
    // Call the previously registered on_close callback.
    self->on_closed(self, self->on_closed_data);
  }
}

static void ten_remote_on_close(ten_remote_t *self) {
  TEN_ASSERT(self && ten_remote_check_integrity(self, true),
             "Should not happen.");

  if (!ten_remote_could_be_close(self)) {
    TEN_LOGI(
        "Failed to close remote (%s) because there are alive resources in it.",
        ten_string_get_raw_str(&self->uri));
    return;
  }
  TEN_LOGD("Remote (%s) can be closed now.",
           ten_string_get_raw_str(&self->uri));

  ten_remote_do_close(self);
}

// This function will be called when the corresponding connection is closed.
void ten_remote_on_connection_closed(TEN_UNUSED ten_connection_t *connection,
                                     void *on_closed_data) {
  ten_remote_t *remote = (ten_remote_t *)on_closed_data;
  TEN_ASSERT(remote && ten_remote_check_integrity(remote, true),
             "Should not happen.");
  TEN_ASSERT(
      connection && remote->connection && remote->connection == connection,
      "Invalid argument.");

  if (remote->is_closing) {
    // Proceed the closing flow of 'remote'.
    ten_remote_on_close(remote);
  } else {
    // This means the connection is closed due to it is broken, so trigger the
    // closing of the remote.
    ten_remote_close(remote);
  }
}

static ten_remote_t *ten_remote_create_empty(const char *uri,
                                             ten_connection_t *connection) {
  TEN_ASSERT(connection && ten_connection_check_integrity(connection, true),
             "Should not happen.");

  ten_remote_t *self = (ten_remote_t *)TEN_MALLOC(sizeof(ten_remote_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  ten_signature_set(&self->signature, (ten_signature_t)TEN_REMOTE_SIGNATURE);

  self->is_closing = false;
  self->is_closed = false;

  self->on_closed = NULL;
  self->on_closed_data = NULL;

  self->on_msg = NULL;
  self->on_msg_data = NULL;

  self->on_server_connected = NULL;
  self->on_server_connected_cmd = NULL;

  ten_string_init_formatted(&self->uri, "%s", uri ? uri : "");

  ten_sanitizer_thread_check_init_with_current_thread(&self->thread_check);

  ten_connection_attach_to_remote(connection, self);

  self->connection = connection;

  self->engine = NULL;

  ten_loc_init_empty(&self->explicit_dest_loc);

  return self;
}

static void ten_remote_set_on_closed(ten_remote_t *self,
                                     ten_remote_on_closed_func_t on_close,
                                     void *on_close_data) {
  TEN_ASSERT(self && ten_remote_check_integrity(self, true),
             "Should not happen.");

  self->on_closed = on_close;
  self->on_closed_data = on_close_data;
}

static void ten_remote_set_on_msg(ten_remote_t *self,
                                  ten_remote_on_msg_func_t on_msg,
                                  void *on_msg_data) {
  TEN_ASSERT(self && ten_remote_check_integrity(self, true) && on_msg,
             "Should not happen.");

  self->on_msg = on_msg;
  self->on_msg_data = on_msg_data;
}

static void ten_remote_attach_to_engine(ten_remote_t *self,
                                        ten_engine_t *engine) {
  TEN_ASSERT(self && ten_remote_check_integrity(self, true),
             "Should not happen.");
  TEN_ASSERT(engine && ten_engine_check_integrity(engine, true),
             "Should not happen.");

  self->engine = engine;

  // Setup a callback when 'remote' wants to send messages to engine.
  ten_remote_set_on_msg(self, ten_engine_receive_msg_from_remote, NULL);

  // Setup a callback to notify the engine when 'remote' is closed.
  ten_remote_set_on_closed(self, ten_engine_on_remote_closed, engine);
}

ten_remote_t *ten_remote_create_for_engine(const char *uri,
                                           ten_engine_t *engine,
                                           ten_connection_t *connection) {
  TEN_ASSERT(connection && ten_connection_check_integrity(connection, true),
             "Should not happen.");
  TEN_ASSERT(engine && ten_engine_check_integrity(engine, true),
             "Should not happen.");

#if defined(_DEBUG)
  ten_remote_t *same_uri_remote = ten_engine_find_remote(engine, uri);
  TEN_ASSERT(!same_uri_remote,
             "There should be no two remotes with the same uri(%s).", uri);
#endif

  ten_remote_t *self = ten_remote_create_empty(uri, connection);
  ten_remote_attach_to_engine(self, engine);

  return self;
}

void ten_remote_close(ten_remote_t *self) {
  TEN_ASSERT(self && ten_remote_check_integrity(self, true),
             "Should not happen.");

  if (self->is_closing) {
    return;
  }

  TEN_LOGD("Try to close remote (%s)", ten_string_get_raw_str(&self->uri));

  self->is_closing = true;

  if (self->connection && !self->connection->is_closed) {
    ten_connection_close(self->connection);
  } else {
    // This remote could be closed directly.
    ten_remote_on_close(self);
  }
}

bool ten_remote_on_input(ten_remote_t *self, ten_shared_ptr_t *msg,
                         ten_error_t *err) {
  TEN_ASSERT(self && ten_remote_check_integrity(self, true),
             "Should not happen.");
  TEN_ASSERT(msg, "Should not happen.");
  TEN_ASSERT(self->engine && ten_engine_check_integrity(self->engine, true),
             "Should not happen.");

  if (self->on_msg) {
    // The source of all the messages coming from this remote will be
    // 'remote->uri'. Remote URI is used to identify the identity of the other
    // side, ex: the other side is a TEN app or a TEN client.
    ten_msg_set_src_uri(msg, ten_string_get_raw_str(&self->uri));

    return self->on_msg(self, msg, self->on_msg_data);
  }
  return true;
}

void ten_remote_send_msg(ten_remote_t *self, ten_shared_ptr_t *msg) {
  TEN_ASSERT(self && ten_remote_check_integrity(self, true),
             "Should not happen.");

  if (self->is_closing) {
    // The remote is closing, do not proceed to send this message to
    // 'connection'.
    return;
  }

  ten_connection_t *connection = self->connection;
  TEN_ASSERT(connection && ten_connection_check_integrity(connection, true),
             "Access across threads.");
  TEN_ASSERT(connection->duplicate == false &&
                 ten_connection_attach_to(self->connection) ==
                     TEN_CONNECTION_ATTACH_TO_REMOTE,
             "Connection should attach to remote.");

  ten_connection_send_msg(connection, msg);
}

static void on_server_connected(ten_protocol_t *protocol, bool success) {
  TEN_ASSERT(
      protocol && ten_protocol_check_integrity(protocol, true) &&
          ten_protocol_attach_to(protocol) == TEN_PROTOCOL_ATTACH_TO_CONNECTION,
      "Should not happen.");

  ten_connection_t *connection = protocol->attached_target.connection;
  TEN_ASSERT(connection && ten_connection_check_integrity(connection, true) &&
                 ten_connection_attach_to(connection) ==
                     TEN_CONNECTION_ATTACH_TO_REMOTE,
             "Should not happen.");

  ten_remote_t *remote = connection->attached_target.remote;
  TEN_ASSERT(remote && ten_remote_check_integrity(remote, true),
             "Should not happen.");

  TEN_ASSERT(remote->engine && ten_engine_check_integrity(remote->engine, true),
             "Should not happen.");

  if (success) {
    TEN_LOGD("Connected to remote (%s)", ten_string_get_raw_str(&remote->uri));

    if (remote->on_server_connected) {
      remote->on_server_connected(remote, remote->on_server_connected_cmd);

      // The callback has completed its mission, its time to clear it.
      remote->on_server_connected = NULL;
    }
  } else {
    TEN_LOGW("Failed to connect to a remote (%s)",
             ten_string_get_raw_str(&remote->uri));

    if (remote->on_error) {
      remote->on_error(remote, remote->on_server_connected_cmd);

      // The callback has completed its mission, its time to clear it.
      remote->on_error = NULL;
    }
  }
}

void ten_remote_connect_to(ten_remote_t *self,
                           ten_remote_on_server_connected_func_t connected,
                           ten_shared_ptr_t *on_server_connected_cmd,
                           ten_remote_on_error_func_t on_error) {
  TEN_ASSERT(self && ten_remote_check_integrity(self, true),
             "Should not happen.");

  TEN_ASSERT(self->engine && ten_engine_check_integrity(self->engine, true),
             "Should not happen.");

  self->on_server_connected = connected;

  TEN_ASSERT(!self->on_server_connected_cmd, "Should not happen.");
  if (on_server_connected_cmd) {
    self->on_server_connected_cmd =
        ten_shared_ptr_clone(on_server_connected_cmd);
  }

  self->on_error = on_error;

  ten_connection_connect_to(self->connection,
                            ten_string_get_raw_str(&self->uri),
                            on_server_connected);
}

bool ten_remote_is_uri_equal_to(ten_remote_t *self, const char *uri) {
  TEN_ASSERT(self && ten_remote_check_integrity(self, true) && uri,
             "Should not happen.");

  if (ten_string_is_equal_c_str(&self->uri, uri)) {
    return true;
  }

  return false;
}

ten_runloop_t *ten_remote_get_attached_runloop(ten_remote_t *self) {
  TEN_ASSERT(self &&
                 // TEN_NOLINTNEXTLINE(thread-check)
                 // thread-check: This function is intended to be called in
                 // different threads.
                 ten_remote_check_integrity(self, false),
             "Should not happen.");

  return ten_engine_get_attached_runloop(self->engine);
}
