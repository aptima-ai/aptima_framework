//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/protocol/protocol.h"

#include "include_internal/axis_runtime/addon/addon.h"
#include "include_internal/axis_runtime/addon/addon_host.h"
#include "include_internal/axis_runtime/addon/protocol/protocol.h"
#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/connection/connection.h"
#include "include_internal/axis_runtime/connection/migration.h"
#include "include_internal/axis_runtime/engine/engine.h"
#include "include_internal/axis_runtime/engine/internal/migration.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/protocol/close.h"
#include "include_internal/axis_runtime/remote/remote.h"
#include "include_internal/axis_utils/log/log.h"
#include "axis_runtime/addon/addon.h"
#include "axis_runtime/app/app.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/mutex.h"
#include "axis_utils/lib/ref.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/uri.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"
#include "axis_utils/sanitizer/thread_check.h"
#include "axis_utils/value/value_object.h"

bool axis_protocol_check_integrity(axis_protocol_t *self, bool check_thread) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) != axis_PROTOCOL_SIGNATURE) {
    return false;
  }

  if (check_thread) {
    return axis_sanitizer_thread_check_do_check(&self->thread_check);
  }

  return true;
}

void axis_protocol_determine_default_property_value(axis_protocol_t *self) {
  axis_ASSERT(self && axis_protocol_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(
      self->addon_host && axis_addon_host_check_integrity(self->addon_host),
      "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  self->cascade_close_upward = axis_value_object_get_bool(
      &self->addon_host->property, axis_STR_CASCADE_CLOSE_UPWARD, &err);
  if (!axis_error_is_success(&err)) {
    self->cascade_close_upward = true;
  }

  axis_error_deinit(&err);
}

// The life cycle of 'protocol' is managed by a axis_ref_t, so the 'destroy'
// function of protocol is declared as 'static' to avoid wrong usage of the
// 'destroy' function.
static void axis_protocol_destroy(axis_protocol_t *self) {
  axis_ASSERT(self &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: The belonging thread of the 'protocol' is
                 // ended when this function is called, so we can not check
                 // thread integrity here.
                 axis_protocol_check_integrity(self, false),
             "Invalid argument.");
  axis_ASSERT(self->is_closed,
             "Protocol should be closed first before been destroyed.");

  self->addon_host->addon->on_destroy_instance(
      self->addon_host->addon, self->addon_host->axis_env, self, NULL);
}

static void axis_protocol_on_end_of_life(axis_UNUSED axis_ref_t *ref,
                                        void *supervisor) {
  axis_protocol_t *self = (axis_protocol_t *)supervisor;
  axis_ASSERT(self && axis_protocol_check_integrity(self, false),
             "Should not happen.");
  axis_ASSERT(self->is_closed,
             "The protocol should be closed first before being destroyed.");

  axis_ref_deinit(&self->ref);

  axis_protocol_destroy(self);
}

void axis_protocol_init(axis_protocol_t *self, const char *name,
                       axis_protocol_close_func_t close,
                       axis_protocol_on_output_func_t on_output,
                       axis_protocol_lisaxis_func_t listen,
                       axis_protocol_connect_to_func_t connect_to,
                       axis_protocol_migrate_func_t migrate,
                       axis_protocol_clean_func_t clean) {
  axis_ASSERT(self && name, "Should not happen.");

  axis_signature_set(&self->signature, (axis_signature_t)axis_PROTOCOL_SIGNATURE);

  axis_sanitizer_thread_check_init_with_current_thread(&self->thread_check);

  self->addon_host = NULL;
  axis_atomic_store(&self->is_closing, 0);
  self->is_closed = false;

  self->close = close;

  self->on_output = on_output;

  self->listen = listen;
  self->connect_to = connect_to;

  self->migrate = migrate;
  self->on_migrated = NULL;

  self->clean = clean;
  self->on_cleaned_for_internal = NULL;

  self->on_cleaned_for_external = NULL;

  self->cascade_close_upward = true;

  axis_string_init(&self->uri);

  self->attach_to = axis_PROTOCOL_ATTACH_TO_INVALID;
  self->attached_target.app = NULL;
  self->attached_target.connection = NULL;

  self->in_lock = axis_mutex_create();
  axis_ASSERT(self->in_lock, "Should not happen.");
  self->out_lock = axis_mutex_create();
  axis_ASSERT(self->out_lock, "Should not happen.");

  self->role = axis_PROTOCOL_ROLE_INVALID;

  axis_list_init(&self->in_msgs);
  axis_list_init(&self->out_msgs);

  axis_ref_init(&self->ref, self, axis_protocol_on_end_of_life);
}

void axis_protocol_deinit(axis_protocol_t *self) {
  axis_ASSERT(self &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: The belonging thread of the 'protocol' is
                 // ended when this function is called, so we can not check
                 // thread integrity here.
                 axis_protocol_check_integrity(self, false),
             "Should not happen.");

  axis_signature_set(&self->signature, 0);

  self->attach_to = axis_PROTOCOL_ATTACH_TO_INVALID;
  self->attached_target.app = NULL;
  self->attached_target.connection = NULL;

  axis_string_deinit(&self->uri);

  axis_mutex_destroy(self->in_lock);
  axis_list_clear(&self->in_msgs);

  axis_mutex_destroy(self->out_lock);
  axis_list_clear(&self->out_msgs);

  if (self->addon_host) {
    // Since the protocol has already been destroyed, there is no need to
    // release its resources through the corresponding addon anymore. Therefore,
    // decrement the reference count of the corresponding addon.
    axis_ref_dec_ref(&self->addon_host->ref);
    self->addon_host = NULL;
  }

  axis_sanitizer_thread_check_deinit(&self->thread_check);
}

void axis_protocol_listen(
    axis_protocol_t *self, const char *uri,
    axis_protocol_on_client_accepted_func_t on_client_accepted) {
  axis_ASSERT(self && axis_protocol_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(axis_protocol_role_is_listening(self),
             "Only the listening protocol could listen.");
  axis_ASSERT(self->listen && uri && on_client_accepted, "Should not happen.");

  axis_ASSERT(self->attach_to == axis_PROTOCOL_ATTACH_TO_APP,
             "Should not happen.");

  axis_app_t *app = self->attached_target.app;
  axis_ASSERT(app && axis_app_check_integrity(app, true),
             "Access across threads.");

  self->listen(self, uri, on_client_accepted);
}

bool axis_protocol_cascade_close_upward(axis_protocol_t *self) {
  axis_ASSERT(self && axis_protocol_check_integrity(self, true),
             "Should not happen.");
  return self->cascade_close_upward;
}

void axis_protocol_attach_to_app(axis_protocol_t *self, axis_app_t *app) {
  axis_ASSERT(self && axis_protocol_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(app && axis_app_check_integrity(app, true), "Should not happen.");

  self->attach_to = axis_PROTOCOL_ATTACH_TO_APP;
  self->attached_target.app = app;
}

void axis_protocol_attach_to_app_and_thread(axis_protocol_t *self,
                                           axis_app_t *app) {
  axis_ASSERT(app && axis_app_check_integrity(app, true), "Should not happen.");
  axis_ASSERT(self, "Should not happen.");

  axis_sanitizer_thread_check_set_belonging_thread_to_current_thread(
      &self->thread_check);

  axis_ASSERT(self && axis_protocol_check_integrity(self, true),
             "Should not happen.");

  axis_protocol_attach_to_app(self, app);
}

void axis_protocol_attach_to_connection(axis_protocol_t *self,
                                       axis_connection_t *connection) {
  axis_ASSERT(self && axis_protocol_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(connection && axis_connection_check_integrity(connection, true),
             "Should not happen.");

  self->attach_to = axis_PROTOCOL_ATTACH_TO_CONNECTION;
  self->attached_target.connection = connection;

  axis_protocol_set_on_closed(self, axis_connection_on_protocol_closed,
                             connection);
}

axis_PROTOCOL_ATTACH_TO
axis_protocol_attach_to(axis_protocol_t *self) {
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: This function will be called from the implementation protocol
  // who has its own thread, and the modification and reading of
  // 'attach_to_type' field is time-staggered.
  axis_ASSERT(self && axis_protocol_check_integrity(self, false),
             "Invalid argument.");

  return self->attach_to;
}

void axis_protocol_on_input(axis_protocol_t *self, axis_shared_ptr_t *msg) {
  axis_ASSERT(self && axis_protocol_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(msg, "Should not happen.");

  if (axis_protocol_is_closing(self)) {
    axis_LOGD("Protocol is closing, do not handle msgs.");
    return;
  }

  axis_ASSERT(axis_protocol_role_is_communication(self),
             "Only the protocols of the communication type should receive TEN "
             "messages.");
  axis_ASSERT(self->attach_to == axis_PROTOCOL_ATTACH_TO_CONNECTION,
             "The protocol should have already been attached to a connection.");

  axis_connection_t *connection = self->attached_target.connection;
  axis_ASSERT(connection,
             "The protocol should have already been attached to a connection.");

  axis_CONNECTION_MIGRATION_STATE migration_state =
      axis_connection_get_migration_state(connection);
  axis_ASSERT(migration_state == axis_CONNECTION_MIGRATION_STATE_INIT ||
                 migration_state == axis_CONNECTION_MIGRATION_STATE_DONE,
             "The protocol only can handle the input messages when the "
             "migration has not started yet or has been completed.");

  if (migration_state == axis_CONNECTION_MIGRATION_STATE_INIT) {
    axis_connection_set_migration_state(
        connection, axis_CONNECTION_MIGRATION_STATE_FIRST_MSG);
  }

  axis_list_t msgs = axis_LIST_INIT_VAL;
  axis_list_push_smart_ptr_back(&msgs, msg);

  axis_connection_on_msgs(connection, &msgs);

  axis_list_clear(&msgs);
}

void axis_protocol_on_inputs(axis_protocol_t *self, axis_list_t *msgs) {
  axis_ASSERT(self && axis_protocol_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(msgs, "Should not happen.");

  if (axis_protocol_is_closing(self)) {
    axis_LOGD("Protocol is closing, do not handle msgs.");
    return;
  }

  axis_ASSERT(axis_protocol_role_is_communication(self),
             "Only the protocols of the communication type should receive TEN "
             "messages.");
  axis_ASSERT(self->attach_to == axis_PROTOCOL_ATTACH_TO_CONNECTION,
             "The protocol should have already been attached to a connection.");

  axis_connection_t *connection = self->attached_target.connection;
  axis_ASSERT(connection,
             "The protocol should have already been attached to a connection.");
  axis_ASSERT(axis_connection_get_migration_state(connection) ==
                 axis_CONNECTION_MIGRATION_STATE_DONE,
             "The connection migration must be completed when batch handling "
             "messages.");

  axis_connection_on_msgs(connection, msgs);
}

void axis_protocol_send_msg(axis_protocol_t *self, axis_shared_ptr_t *msg) {
  axis_ASSERT(self && axis_protocol_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(msg, "Should not happen.");

  if (axis_protocol_is_closing(self)) {
    axis_LOGD("Protocol is closing, do not send msgs.");
    return;
  }

  if (self->on_output) {
    axis_list_t msgs = axis_LIST_INIT_VAL;

    axis_listnode_t *node = axis_smart_ptr_listnode_create(msg);
    axis_list_push_back(&msgs, node);

    self->on_output(self, &msgs);
  }
}

void axis_protocol_connect_to(
    axis_protocol_t *self, const char *uri,
    axis_protocol_on_server_connected_func_t on_server_connected) {
  axis_ASSERT(self && axis_protocol_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(axis_protocol_role_is_communication(self),
             "Only the communication protocol could connect to remote.");
  axis_ASSERT(uri, "Should not happen.");
  axis_ASSERT(on_server_connected, "Should not happen.");

  if (self->attach_to == axis_PROTOCOL_ATTACH_TO_CONNECTION &&
      axis_connection_attach_to(self->attached_target.connection) ==
          axis_CONNECTION_ATTACH_TO_REMOTE) {
    axis_ASSERT(
        axis_engine_check_integrity(
            self->attached_target.connection->attached_target.remote->engine,
            true),
        "Should not happen.");
  }

  if (self->connect_to) {
    self->connect_to(self, uri, on_server_connected);
  } else {
    // The protocol doesn't implement the 'connect_to' function, so the
    // 'on_server_connected' callback is called directly.
    on_server_connected(self, false);
  }
}

void axis_protocol_migrate(axis_protocol_t *self, axis_engine_t *engine,
                          axis_connection_t *connection, axis_shared_ptr_t *cmd,
                          axis_protocol_on_migrated_func_t on_migrated) {
  axis_ASSERT(self && axis_protocol_check_integrity(self, true),
             "Should not happen.");
  // Call in the app thread.
  axis_ASSERT(axis_app_check_integrity(engine->app, true), "Should not happen.");

  self->on_migrated = on_migrated;
  if (self->migrate) {
    self->migrate(self, engine, connection, cmd);
  }
}

void axis_protocol_clean(
    axis_protocol_t *self,
    axis_protocol_on_cleaned_for_internal_func_t on_cleaned_for_internal) {
  axis_ASSERT(self && axis_protocol_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(on_cleaned_for_internal, "Should not happen.");
  axis_ASSERT(self->attached_target.connection &&
                 axis_connection_attach_to(self->attached_target.connection) ==
                     axis_CONNECTION_ATTACH_TO_APP,
             "Should not happen.");
  axis_ASSERT(axis_app_check_integrity(
                 self->attached_target.connection->attached_target.app, true),
             "Should not happen.");

  self->on_cleaned_for_internal = on_cleaned_for_internal;
  if (self->clean) {
    self->clean(self);
  } else {
    // The actual protocol implementation doesn't need to do cleanup during the
    // migration, so the 'on_cleaned_for_internal' callback is called now.
    self->on_cleaned_for_internal(self);
  }
}

void axis_protocol_update_belonging_thread_on_cleaned(axis_protocol_t *self) {
  axis_ASSERT(self, "Should not happen.");

  axis_sanitizer_thread_check_set_belonging_thread_to_current_thread(
      &self->thread_check);
  axis_ASSERT(axis_protocol_check_integrity(self, true),
             "Access across threads.");
}

void axis_protocol_set_addon(axis_protocol_t *self,
                            axis_addon_host_t *addon_host) {
  axis_ASSERT(self, "Should not happen.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: in the case of JS binding, the extension group would
  // initially created in the JS main thread, and and engine thread will
  // call this function. However, these operations are all occurred
  // before the whole extension system is running, so it's thread safe.
  axis_ASSERT(axis_protocol_check_integrity(self, false), "Should not happen.");

  axis_ASSERT(addon_host, "Should not happen.");
  axis_ASSERT(axis_addon_host_check_integrity(addon_host), "Should not happen.");

  // Since the extension requires the corresponding addon to release
  // its resources, therefore, hold on to a reference count of the corresponding
  // addon.
  axis_ASSERT(!self->addon_host, "Should not happen.");
  self->addon_host = addon_host;
  axis_ref_inc_ref(&addon_host->ref);
}

axis_string_t *axis_protocol_uri_to_transport_uri(const char *uri) {
  axis_ASSERT(uri && strlen(uri), "Should not happen.");

  axis_string_t *protocol = axis_uri_get_protocol(uri);
  axis_string_t *host = axis_uri_get_host(uri);
  uint16_t port = axis_uri_get_port(uri);

  axis_addon_host_t *addon_host =
      axis_addon_protocol_find(axis_string_get_raw_str(protocol));
  axis_ASSERT(addon_host && addon_host->type == axis_ADDON_TYPE_PROTOCOL,
             "Should not happen.");

  const char *transport_type = axis_value_object_peek_string(
      &addon_host->manifest, axis_STR_TRANSPORT_TYPE);
  if (!transport_type) {
    transport_type = axis_STR_TCP;
  }

  axis_string_t *transport_uri = axis_string_create_formatted(
      "%s://%s:%d/", transport_type, axis_string_get_raw_str(host), port);

  axis_string_destroy(protocol);
  axis_string_destroy(host);

  return transport_uri;
}

axis_runloop_t *axis_protocol_get_attached_runloop(axis_protocol_t *self) {
  axis_ASSERT(self &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 axis_protocol_check_integrity(self, false),
             "This function is intended to be called in different threads.");

  switch (self->attach_to) {
    case axis_PROTOCOL_ATTACH_TO_APP:
      return axis_app_get_attached_runloop(self->attached_target.app);
    case axis_PROTOCOL_ATTACH_TO_CONNECTION:
      return axis_connection_get_attached_runloop(
          self->attached_target.connection);

    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }

  return NULL;
}

void axis_protocol_set_uri(axis_protocol_t *self, axis_string_t *uri) {
  axis_ASSERT(self && uri, "Invalid argument.");
  axis_ASSERT(axis_protocol_check_integrity(self, true),
             "Access across threads.");

  axis_string_copy(&self->uri, uri);
}

bool axis_protocol_role_is_communication(axis_protocol_t *self) {
  axis_ASSERT(self && axis_protocol_check_integrity(self, true),
             "Access across threads.");

  return self->role > axis_PROTOCOL_ROLE_LISTEN;
}

bool axis_protocol_role_is_listening(axis_protocol_t *self) {
  axis_ASSERT(self && axis_protocol_check_integrity(self, true),
             "Access across threads.");

  return self->role == axis_PROTOCOL_ROLE_LISTEN;
}
