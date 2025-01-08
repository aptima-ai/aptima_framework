//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/engine/internal/migration.h"

#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/connection/connection.h"
#include "include_internal/axis_runtime/connection/migration.h"
#include "include_internal/axis_runtime/engine/engine.h"
#include "include_internal/axis_runtime/engine/msg_interface/common.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/protocol/protocol.h"
#include "axis_utils/io/runloop.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"
#include "axis_utils/macro/memory.h"

static axis_engine_migration_user_data_t *axis_engine_migration_user_data_create(
    axis_connection_t *connection, axis_shared_ptr_t *cmd) {
  axis_ASSERT(connection && cmd, "Invalid argument.");

  axis_engine_migration_user_data_t *self =
      axis_MALLOC(sizeof(axis_engine_migration_user_data_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  self->connection = connection;
  self->cmd = axis_shared_ptr_clone(cmd);

  return self;
}

static void axis_engine_migration_user_data_destroy(
    axis_engine_migration_user_data_t *self) {
  axis_ASSERT(self && self->cmd, "Invalid argument.");

  self->connection = NULL;

  axis_shared_ptr_destroy(self->cmd);
  self->cmd = NULL;

  axis_FREE(self);
}

void axis_engine_on_connection_cleaned(axis_engine_t *self,
                                      axis_connection_t *connection,
                                      axis_shared_ptr_t *cmd) {
  axis_ASSERT(connection, "Invalid argument.");

  axis_protocol_t *protocol = connection->protocol;
  axis_ASSERT(protocol, "Invalid argument.");

  axis_UNUSED int rc = axis_event_wait(connection->is_cleaned,
                                     TIMEOUT_FOR_CONNECTION_ALL_CLEANED);
  axis_ASSERT(!rc, "Should not happen.");

  axis_sanitizer_thread_check_set_belonging_thread_to_current_thread(
      &connection->thread_check);
  axis_ASSERT(axis_connection_check_integrity(connection, true),
             "Access across threads.");

  axis_protocol_update_belonging_thread_on_cleaned(protocol);

  // Because the command is from externally (ex: clients or other engines),
  // there will be some things the engine needs to do to handle this command, so
  // we need to put it into the queue for external commands first, rather than
  // enabling the engine to handle this command directly at this time point.
  //
  // i.e., should not call `axis_engine_handle_msg(self, cmd);` directly here.
  axis_engine_append_to_in_msgs_queue(self, cmd);

  // This is the last stage of the connection migration process, the
  // implementation protocol will be notified to do some things (ex: continue to
  // handle messages if there are any messages received during the connection
  // migration) by the following function. And
  // 'axis_connection_upgrade_migration_state_to_done()' _must_ be called after
  // the above 'axis_engine_push_to_external_cmds_queue()', as the messages must
  // be handled by engine in their original order.
  axis_connection_upgrade_migration_state_to_done(connection, self);
}

static void axis_engine_on_connection_cleaned_task(void *self_, void *arg) {
  axis_engine_t *self = (axis_engine_t *)self_;
  axis_ASSERT(self && axis_engine_check_integrity(self, true),
             "Access across threads.");

  axis_engine_migration_user_data_t *user_data =
      (axis_engine_migration_user_data_t *)arg;
  axis_ASSERT(user_data, "Invalid argument.");

  axis_shared_ptr_t *cmd = user_data->cmd;
  axis_ASSERT(cmd && axis_msg_check_integrity(cmd), "Invalid argument.");

  axis_connection_t *connection = user_data->connection;
  axis_ASSERT(connection, "Invalid argument.");

  axis_engine_on_connection_cleaned(self, connection, cmd);

  axis_engine_migration_user_data_destroy(user_data);
}

void axis_engine_on_connection_cleaned_async(axis_engine_t *self,
                                            axis_connection_t *connection,
                                            axis_shared_ptr_t *cmd) {
  axis_ASSERT(
      self &&
          // axis_NOLINTNEXTLINE(thread-check)
          axis_engine_check_integrity(self, false),
      "This function is intended to be called outside of the engine thread.");
  axis_ASSERT(connection && axis_connection_check_integrity(connection, true),
             "Access across threads.");
  axis_ASSERT(cmd && axis_msg_check_integrity(cmd), "Invalid argument.");

  // TODO(Liu): The 'connection' should be the 'original_connection' of the
  // 'cmd', so we can use the 'cmd' as the parameter directly. But we have to
  // refine the 'axis_app_on_msg()' first.
  axis_engine_migration_user_data_t *user_data =
      axis_engine_migration_user_data_create(connection, cmd);

  axis_runloop_post_task_tail(axis_engine_get_attached_runloop(self),
                             axis_engine_on_connection_cleaned_task, self,
                             user_data);
}

void axis_engine_on_connection_closed(axis_connection_t *connection,
                                     axis_UNUSED void *user_data) {
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: The connection only attaches to the engine before the
  // corresponding 'axis_remote_t' object is created, which should be an
  // intermediate state. It means the protocol has been closed if this function
  // is called, and the engine thread might be ended, so we do not check thread
  // integrity here.
  axis_ASSERT(connection && axis_connection_check_integrity(connection, false),
             "Invalid argument.");

  axis_connection_destroy(connection);
}
