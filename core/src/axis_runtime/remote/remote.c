//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/remote/remote.h"

#include <stdlib.h>

#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/connection/connection.h"
#include "include_internal/axis_runtime/engine/engine.h"
#include "include_internal/axis_runtime/engine/internal/remote_interface.h"
#include "include_internal/axis_runtime/engine/internal/thread.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/protocol/protocol.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"
#include "axis_utils/sanitizer/thread_check.h"

bool axis_remote_check_integrity(axis_remote_t *self, bool check_thread) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_REMOTE_SIGNATURE) {
    return false;
  }

  if (check_thread) {
    return axis_sanitizer_thread_check_do_check(&self->thread_check);
  }

  return true;
}

static bool axis_remote_could_be_close(axis_remote_t *self) {
  axis_ASSERT(self && axis_remote_check_integrity(self, true),
             "Should not happen.");

  if (!self->connection || self->connection->is_closed) {
    // The 'connection' has already been closed, so this 'remote' could be
    // closed, too.
    return true;
  }
  return false;
}

void axis_remote_destroy(axis_remote_t *self) {
  axis_ASSERT(
      self &&
          // axis_NOLINTNEXTLINE(thread-check)
          // thread-check: The belonging thread of the 'remote' is ended when
          // this function is called, so we can not check thread integrity here.
          axis_remote_check_integrity(self, false),
      "Should not happen.");
  axis_ASSERT(self->is_closed == true,
             "Remote should be closed first before been destroyed.");

  axis_signature_set(&self->signature, 0);

  if (self->connection) {
    axis_connection_destroy(self->connection);
  }

  axis_string_deinit(&self->uri);
  axis_loc_deinit(&self->explicit_dest_loc);
  axis_sanitizer_thread_check_deinit(&self->thread_check);

  if (self->on_server_connected_cmd) {
    axis_shared_ptr_destroy(self->on_server_connected_cmd);
  }

  axis_FREE(self);
}

static void axis_remote_do_close(axis_remote_t *self) {
  axis_ASSERT(self && axis_remote_check_integrity(self, true),
             "Should not happen.");

  // Mark this 'remote' as having been closed, so that other modules could know
  // this fact.
  self->is_closed = true;

  if (self->on_closed) {
    // Call the previously registered on_close callback.
    self->on_closed(self, self->on_closed_data);
  }
}

static void axis_remote_on_close(axis_remote_t *self) {
  axis_ASSERT(self && axis_remote_check_integrity(self, true),
             "Should not happen.");

  if (!axis_remote_could_be_close(self)) {
    axis_LOGI(
        "Failed to close remote (%s) because there are alive resources in it.",
        axis_string_get_raw_str(&self->uri));
    return;
  }
  axis_LOGD("Remote (%s) can be closed now.",
           axis_string_get_raw_str(&self->uri));

  axis_remote_do_close(self);
}

// This function will be called when the corresponding connection is closed.
void axis_remote_on_connection_closed(axis_UNUSED axis_connection_t *connection,
                                     void *on_closed_data) {
  axis_remote_t *remote = (axis_remote_t *)on_closed_data;
  axis_ASSERT(remote && axis_remote_check_integrity(remote, true),
             "Should not happen.");
  axis_ASSERT(
      connection && remote->connection && remote->connection == connection,
      "Invalid argument.");

  if (remote->is_closing) {
    // Proceed the closing flow of 'remote'.
    axis_remote_on_close(remote);
  } else {
    // This means the connection is closed due to it is broken, so trigger the
    // closing of the remote.
    axis_remote_close(remote);
  }
}

static axis_remote_t *axis_remote_create_empty(const char *uri,
                                             axis_connection_t *connection) {
  axis_ASSERT(connection && axis_connection_check_integrity(connection, true),
             "Should not happen.");

  axis_remote_t *self = (axis_remote_t *)axis_MALLOC(sizeof(axis_remote_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_signature_set(&self->signature, (axis_signature_t)axis_REMOTE_SIGNATURE);

  self->is_closing = false;
  self->is_closed = false;

  self->on_closed = NULL;
  self->on_closed_data = NULL;

  self->on_msg = NULL;
  self->on_msg_data = NULL;

  self->on_server_connected = NULL;
  self->on_server_connected_cmd = NULL;

  axis_string_init_formatted(&self->uri, "%s", uri ? uri : "");

  axis_sanitizer_thread_check_init_with_current_thread(&self->thread_check);

  axis_connection_attach_to_remote(connection, self);

  self->connection = connection;

  self->engine = NULL;

  axis_loc_init_empty(&self->explicit_dest_loc);

  return self;
}

static void axis_remote_set_on_closed(axis_remote_t *self,
                                     axis_remote_on_closed_func_t on_close,
                                     void *on_close_data) {
  axis_ASSERT(self && axis_remote_check_integrity(self, true),
             "Should not happen.");

  self->on_closed = on_close;
  self->on_closed_data = on_close_data;
}

static void axis_remote_set_on_msg(axis_remote_t *self,
                                  axis_remote_on_msg_func_t on_msg,
                                  void *on_msg_data) {
  axis_ASSERT(self && axis_remote_check_integrity(self, true) && on_msg,
             "Should not happen.");

  self->on_msg = on_msg;
  self->on_msg_data = on_msg_data;
}

static void axis_remote_attach_to_engine(axis_remote_t *self,
                                        axis_engine_t *engine) {
  axis_ASSERT(self && axis_remote_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(engine && axis_engine_check_integrity(engine, true),
             "Should not happen.");

  self->engine = engine;

  // Setup a callback when 'remote' wants to send messages to engine.
  axis_remote_set_on_msg(self, axis_engine_receive_msg_from_remote, NULL);

  // Setup a callback to notify the engine when 'remote' is closed.
  axis_remote_set_on_closed(self, axis_engine_on_remote_closed, engine);
}

axis_remote_t *axis_remote_create_for_engine(const char *uri,
                                           axis_engine_t *engine,
                                           axis_connection_t *connection) {
  axis_ASSERT(connection && axis_connection_check_integrity(connection, true),
             "Should not happen.");
  axis_ASSERT(engine && axis_engine_check_integrity(engine, true),
             "Should not happen.");

  // NOTE: Whether the remote uri is duplicated in the engine should _not_ be
  // checked when the remote is created, but should be checked when the engine
  // is trying to connect to the remote.

  axis_remote_t *self = axis_remote_create_empty(uri, connection);
  axis_remote_attach_to_engine(self, engine);

  return self;
}

void axis_remote_close(axis_remote_t *self) {
  axis_ASSERT(self && axis_remote_check_integrity(self, true),
             "Should not happen.");

  if (self->is_closing) {
    return;
  }

  axis_LOGD("Try to close remote (%s)", axis_string_get_raw_str(&self->uri));

  self->is_closing = true;

  if (self->connection && !self->connection->is_closed) {
    axis_connection_close(self->connection);
  } else {
    // This remote could be closed directly.
    axis_remote_on_close(self);
  }
}

bool axis_remote_on_input(axis_remote_t *self, axis_shared_ptr_t *msg,
                         axis_error_t *err) {
  axis_ASSERT(self && axis_remote_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(msg, "Should not happen.");
  axis_ASSERT(self->engine && axis_engine_check_integrity(self->engine, true),
             "Should not happen.");

  if (self->on_msg) {
    // The source of all the messages coming from this remote will be
    // 'remote->uri'. Remote URI is used to identify the identity of the other
    // side, ex: the other side is a TEN app or a TEN client.
    axis_msg_set_src_uri(msg, axis_string_get_raw_str(&self->uri));

    return self->on_msg(self, msg, self->on_msg_data);
  }
  return true;
}

void axis_remote_send_msg(axis_remote_t *self, axis_shared_ptr_t *msg) {
  axis_ASSERT(self && axis_remote_check_integrity(self, true),
             "Should not happen.");

  if (self->is_closing) {
    // The remote is closing, do not proceed to send this message to
    // 'connection'.
    return;
  }

  axis_connection_t *connection = self->connection;
  axis_ASSERT(connection && axis_connection_check_integrity(connection, true),
             "Access across threads.");
  axis_ASSERT(connection->duplicate == false &&
                 axis_connection_attach_to(self->connection) ==
                     axis_CONNECTION_ATTACH_TO_REMOTE,
             "Connection should attach to remote.");

  axis_connection_send_msg(connection, msg);
}

static void on_server_connected(axis_protocol_t *protocol, bool success) {
  axis_ASSERT(
      protocol && axis_protocol_check_integrity(protocol, true) &&
          axis_protocol_attach_to(protocol) == axis_PROTOCOL_ATTACH_TO_CONNECTION,
      "Should not happen.");

  axis_connection_t *connection = protocol->attached_target.connection;
  axis_ASSERT(connection && axis_connection_check_integrity(connection, true) &&
                 axis_connection_attach_to(connection) ==
                     axis_CONNECTION_ATTACH_TO_REMOTE,
             "Should not happen.");

  axis_remote_t *remote = connection->attached_target.remote;
  axis_ASSERT(remote && axis_remote_check_integrity(remote, true),
             "Should not happen.");

  axis_ASSERT(remote->engine && axis_engine_check_integrity(remote->engine, true),
             "Should not happen.");

  if (success) {
    axis_LOGD("Connected to remote (%s)", axis_string_get_raw_str(&remote->uri));

    if (remote->on_server_connected) {
      remote->on_server_connected(remote, remote->on_server_connected_cmd);

      // The callback has completed its mission, its time to clear it.
      remote->on_server_connected = NULL;
    }
  } else {
    axis_LOGW("Failed to connect to a remote (%s)",
             axis_string_get_raw_str(&remote->uri));

    if (remote->on_error) {
      remote->on_error(remote, remote->on_server_connected_cmd);

      // The callback has completed its mission, its time to clear it.
      remote->on_error = NULL;
    }
  }
}

void axis_remote_connect_to(axis_remote_t *self,
                           axis_remote_on_server_connected_func_t connected,
                           axis_shared_ptr_t *on_server_connected_cmd,
                           axis_remote_on_error_func_t on_error) {
  axis_ASSERT(self && axis_remote_check_integrity(self, true),
             "Should not happen.");

  axis_ASSERT(self->engine && axis_engine_check_integrity(self->engine, true),
             "Should not happen.");

  self->on_server_connected = connected;

  axis_ASSERT(!self->on_server_connected_cmd, "Should not happen.");
  if (on_server_connected_cmd) {
    self->on_server_connected_cmd =
        axis_shared_ptr_clone(on_server_connected_cmd);
  }

  self->on_error = on_error;

  axis_connection_connect_to(self->connection,
                            axis_string_get_raw_str(&self->uri),
                            on_server_connected);
}

bool axis_remote_is_uri_equal_to(axis_remote_t *self, const char *uri) {
  axis_ASSERT(self && axis_remote_check_integrity(self, true) && uri,
             "Should not happen.");

  if (axis_string_is_equal_c_str(&self->uri, uri)) {
    return true;
  }

  return false;
}

axis_runloop_t *axis_remote_get_attached_runloop(axis_remote_t *self) {
  axis_ASSERT(self &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: This function is intended to be called in
                 // different threads.
                 axis_remote_check_integrity(self, false),
             "Should not happen.");

  return axis_engine_get_attached_runloop(self->engine);
}
