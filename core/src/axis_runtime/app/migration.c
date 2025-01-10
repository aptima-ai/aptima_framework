//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/app/migration.h"

#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/app/close.h"
#include "include_internal/axis_runtime/connection/connection.h"
#include "include_internal/axis_runtime/engine/engine.h"
#include "axis_utils/io/runloop.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"

void axis_app_clean_connection(axis_app_t *self, axis_connection_t *connection) {
  axis_ASSERT(self && axis_app_check_integrity(self, true),
             "This function is called from the app thread when the protocol "
             "has been migrated.");
  axis_ASSERT(connection && axis_connection_check_integrity(connection, true),
             "The connection still belongs to the app thread before cleaning.");
  axis_ASSERT(
      axis_connection_attach_to(connection) == axis_CONNECTION_ATTACH_TO_APP,
      "The connection still attaches to the app before cleaning.");

  axis_app_del_orphan_connection(self, connection);
  axis_connection_clean(connection);
}

static void axis_app_clean_connection_task(void *connection_,
                                          axis_UNUSED void *arg) {
  axis_connection_t *connection = (axis_connection_t *)connection_;
  axis_ASSERT(connection && axis_connection_check_integrity(connection, true),
             "Should not happen.");

  axis_ASSERT(
      axis_connection_attach_to(connection) == axis_CONNECTION_ATTACH_TO_APP,
      "The connection still attaches to the app before cleaning.");

  axis_app_t *app = connection->attached_target.app;
  axis_ASSERT(app && axis_app_check_integrity(app, true),
             "Access across threads.");

  axis_app_clean_connection(app, connection);
}

// The relevant resource of a connection might be bound to the event loop of the
// app, so the app thread is responsible for cleaning them up.
void axis_app_clean_connection_async(axis_app_t *self,
                                    axis_connection_t *connection) {
  axis_ASSERT(self &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: This function is intended to be called outside
                 // of the APTIMA app thread.
                 axis_app_check_integrity(self, false),
             "Should not happen.");
  axis_ASSERT(connection &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: 'connection' belongs to app thread, but this
                 // function is intended to be called outside of the app thread.
                 axis_connection_check_integrity(connection, false),
             "Should not happen.");

  axis_runloop_post_task_tail(axis_app_get_attached_runloop(self),
                             axis_app_clean_connection_task, connection, NULL);
}
