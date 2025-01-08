//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/app/close.h"

#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/app/endpoint.h"
#include "include_internal/axis_runtime/app/engine_interface.h"
#include "include_internal/axis_runtime/app/msg_interface/common.h"
#include "include_internal/axis_runtime/connection/connection.h"
#include "include_internal/axis_runtime/engine/engine.h"
#include "include_internal/axis_runtime/engine/msg_interface/common.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/protocol/protocol.h"
#include "axis_runtime/app/app.h"
#include "axis_runtime/msg/cmd_result/cmd_result.h"
#include "axis_utils/io/runloop.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"
#include "axis_utils/sanitizer/thread_check.h"

static bool axis_app_has_no_work(axis_app_t *self) {
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Should not happen.");

  if (axis_list_is_empty(&self->engines) &&
      axis_list_is_empty(&self->orphan_connections)) {
    return true;
  }

  return false;
}

static bool axis_app_could_be_close(axis_app_t *self) {
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Should not happen.");

  if (axis_app_has_no_work(self) && axis_app_is_endpoint_closed(self)) {
    return true;
  }

  return false;
}

static void axis_app_proceed_to_close(axis_app_t *self) {
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Should not happen.");

  if (!axis_app_could_be_close(self)) {
    axis_LOGD("[%s] Could not close alive app.", axis_app_get_uri(self));
    return;
  }
  axis_LOGD("[%s] Close app.", axis_app_get_uri(self));

  axis_app_on_deinit(self);
}

static void axis_app_close_sync(axis_app_t *self) {
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Should not happen.");

  axis_LOGD("[%s] Try to close app.", axis_app_get_uri(self));

  axis_list_foreach (&self->engines, iter) {
    axis_engine_close_async(axis_ptr_listnode_get(iter.node));
  }

  axis_list_foreach (&self->orphan_connections, iter) {
    axis_connection_close(axis_ptr_listnode_get(iter.node));
  }

  if (self->endpoint_protocol) {
    axis_protocol_close(self->endpoint_protocol);
  }
}

static void axis_app_close_task(void *app_, axis_UNUSED void *arg) {
  axis_app_t *app = (axis_app_t *)app_;
  axis_ASSERT(app_ && axis_app_check_integrity(app_, true), "Should not happen.");

  // The app might be closed due to the problems during creation, ex: some
  // property is invalid. And all resources have not been created yet.
  if (axis_app_could_be_close(app)) {
    axis_app_proceed_to_close(app);
    return;
  }

  axis_app_close_sync(app);
}

bool axis_app_close(axis_app_t *self, axis_UNUSED axis_error_t *err) {
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: this function is used to be called in threads other than the
  // app thread, so the whole function is protected by a lock.
  axis_ASSERT(self && axis_app_check_integrity(self, false),
             "Should not happen.");

  axis_mutex_lock(self->state_lock);

  if (self->state >= axis_APP_STATE_CLOSING) {
    axis_LOGD("[%s] App has been signaled to close.", axis_app_get_uri(self));
    goto done;
  }

  axis_LOGD("[%s] Try to close app.", axis_app_get_uri(self));

  self->state = axis_APP_STATE_CLOSING;

  axis_runloop_post_task_tail(axis_app_get_attached_runloop(self),
                             axis_app_close_task, self, NULL);

done:
  axis_mutex_unlock(self->state_lock);
  return true;
}

bool axis_app_is_closing(axis_app_t *self) {
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: this function is used to be called in threads other than the
  // app thread, so the whole function is protected by a lock.
  axis_ASSERT(self && axis_app_check_integrity(self, false),
             "Should not happen.");

  bool is_closing = false;

  axis_mutex_lock(self->state_lock);
  is_closing = self->state >= axis_APP_STATE_CLOSING ? true : false;
  axis_mutex_unlock(self->state_lock);

  return is_closing;
}

void axis_app_check_termination_when_engine_closed(axis_app_t *self,
                                                  axis_engine_t *engine) {
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Should not happen.");

  if (engine->has_own_loop) {
    // Wait for the engine thread to be reclaimed. Because the engine thread
    // should have been terminated, this operation should be very fast.
    axis_LOGD("App waiting engine thread be reclaimed.");

    axis_UNUSED int rc = axis_thread_join(
        axis_sanitizer_thread_check_get_belonging_thread(&engine->thread_check),
        -1);
    axis_ASSERT(!rc, "Should not happen.");

    axis_LOGD(
        "Engine thread is reclaimed, and after this point, modify fields of "
        "'engine' is safe.");
  }

  if (engine->cmd_stop_graph != NULL) {
    const char *src_graph_id = axis_msg_get_src_graph_id(engine->cmd_stop_graph);
    if (!axis_string_is_equal_c_str(&engine->graph_id, src_graph_id)) {
      // This engine is _not_ suicidal.

      axis_shared_ptr_t *ret_cmd = axis_cmd_result_create_from_cmd(
          axis_STATUS_CODE_OK, engine->cmd_stop_graph);
      axis_msg_set_property(ret_cmd, "detail",
                           axis_value_create_string("close engine done"), NULL);

      axis_app_push_to_in_msgs_queue(self, ret_cmd);

      axis_shared_ptr_destroy(ret_cmd);
    }
  }

  axis_app_del_engine(self, engine);

  // In the case of the engine has its own thread, the engine thread has already
  // been reclaimed now, so it's safe to destroy the engine object here.
  axis_engine_destroy(engine);

  if (self->long_running_mode) {
    axis_LOGD("[%s] Don't close App due to it's in long running mode.",
             axis_app_get_uri(self));
  } else {
    // Here, we do not rely on whether the app has any remaining orphan
    // connections to decide whether to shut it down. This is because if a bad
    // client connects to the app but does not perform any subsequent actions,
    // such as failing to send a `start_graph` command, the orphan connection
    // will remain indefinitely. Consequently, if the decision to close the app
    // is based on whether there are orphan connections, the app may never be
    // able to shut down. If you want to prevent the app from terminating itself
    // simply because there are no engines at a certain moment, you can use the
    // app's `long_running_mode` mechanism. Once `long_running_mode` is enabled,
    // the app must be explicitly closed using the `close_app` command. This is
    // both normal and reasonable behavior.
    if (axis_list_is_empty(&self->engines)) {
      axis_app_close(self, NULL);
    }
  }

  if (axis_app_is_closing(self)) {
    axis_app_proceed_to_close(self);
  }
}

void axis_app_on_orphan_connection_closed(axis_connection_t *connection,
                                         axis_UNUSED void *on_closed_data) {
  axis_ASSERT(connection && axis_connection_check_integrity(connection, true),
             "Should not happen.");

  axis_app_t *self = connection->attached_target.app;
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Should not happen.");

  axis_LOGD("[%s] Orphan connection %p closed", axis_app_get_uri(self),
           connection);

  axis_app_del_orphan_connection(self, connection);
  axis_connection_destroy(connection);

  // Check if the app is in the closing phase.
  if (axis_app_is_closing(self)) {
    axis_LOGD("[%s] App is closing, check to see if it could proceed.",
             axis_app_get_uri(self));
    axis_app_proceed_to_close(self);
  } else {
    // If 'connection' is an orphan connection, it means the connection is not
    // attached to an engine, and the TEN app should _not_ be closed due to an
    // strange connection like this, otherwise, the TEN app will be very
    // fragile, anyone could simply connect to the TEN app, and close the app
    // through disconnection.
  }
}

void axis_app_on_protocol_closed(axis_UNUSED axis_protocol_t *protocol,
                                void *on_closed_data) {
  axis_app_t *self = (axis_app_t *)on_closed_data;
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Should not happen.");

  if (axis_app_is_closing(self)) {
    axis_app_proceed_to_close(self);
  }
}
