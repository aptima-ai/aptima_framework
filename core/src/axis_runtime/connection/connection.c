//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/connection/connection.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/app/msg_interface/common.h"
#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/engine/engine.h"
#include "include_internal/axis_runtime/engine/internal/migration.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd_base.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/protocol/protocol.h"
#include "include_internal/axis_runtime/remote/remote.h"
#include "axis_runtime/app/app.h"
#include "axis_runtime/msg/cmd_result/cmd_result.h"
#include "axis_utils/container/list.h"
#include "axis_utils/container/list_node.h"
#include "axis_utils/io/runloop.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/atomic.h"
#include "axis_utils/lib/ref.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"
#include "axis_utils/sanitizer/thread_check.h"

bool axis_connection_check_integrity(axis_connection_t *self, bool check_thread) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_CONNECTION_SIGNATURE) {
    return false;
  }

  if (check_thread) {
    return axis_sanitizer_thread_check_do_check(&self->thread_check);
  }

  return true;
}

static bool axis_connection_is_closing(axis_connection_t *self) {
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is intends to be called in different threads.
  axis_ASSERT(self && axis_connection_check_integrity(self, false),
             "Should not happen.");
  return axis_atomic_load(&self->is_closing) == 1;
}

static bool axis_connection_could_be_close(axis_connection_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_connection_check_integrity(self, true),
             "Invalid use of connection %p.", self);

  if (!self->protocol || self->protocol->is_closed) {
    // If there is no protocol, or the protocol has already been closed, then
    // this 'connection' could be closed, too.
    return true;
  }

  return false;
}

void axis_connection_destroy(axis_connection_t *self) {
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: The belonging thread of the 'connection' is ended when this
  // function is called, so we can not check thread integrity here.
  axis_ASSERT(self && axis_connection_check_integrity(self, false),
             "Should not happen.");
  axis_ASSERT(self->is_closed == true,
             "Connection should be closed first before been destroyed.");

  axis_signature_set(&self->signature, 0);

  if (self->protocol) {
    axis_ref_dec_ref(&self->protocol->ref);
  }

  axis_sanitizer_thread_check_deinit(&self->thread_check);
  axis_event_destroy(self->is_cleaned);

  axis_FREE(self);
}

static void axis_connection_do_close(axis_connection_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_connection_check_integrity(self, true),
             "Invalid use of connection %p.", self);

  axis_ASSERT(self->on_closed,
             "For now, the 'on_closed' callback could not be NULL, otherwise "
             "the connection would not be destroyed.");

  // Mark the 'connection' has already been closed, so that other modules (ex:
  // remote) can know this fact.
  self->is_closed = true;

  // Call the registered on_close callback.
  self->on_closed(self, self->on_closed_data);
}

// 'self->stream' could be NULL, ex: when the connection encounters some
// errors before being established.
static void axis_connection_on_close(axis_connection_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_connection_check_integrity(self, true),
             "Invalid use of connection %p.", self);

  if (axis_connection_could_be_close(self) == false) {
    axis_LOGD("Failed to close alive connection.");
    return;
  }
  axis_LOGD("Close connection.");

  axis_connection_do_close(self);
}

// This function doesn't do anything, just initiate the closing flow.
void axis_connection_close(axis_connection_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_connection_check_integrity(self, true),
             "Invalid use of connection %p.", self);

  if (axis_atomic_bool_compare_swap(&self->is_closing, 0, 1)) {
    axis_LOGD("Try to close connection.");

    axis_protocol_t *protocol = self->protocol;
    if (protocol && !protocol->is_closed) {
      // The protocol still exists, close it first.
      axis_protocol_close(protocol);
    } else {
      // The protocol has been closed, proceed to close the connection directly.
      axis_connection_on_close(self);
    }
  }
}

void axis_connection_on_protocol_closed(axis_UNUSED axis_protocol_t *protocol,
                                       void *on_closed_data) {
  axis_connection_t *self = (axis_connection_t *)on_closed_data;
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_connection_check_integrity(self, true),
             "Invalid use of connection %p.", self);

  if (axis_connection_is_closing(self)) {
    // The connection is closing, which means that the closure of the connection
    // is triggered by APTIMA runtime, e.g.: the closure of app => engine => remote
    // => connection => protocol. So when the protocol has closed, we continue
    // to close the related connection.
    axis_connection_on_close(self);
  } else {
    axis_connection_close(self);
  }
}

axis_connection_t *axis_connection_create(axis_protocol_t *protocol) {
  axis_ASSERT(protocol, "Should not happen.");

  axis_connection_t *self =
      (axis_connection_t *)axis_MALLOC(sizeof(axis_connection_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_signature_set(&self->signature,
                    (axis_signature_t)axis_CONNECTION_SIGNATURE);

  axis_sanitizer_thread_check_init_with_current_thread(&self->thread_check);

  axis_atomic_store(&self->attach_to, axis_CONNECTION_ATTACH_TO_INVALID);
  self->attached_target.app = NULL;
  self->attached_target.remote = NULL;

  self->migration_state = axis_CONNECTION_MIGRATION_STATE_INIT;

  axis_atomic_store(&self->is_closing, 0);
  self->is_closed = false;

  self->on_closed = NULL;
  self->on_closed_data = NULL;

  self->is_cleaned = axis_event_create(0, 0);

  self->protocol = protocol;
  axis_protocol_attach_to_connection(self->protocol, self);

  self->duplicate = false;

  axis_LOGD("Create a connection %p", self);

  return self;
}

void axis_connection_set_on_closed(axis_connection_t *self,
                                  axis_connection_on_closed_func_t on_closed,
                                  void *on_closed_data) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_connection_check_integrity(self, true),
             "Invalid use of connection %p.", self);

  axis_ASSERT(on_closed, "Should not happen.");

  self->on_closed = on_closed;
  self->on_closed_data = on_closed_data;
}

void axis_connection_send_msg(axis_connection_t *self, axis_shared_ptr_t *msg) {
  axis_ASSERT(self && axis_connection_check_integrity(self, true),
             "Should not happen.");

  // The message sends to connection channel MUST have dest loc.
  axis_ASSERT(msg && axis_msg_get_dest_cnt(msg) == 1, "Should not happen.");

  if (axis_connection_is_closing(self)) {
    axis_LOGD("Connection is closing, do not send msgs.");
    return;
  }

  axis_protocol_send_msg(self->protocol, msg);
}

static bool axis_connection_on_input(axis_connection_t *self,
                                    axis_shared_ptr_t *msg, axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_connection_check_integrity(self, true),
             "Invalid use of connection %p.", self);

  axis_ASSERT(msg && axis_msg_check_integrity(msg), "Should not happen.");

  // A 'connection' must be attached to an engine or a app. The way to attaching
  // to an engine is through a remote.

  switch (axis_atomic_load(&self->attach_to)) {
    case axis_CONNECTION_ATTACH_TO_REMOTE:
      // Enable the 'remote' to handle this message.
      return axis_remote_on_input(self->attached_target.remote, msg, err);
    case axis_CONNECTION_ATTACH_TO_APP:
      // Enable the 'app' to handle this message.
      return axis_app_handle_in_msg(self->attached_target.app, self, msg, err);
    default:
      axis_ASSERT(0, "Should not happen.");
      return false;
  }
}

static void axis_connection_on_protocol_cleaned(axis_protocol_t *protocol) {
  axis_ASSERT(protocol && axis_protocol_check_integrity(protocol, true),
             "We are in the app thread, and 'protocol' still belongs to the "
             "app thread now.");

  axis_connection_t *connection = protocol->attached_target.connection;
  axis_ASSERT(connection, "Invalid argument.");
  axis_ASSERT(axis_connection_check_integrity(connection, true),
             "Invalid use of connection %p. We are in the app thread, and "
             "'connection' still belongs to the app thread now.",
             connection);

  axis_event_set(connection->is_cleaned);
}

void axis_connection_clean(axis_connection_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  // The connection belongs to the app thread initially, and will be transferred
  // to the engine thread after the migration. But now (before the 'cleaning'),
  // the connection belongs to the app thread, and this function is called in
  // the app thread, so we can perform thread checking here.
  axis_ASSERT(self && axis_connection_check_integrity(self, true),
             "Invalid use of connection %p.", self);

  axis_ASSERT(self->attach_to == axis_CONNECTION_ATTACH_TO_APP,
             "Invalid argument.");
  axis_ASSERT(self->attached_target.app &&
                 axis_app_check_integrity(self->attached_target.app, true),
             "This function is called in the app thread");

  // The only thing which a connection needs to clean is the containing
  // protocol.
  axis_protocol_clean(self->protocol, axis_connection_on_protocol_cleaned);
}

static void axis_connection_handle_command_from_external_client(
    axis_connection_t *self, axis_shared_ptr_t *cmd) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_connection_check_integrity(self, true),
             "Invalid use of connection %p.", self);

  axis_ASSERT(cmd && axis_cmd_base_check_integrity(cmd), "Invalid argument.");

  // The command is coming from the outside of the APTIMA world, generate a
  // command ID for it.
  const char *cmd_id = axis_cmd_base_gen_new_cmd_id_forcibly(cmd);

  const char *src_uri = axis_msg_get_src_app_uri(cmd);
  axis_ASSERT(src_uri, "Should not happen.");

  // If this message is coming from the outside of the APTIMA world (i.e.,
  // a client), regardless of whether the src_uri of the command is set or
  // not, we forcibly use the command ID as the identity of that client.
  //
  // The effect of this operation is that when the corresponding remote is
  // created, the URI of that remote will be the source URI of the first
  // command it receives.
  axis_msg_set_src_uri(cmd, cmd_id);

  axis_protocol_t *protocol = self->protocol;
  axis_ASSERT(protocol && axis_protocol_check_integrity(protocol, true),
             "Access across threads.");

  protocol->role = axis_PROTOCOL_ROLE_IN_EXTERNAL;
}

void axis_connection_on_msgs(axis_connection_t *self, axis_list_t *msgs) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_connection_check_integrity(self, true),
             "Invalid use of connection %p.", self);

  axis_ASSERT(msgs, "Should not happen.");

  // Do some thread-safety checking.
  switch (axis_connection_attach_to(self)) {
    case axis_CONNECTION_ATTACH_TO_APP:
      axis_ASSERT(axis_app_check_integrity(self->attached_target.app, true),
                 "Should not happen.");
      break;
    case axis_CONNECTION_ATTACH_TO_REMOTE:
      axis_ASSERT(axis_engine_check_integrity(
                     self->attached_target.remote->engine, true),
                 "Should not happen.");
      break;
    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }

  axis_error_t err;
  axis_error_init(&err);

  axis_list_foreach (msgs, iter) {
    axis_shared_ptr_t *msg = axis_smart_ptr_listnode_get(iter.node);

    if (axis_msg_is_cmd_and_result(msg)) {
      // For a command message, remember which connection this command is coming
      // from.
      axis_cmd_base_set_original_connection(msg, self);

      // If this command is coming from outside of the APTIMA world (i.e.,
      // clients), the command ID would be empty, so we generate a new one for
      // it in this case now.
      const char *cmd_id = axis_cmd_base_get_cmd_id(msg);
      axis_ASSERT(cmd_id, "Should not happen.");
      if (!strlen(cmd_id)) {
        axis_connection_handle_command_from_external_client(self, msg);
      }
    } else {
      if (axis_atomic_load(&self->attach_to) !=
          axis_CONNECTION_ATTACH_TO_REMOTE) {
        // For a non-command message, if the connection isn't attached to a
        // engine, the message is nowhere to go, therefore, what we can do is to
        // drop them directly.
        continue;
      }
    }

    // Send into the APTIMA runtime to be processed.
    axis_connection_on_input(self, msg, &err);
  }

  axis_error_deinit(&err);
}

void axis_connection_connect_to(axis_connection_t *self, const char *uri,
                               void (*on_server_connected)(axis_protocol_t *,
                                                           bool)) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_connection_check_integrity(self, true),
             "Invalid use of connection %p.", self);

  axis_ASSERT(uri, "Should not happen.");

  if (axis_atomic_load(&self->attach_to) == axis_CONNECTION_ATTACH_TO_REMOTE) {
    axis_ASSERT(
        axis_engine_check_integrity(self->attached_target.remote->engine, true),
        "Should not happen.");
  }

  axis_ASSERT(
      self->protocol && axis_protocol_check_integrity(self->protocol, true),
      "Should not happen.");
  axis_ASSERT(axis_protocol_role_is_communication(self->protocol),
             "Should not happen.");

  axis_protocol_connect_to(self->protocol, uri, on_server_connected);
}

void axis_connection_attach_to_remote(axis_connection_t *self,
                                     axis_remote_t *remote) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_connection_check_integrity(self, true),
             "Invalid use of connection %p.", self);

  axis_ASSERT(remote && axis_remote_check_integrity(remote, true),
             "Should not happen.");

  axis_atomic_store(&self->attach_to, axis_CONNECTION_ATTACH_TO_REMOTE);
  self->attached_target.remote = remote;

  axis_connection_set_on_closed(self, axis_remote_on_connection_closed, remote);

  if (self->protocol) {
    axis_protocol_set_uri(self->protocol, &remote->uri);
  }
}

void axis_connection_attach_to_app(axis_connection_t *self, axis_app_t *app) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_connection_check_integrity(self, true),
             "Invalid use of connection %p.", self);

  axis_ASSERT(app && axis_app_check_integrity(app, true), "Should not happen.");

  axis_atomic_store(&self->attach_to, axis_CONNECTION_ATTACH_TO_APP);
  self->attached_target.app = app;

  // This connection has not been attached to a remote, so we record this to
  // prevent resource leak when we perform garbage collection.
  axis_app_add_orphan_connection(app, self);
}

axis_CONNECTION_ATTACH_TO axis_connection_attach_to(axis_connection_t *self) {
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: This function will be called from the protocol thread, so we
  // use 'axis_atomic_t' here.
  axis_ASSERT(self && axis_connection_check_integrity(self, false),
             "Should not happen.");
  return axis_atomic_load(&self->attach_to);
}

axis_runloop_t *axis_connection_get_attached_runloop(axis_connection_t *self) {
  axis_ASSERT(self &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: This function is intended to be called in
                 // different threads.
                 axis_connection_check_integrity(self, false),
             "Should not happen.");

  // This function will be called from the implementation protocol thread (ex:
  // 'axis_protocol_asynced_on_input_async()'), and the
  // 'axis_connection_t::migration_state' must be only accessed from the APTIMA
  // world, so do _not_ check 'axis_connection_t::migration_state' here.
  //
  // The caller side must be responsible for calling this function at the right
  // time (i.e., when the first message received which means the migration has
  // not started yet or the 'axis_protocol_t::on_cleaned_for_external()'
  // function has been called which means the migration is completed). Refer to
  // 'axis_protocol_asynced_on_input_async()'.

  switch (axis_atomic_load(&self->attach_to)) {
    case axis_CONNECTION_ATTACH_TO_REMOTE:
      return axis_remote_get_attached_runloop(self->attached_target.remote);
    case axis_CONNECTION_ATTACH_TO_ENGINE:
      return axis_engine_get_attached_runloop(self->attached_target.engine);
    case axis_CONNECTION_ATTACH_TO_APP:
      return axis_app_get_attached_runloop(self->attached_target.app);
    default:
      axis_ASSERT(0, "Should not happen.");
      return NULL;
  }
}

void axis_connection_send_result_for_duplicate_connection(
    axis_connection_t *self, axis_shared_ptr_t *cmd_start_graph) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_connection_check_integrity(self, true),
             "Invalid use of connection %p.", self);

  axis_ASSERT(cmd_start_graph, "Invalid argument.");
  axis_ASSERT(axis_cmd_base_check_integrity(cmd_start_graph),
             "Invalid use of cmd %p.", cmd_start_graph);

  self->duplicate = true;

  axis_shared_ptr_t *ret_cmd =
      axis_cmd_result_create_from_cmd(axis_STATUS_CODE_OK, cmd_start_graph);
  axis_msg_set_property(ret_cmd, "detail",
                       axis_value_create_string(axis_STR_DUPLICATE), NULL);
  axis_msg_clear_and_set_dest_from_msg_src(ret_cmd, cmd_start_graph);
  axis_connection_send_msg(self, ret_cmd);
  axis_shared_ptr_destroy(ret_cmd);
}
