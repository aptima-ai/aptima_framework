//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/app/base_dir.h"
#include "include_internal/axis_runtime/app/close.h"
#include "include_internal/axis_runtime/app/engine_interface.h"
#include "include_internal/axis_runtime/app/metadata.h"
#include "include_internal/axis_runtime/app/msg_interface/common.h"
#include "include_internal/axis_runtime/app/predefined_graph.h"
#include "include_internal/axis_runtime/connection/connection.h"
#include "include_internal/axis_runtime/engine/engine.h"
#include "axis_runtime/app/app.h"
#include "axis_utils/container/list.h"
#include "axis_utils/container/list_node.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"

bool axis_app_check_integrity(axis_app_t *self, bool check_thread) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_APP_SIGNATURE) {
    return false;
  }

  if (check_thread &&
      !axis_sanitizer_thread_check_do_check(&self->thread_check)) {
    return false;
  }

  return true;
}

static void axis_app_handle_metadata_task(void *self_, void *arg) {
  axis_app_t *self = (axis_app_t *)self_;
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_app_check_integrity(self, true), "Invalid use of app %p.",
             self);

  axis_app_handle_metadata(self);
}

void axis_app_start(axis_app_t *self) {
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Should not happen.");

  axis_app_find_and_set_base_dir(self);

  // Add the first task of app.
  axis_runloop_post_task_tail(self->loop, axis_app_handle_metadata_task, self,
                             NULL);

  axis_runloop_run(self->loop);

  axis_LOGD("TEN app runloop ends.");
}

void axis_app_add_orphan_connection(axis_app_t *self,
                                   axis_connection_t *connection) {
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Should not happen.");
  axis_ASSERT(connection && axis_connection_check_integrity(connection, true),
             "Should not happen.");

  axis_LOGD("[%s] Add a orphan connection %p (total cnt %zu)",
           axis_app_get_uri(self), connection,
           axis_list_size(&self->orphan_connections));

  axis_connection_set_on_closed(connection, axis_app_on_orphan_connection_closed,
                               NULL);

  // Do not set 'axis_connection_destroy' as the destroy function, because we
  // might _move_ a connection out of 'orphan_connections' list when it is
  // associated with an engine.
  axis_list_push_ptr_back(&self->orphan_connections, connection, NULL);
}

void axis_app_del_orphan_connection(axis_app_t *self,
                                   axis_connection_t *connection) {
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Should not happen.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: this function is always called in the app thread, however,
  // this function maybe called _after_ the connection has migrated to the
  // engine thread, so the connection belongs to the engine thread in that case.
  // And what we do here is just to check if the pointer points to a valid
  // connection instance, and we don't access the internal of the connection
  // instance here, so it is thread safe.
  axis_ASSERT(connection && axis_connection_check_integrity(connection, false),
             "Should not happen.");

  axis_LOGD("[%s] Remove a orphan connection %p", axis_app_get_uri(self),
           connection);

  axis_UNUSED bool rc =
      axis_list_remove_ptr(&self->orphan_connections, connection);
  axis_ASSERT(rc, "Should not happen.");

  connection->on_closed = NULL;
  connection->on_closed_data = NULL;
}

bool axis_app_has_orphan_connection(axis_app_t *self,
                                   axis_connection_t *connection) {
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Should not happen.");
  axis_ASSERT(connection && axis_connection_check_integrity(connection, true),
             "Should not happen.");

  axis_listnode_t *found =
      axis_list_find_ptr(&self->orphan_connections, connection);

  return found != NULL;
}

axis_runloop_t *axis_app_get_attached_runloop(axis_app_t *self) {
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is intended to be called in different threads.
  axis_ASSERT(self && axis_app_check_integrity(self, false),
             "Should not happen.");
  axis_ASSERT(self->loop, "Should not happen.");

  return self->loop;
}

const char *axis_app_get_uri(axis_app_t *self) {
  axis_ASSERT(self &&
                 // The app uri should be read-only after it has been set
                 // initially, so it's safe to read it from any other threads.
                 axis_app_check_integrity(self, false),
             "Should not happen.");

  return axis_string_get_raw_str(&self->uri);
}

axis_env_t *axis_app_get_axis_env(axis_app_t *self) {
  axis_ASSERT(self &&
                 // The app uri should be read-only after it has been set
                 // initially, so it's safe to read it from any other threads.
                 axis_app_check_integrity(self, false),
             "Should not happen.");

  return self->axis_env;
}

axis_sanitizer_thread_check_t *axis_app_get_thread_check(axis_app_t *self) {
  axis_ASSERT(self &&
                 // The app uri should be read-only after it has been set
                 // initially, so it's safe to read it from any other threads.
                 axis_app_check_integrity(self, false),
             "Should not happen.");

  return &self->thread_check;
}
