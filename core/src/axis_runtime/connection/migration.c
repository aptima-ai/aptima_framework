//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/connection/migration.h"

#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/connection/connection.h"
#include "include_internal/axis_runtime/engine/engine.h"
#include "include_internal/axis_runtime/engine/internal/migration.h"
#include "include_internal/axis_runtime/protocol/protocol.h"
#include "axis_utils/io/runloop.h"
#include "axis_utils/lib/ref.h"
#include "axis_utils/macro/check.h"

void axis_connection_migrate(axis_connection_t *self, axis_engine_t *engine,
                            axis_shared_ptr_t *cmd) {
  axis_ASSERT(self && axis_connection_check_integrity(self, true),
             "Should not happen.");
  // Call in the app thread.
  axis_ASSERT(axis_app_check_integrity(engine->app, true), "Should not happen.");
  axis_ASSERT(self->protocol, "Should not happen.");

  axis_protocol_migrate(self->protocol, engine, self, cmd, NULL);
}

bool axis_connection_needs_to_migrate(axis_connection_t *self,
                                     axis_engine_t *engine) {
  axis_ASSERT(self, "Invalid argument.");

  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is always called in the app thread before
  // handling the cmd. If the connection needs to migrate, it means the app and
  // engine thread are different, and the belonging thread of the connection
  // will be switched from the app thread to the engine thread. So we do _not_
  // check thread safety here.
  axis_ASSERT(axis_connection_check_integrity(self, false), "Invalid argument.");

  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is always called in the app thread, and the
  // engine may have its own thread, so we do not check thread safety here. As
  // the 'axis_engine_t::has_own_loop' is immutable, there is no access issues.
  axis_ASSERT(engine && axis_engine_check_integrity(engine, false),
             "Invalid argument.");

  if (engine->has_own_loop) {
    return self->migration_state == axis_CONNECTION_MIGRATION_STATE_FIRST_MSG;
  }

  // The engine does not have its own thread, it's time to upgrade the
  // migration_state to 'DONE' directly.
  if (self->migration_state == axis_CONNECTION_MIGRATION_STATE_FIRST_MSG) {
    axis_ASSERT(axis_connection_attach_to(self) == axis_CONNECTION_ATTACH_TO_APP,
               "The connection still attaches to the app before migration.");

    axis_app_t *app = self->attached_target.app;
    axis_ASSERT(
        app && axis_app_check_integrity(app, true),
        "This function is called from the app thread before handling the cmd.");

    axis_app_del_orphan_connection(app, self);
    axis_connection_upgrade_migration_state_to_done(self, engine);
  }

  return false;
}

static void axis_protocol_on_cleaned_task(void *self_, void *arg) {
  axis_protocol_t *self = (axis_protocol_t *)self_;
  axis_ASSERT(self && axis_protocol_check_integrity(self, true),
             "Access across threads.");

  bool is_migration_state_reset = arg;

  self->on_cleaned_for_external(self, is_migration_state_reset);

  axis_ref_dec_ref(&self->ref);
}

static void axis_connection_on_migration_is_done_or_reset(
    axis_connection_t *self, bool is_migration_state_reset) {
  axis_ASSERT(self && axis_connection_check_integrity(self, true),
             "Access across threads.");

  axis_protocol_t *protocol = self->protocol;
  axis_ASSERT(protocol && axis_protocol_check_integrity(protocol, true),
             "Access across threads.");

  if (protocol->on_cleaned_for_external) {
    axis_ref_inc_ref(&protocol->ref);

    // The connection migration is completed, it's time to notify the
    // implementation protocol to do the post processing (ex: continue to handle
    // the messages if there are any messages received during the connection
    // migration) by calling 'axis_protocol_t::on_cleaned_for_external()'. But
    // we can not call this function directly because of the following 2
    // reasons:
    //
    // 1) The engine has not started to handle the first message from the
    //    connection after the migration is completed (the message delivered to
    //    the connection before the migration has started), refer to
    //    'axis_engine_on_connection_cleaned()'. In other words, the
    //    corresponding 'axis_remote_t' for the connection has not been created
    //    yet.
    //
    // 2) The implementation protocol who implements the _integrated_ protocol
    //    interface attaches to the engine's runloop if the migration is
    //    completed. If we call 'axis_protocol_t::on_cleaned_for_external()'
    //    directly and there are messages pending during the migration, those
    //    messages will be handled _before_ the first message, in other words,
    //    the messages handled by the engine are out of order.
    //
    // So we prefer to use a runloop task here even through we are in the engine
    // thread if the migration is completed. We must make sure the engine handle
    // all the messages in the original order.
    axis_runloop_t *loop = axis_connection_get_attached_runloop(self);
    axis_runloop_post_task_tail(loop, axis_protocol_on_cleaned_task, protocol,
                               (void *)is_migration_state_reset);
  } else {
    axis_ASSERT(0, "Should not happen.");
  }
}

void axis_connection_upgrade_migration_state_to_done(axis_connection_t *self,
                                                    axis_engine_t *engine) {
  axis_ASSERT(self && axis_connection_check_integrity(self, true),
             "The migration is completed, the belonging thread must be the "
             "engine's thread.");

  if (engine) {
    // The message is sent to the app, not an engine.

    axis_ASSERT(engine && axis_engine_check_integrity(engine, true),
               "Access across threads.");

    // @{
    // Attach to engine.
    self->attached_target.engine = engine;
    axis_atomic_store(&self->attach_to, axis_CONNECTION_ATTACH_TO_ENGINE);

    // We have to set the 'on_closed' callback to destroy the connection, if the
    // connection is being closed before the corresponding 'axis_remote_t' object
    // is created. Ex: the connection is 'duplicated' in the 'start_graph'
    // stage, refer to
    // 'axis_engine_close_duplicated_remote_or_upgrade_it_to_normal()'.
    axis_connection_set_on_closed(self, axis_engine_on_connection_closed, NULL);
    // @}
  }

  self->migration_state = axis_CONNECTION_MIGRATION_STATE_DONE;

  axis_connection_on_migration_is_done_or_reset(self, false);
}

void axis_connection_migration_state_reset_when_engine_not_found(
    axis_connection_t *self) {
  axis_ASSERT(self && axis_connection_check_integrity(self, true),
             "This function is always called from the app thread when the "
             "expected engine was not found.");
  axis_ASSERT(
      axis_connection_attach_to(self) == axis_CONNECTION_ATTACH_TO_APP,
      "No engine has been matched yet, the connection still attaches to the "
      "app now.");

  self->migration_state = axis_CONNECTION_MIGRATION_STATE_INIT;

  axis_connection_on_migration_is_done_or_reset(self, true);
}

axis_CONNECTION_MIGRATION_STATE axis_connection_get_migration_state(
    axis_connection_t *self) {
  axis_ASSERT(self && axis_connection_check_integrity(self, true),
             "Access across threads.");

  return self->migration_state;
}

void axis_connection_set_migration_state(
    axis_connection_t *self, axis_CONNECTION_MIGRATION_STATE new_state) {
  axis_ASSERT(self && axis_connection_check_integrity(self, true),
             "Access across threads.");

  self->migration_state = new_state;
}
