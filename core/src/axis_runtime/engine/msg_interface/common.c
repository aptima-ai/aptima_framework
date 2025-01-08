//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/engine/msg_interface/common.h"

#include <stddef.h>

#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/app/msg_interface/common.h"
#include "include_internal/axis_runtime/common/loc.h"
#include "include_internal/axis_runtime/connection/connection.h"
#include "include_internal/axis_runtime/connection/migration.h"
#include "include_internal/axis_runtime/engine/engine.h"
#include "include_internal/axis_runtime/engine/internal/remote_interface.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension_context/extension_context.h"
#include "include_internal/axis_runtime/extension_group/extension_group.h"
#include "include_internal/axis_runtime/extension_thread/extension_thread.h"
#include "include_internal/axis_runtime/extension_thread/msg_interface/common.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd_base.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/msg/msg_info.h"
#include "include_internal/axis_runtime/remote/remote.h"
#include "axis_runtime/app/app.h"
#include "axis_runtime/msg/msg.h"
#include "axis_utils/container/list.h"
#include "axis_utils/container/list_node.h"
#include "axis_utils/io/runloop.h"
#include "axis_utils/lib/mutex.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"

static void axis_engine_prepend_to_in_msgs_queue(axis_engine_t *self,
                                                axis_list_t *msgs) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_engine_check_integrity(self, true),
             "Invalid use of engine %p.", self);

  if (axis_list_size(msgs)) {
    axis_UNUSED int rc = axis_mutex_lock(self->in_msgs_lock);
    axis_ASSERT(!rc, "Should not happen.");

    axis_list_concat(msgs, &self->in_msgs);
    axis_list_swap(msgs, &self->in_msgs);

    rc = axis_mutex_unlock(self->in_msgs_lock);
    axis_ASSERT(!rc, "Should not happen.");
  }
}

static void axis_engine_handle_in_msgs_sync(axis_engine_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_engine_check_integrity(self, true),
             "Invalid use of engine %p.", self);

  axis_list_t in_msgs_ = axis_LIST_INIT_VAL;

  axis_UNUSED int rc = axis_mutex_lock(self->in_msgs_lock);
  axis_ASSERT(!rc, "Should not happen.");

  axis_list_swap(&in_msgs_, &self->in_msgs);

  rc = axis_mutex_unlock(self->in_msgs_lock);
  axis_ASSERT(!rc, "Should not happen.");

  // This list stores any msgs which needs to be put back to the in_msgs queue.
  axis_list_t put_back_msgs = axis_LIST_INIT_VAL;

  axis_list_foreach (&in_msgs_, iter) {
    axis_shared_ptr_t *msg = axis_smart_ptr_listnode_get(iter.node);
    axis_ASSERT(msg && axis_msg_check_integrity(msg), "Should not happen.");
    axis_ASSERT(!axis_msg_src_is_empty(msg),
               "The message source should have been set.");

    if (axis_msg_is_cmd_and_result(msg)) {
      axis_connection_t *connection = axis_cmd_base_get_original_connection(msg);
      if (connection) {
        // If 'connection' is non-NULL, it means the command is from externally
        // (another external TEN app or client), so we need to check if the
        // 'connection' is duplicated.
        //
        // - If it is duplicated, remove it, and do not handle this command.
        // - Otherwise, create a 'remote' for this connection if there is none.

        // The connection should have already migrated to the engine thread, so
        // the thread safety of 'connection' can be maintained.
        axis_ASSERT(axis_connection_check_integrity(connection, true),
                   "Should not happen.");
        axis_ASSERT(
            axis_connection_get_migration_state(connection) ==
                axis_CONNECTION_MIGRATION_STATE_DONE,
            "The connection migration must be completed before the engine "
            "handling the cmd.");

        // The 'start_graph' command should only result in a unique channel
        // between any two TEN apps in the graph.
        if ((axis_msg_get_type(msg) == axis_MSG_TYPE_CMD_START_GRAPH) &&
            // Check if there is already a 'remote' for the other side.
            axis_engine_check_remote_is_duplicated(
                self, axis_msg_get_src_app_uri(msg))) {
          // Do not handle this 'start_graph' command, return a special
          // 'duplicate' result to the remote TEN app, so that it can close this
          // connection, and this TEN app could know that the closing of that
          // connection is normal (through the 'connect->duplicate' variable),
          // not an error condition, so does _not_ trigger the closing of the
          // whole engine.

          axis_connection_send_result_for_duplicate_connection(connection, msg);

          // The cmd result goes to the other side directly, so do not route
          // 'duplicate' cmd result to engine.
          continue;
        } else {
          if (axis_connection_attach_to(connection) !=
              axis_CONNECTION_ATTACH_TO_REMOTE) {
            // If this connection doesn't attach to a remote, we need to create
            // a remote for this connection before the engine starting to
            // dispatch the message.
            axis_engine_link_connection_to_remote(self, connection,
                                                 axis_msg_get_src_app_uri(msg));
          }
        }
      }
    }

    if (axis_engine_is_ready_to_handle_msg(self)) {
      // Only trigger the engine to handle messages if it is ready.
      axis_engine_dispatch_msg(self, msg);
    } else {
      switch (axis_msg_get_type(msg)) {
        case axis_MSG_TYPE_CMD_START_GRAPH:
        case axis_MSG_TYPE_CMD_RESULT:
          // The only message types which can be handled before the engine is
          // ready is relevant to 'start_graph' command.
          axis_engine_dispatch_msg(self, msg);
          break;

        default:
          // Otherwise put back those messages to the original external commands
          // queue.
          //
          // axis_msg_dump(msg, NULL,
          //              "Engine is unable to handle msg now, put back it:
          //              ^m");
          axis_list_push_smart_ptr_back(&put_back_msgs, msg);
          break;
      }
    }
  }

  axis_list_clear(&in_msgs_);

  // The commands in 'put back' queue should be in the front of in_msgs queue,
  // so that they can be handled first next time.
  axis_engine_prepend_to_in_msgs_queue(self, &put_back_msgs);
}

static void axis_engine_handle_in_msgs_task(void *engine_,
                                           axis_UNUSED void *arg) {
  axis_engine_t *engine = (axis_engine_t *)engine_;
  axis_ASSERT(engine && axis_engine_check_integrity(engine, true),
             "Should not happen.");

  axis_engine_handle_in_msgs_sync(engine);
}

void axis_engine_handle_in_msgs_async(axis_engine_t *self) {
  axis_ASSERT(self &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: This function is intended to be called in
                 // different threads.
                 axis_engine_check_integrity(self, false),
             "Should not happen.");

  axis_runloop_post_task_tail(axis_engine_get_attached_runloop(self),
                             axis_engine_handle_in_msgs_task, self, NULL);
}

void axis_engine_append_to_in_msgs_queue(axis_engine_t *self,
                                        axis_shared_ptr_t *cmd) {
  axis_ASSERT(self, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is used to be called from threads other than
  // the engine thread.
  axis_ASSERT(axis_engine_check_integrity(self, false),
             "Invalid use of engine %p.", self);

  axis_ASSERT(cmd && axis_msg_is_cmd_and_result(cmd), "Should not happen.");

  axis_mutex_lock(self->in_msgs_lock);
  axis_list_push_smart_ptr_back(&self->in_msgs, cmd);
  axis_mutex_unlock(self->in_msgs_lock);

  axis_engine_handle_in_msgs_async(self);
}

static void axis_engine_handle_msg(axis_engine_t *self, axis_shared_ptr_t *msg) {
  axis_ASSERT(self && axis_engine_check_integrity(self, true),
             "Invalid argument.");
  axis_ASSERT(msg && axis_msg_check_integrity(msg), "Should not happen.");

  if (axis_engine_is_closing(self) &&
      !axis_msg_type_to_handle_when_closing(msg)) {
    // Except some special commands, do not handle messages anymore if the
    // engine is closing.
    return;
  }

  if (axis_msg_is_cmd_and_result(msg)) {
    // Because the command ID is a critical information which is necessary for
    // the correct handling of all command-type messages, we need to assign a
    // command ID to messages which don't have one.
    axis_cmd_base_gen_cmd_id_if_empty(msg);
  }

  axis_error_t err;
  axis_error_init(&err);

  axis_msg_engine_handler_func_t engine_handler =
      axis_msg_info[axis_msg_get_type(msg)].engine_handler;
  if (engine_handler) {
    engine_handler(self, msg, &err);
  }

  axis_error_deinit(&err);
}

bool axis_engine_dispatch_msg(axis_engine_t *self, axis_shared_ptr_t *msg) {
  axis_ASSERT(self && axis_engine_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(msg && axis_msg_check_integrity(msg), "Should not happen.");
  axis_ASSERT(axis_msg_get_dest_cnt(msg) == 1,
             "When this function is executed, there should be only one "
             "destination remaining in the message's dest.");

  if (axis_engine_is_closing(self)) {
    // Do not dispatch the message if the engine is closing.
    return true;
  }

  axis_loc_t *dest_loc = axis_msg_get_first_dest_loc(msg);
  axis_ASSERT(dest_loc && axis_loc_check_integrity(dest_loc),
             "Should not happen.");

  axis_app_t *app = self->app;
  axis_ASSERT(app, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: The engine might have its own thread, and it is different
  // from the app's thread. When the engine is still alive, the app must also be
  // alive. Furthermore, the app associated with the engine remains unchanged
  // throughout the engine's lifecycle, and the app fields accessed underneath
  // are constant once the app is initialized. Therefore, the use of the app
  // here is considered thread-safe.
  axis_ASSERT(axis_app_check_integrity(app, false), "Invalid use of app %p.",
             app);

  if (!axis_string_is_equal_c_str(&dest_loc->app_uri, axis_app_get_uri(app))) {
    axis_ASSERT(!axis_string_is_empty(&dest_loc->app_uri),
               "The uri of the app should not be empty.");

    // The message is _not_ for the current TEN app, so route the message to the
    // correct TEN app through the correct remote.
    axis_engine_route_msg_to_remote(self, msg);
  } else {
    // The destination of the message is the current TEN app.

    if (
        // It means asking the current TEN app to do something.
        axis_string_is_empty(&dest_loc->graph_id) ||
        // It means asking another engine in the same app to do something.
        !axis_string_is_equal(&dest_loc->graph_id, &self->graph_id)) {
      // Both of these 2 cases will need the current TEN app to dispatch the
      // message, and the threads of the TEN app and the current TEN engine
      // might be different, so push the message to the command queue of the
      // current TEN app.
      axis_app_push_to_in_msgs_queue(app, msg);
    } else {
      if (axis_string_is_empty(&dest_loc->extension_group_name)) {
        // It means the destination is the current engine, so ask the current
        // engine to handle this message.

        axis_engine_handle_msg(self, msg);
      } else {
        // Find the correct extension thread to handle this message.

        bool found = false;

        axis_list_foreach (&self->extension_context->extension_groups, iter) {
          axis_extension_group_t *extension_group =
              axis_ptr_listnode_get(iter.node);
          axis_ASSERT(
              extension_group &&
                  // axis_NOLINTNEXTLINE(thread-check)
                  // thread-check: We are in the engine thread, _not_ in the
                  // extension thread. However, before the engine is closed, the
                  // pointer of the extension group and the pointer of the
                  // extension thread will not be changed, and the closing of
                  // the entire engine must start from the engine, so the
                  // execution to this position means that the engine has not
                  // been closed, so there will be no thread safety issue.
                  axis_extension_group_check_integrity(extension_group, false),
              "Should not happen.");

          if (axis_string_is_equal(&extension_group->name,
                                  &dest_loc->extension_group_name)) {
            // Find the correct extension thread, ask it to handle the message.
            found = true;
            axis_extension_thread_handle_in_msg_async(
                extension_group->extension_thread, msg);
            break;
          }
        }

        if (!found) {
          // axis_msg_dump(msg, NULL,
          //              "Failed to find the destination extension thread for "
          //              "the message ^m");

          axis_shared_ptr_t *status =
              axis_extension_group_create_invalid_dest_status(
                  msg, &dest_loc->extension_group_name);

          axis_engine_dispatch_msg(self, status);

          axis_shared_ptr_destroy(status);
        }
      }
    }
  }

  return true;
}
