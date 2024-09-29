//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/engine/msg_interface/common.h"

#include <stddef.h>

#include "include_internal/ten_runtime/app/app.h"
#include "include_internal/ten_runtime/app/msg_interface/common.h"
#include "include_internal/ten_runtime/common/loc.h"
#include "include_internal/ten_runtime/connection/connection.h"
#include "include_internal/ten_runtime/connection/migration.h"
#include "include_internal/ten_runtime/engine/engine.h"
#include "include_internal/ten_runtime/engine/internal/remote_interface.h"
#include "include_internal/ten_runtime/extension/extension.h"
#include "include_internal/ten_runtime/extension_context/extension_context.h"
#include "include_internal/ten_runtime/extension_group/extension_group.h"
#include "include_internal/ten_runtime/extension_thread/extension_thread.h"
#include "include_internal/ten_runtime/extension_thread/msg_interface/common.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd_base.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "include_internal/ten_runtime/msg/msg_info.h"
#include "include_internal/ten_runtime/remote/remote.h"
#include "ten_runtime/app/app.h"
#include "ten_utils/container/list.h"
#include "ten_utils/container/list_node.h"
#include "ten_utils/io/runloop.h"
#include "ten_utils/lib/mutex.h"
#include "ten_utils/lib/smart_ptr.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"

static void ten_engine_prepend_to_in_msgs_queue(ten_engine_t *self,
                                                ten_list_t *msgs) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_engine_check_integrity(self, true),
             "Invalid use of engine %p.", self);

  if (ten_list_size(msgs)) {
    TEN_UNUSED int rc = ten_mutex_lock(self->in_msgs_lock);
    TEN_ASSERT(!rc, "Should not happen.");

    ten_list_concat(msgs, &self->in_msgs);
    ten_list_swap(msgs, &self->in_msgs);

    rc = ten_mutex_unlock(self->in_msgs_lock);
    TEN_ASSERT(!rc, "Should not happen.");
  }
}

static void ten_engine_handle_in_msgs_sync(ten_engine_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_engine_check_integrity(self, true),
             "Invalid use of engine %p.", self);

  ten_list_t in_msgs_ = TEN_LIST_INIT_VAL;

  TEN_UNUSED int rc = ten_mutex_lock(self->in_msgs_lock);
  TEN_ASSERT(!rc, "Should not happen.");

  ten_list_swap(&in_msgs_, &self->in_msgs);

  rc = ten_mutex_unlock(self->in_msgs_lock);
  TEN_ASSERT(!rc, "Should not happen.");

  // This list stores any msgs which needs to be put back to the in_msgs queue.
  ten_list_t put_back_msgs = TEN_LIST_INIT_VAL;

  ten_list_foreach (&in_msgs_, iter) {
    ten_shared_ptr_t *msg = ten_smart_ptr_listnode_get(iter.node);
    TEN_ASSERT(msg && ten_msg_check_integrity(msg), "Should not happen.");
    TEN_ASSERT(!ten_msg_src_is_empty(msg),
               "The message source should have been set.");

    if (ten_msg_is_cmd_and_result(msg)) {
      ten_connection_t *connection = ten_cmd_base_get_original_connection(msg);
      if (connection) {
        // If 'connection' is non-NULL, it means the command is from externally
        // (another external TEN app or client), so we need to check if the
        // 'connection' is duplicated.
        //
        // - If it is duplicated, remove it, and do not handle this command.
        // - Otherwise, create a 'remote' for this connection if there is none.

        // The connection should have already migrated to the engine thread, so
        // the thread safety of 'connection' can be maintained.
        TEN_ASSERT(ten_connection_check_integrity(connection, true),
                   "Should not happen.");
        TEN_ASSERT(
            ten_connection_get_migration_state(connection) ==
                TEN_CONNECTION_MIGRATION_STATE_DONE,
            "The connection migration must be completed before the engine "
            "handling the cmd.");

        // The 'start_graph' command should only result in a unique channel
        // between any two TEN apps in the graph.
        if ((ten_msg_get_type(msg) == TEN_MSG_TYPE_CMD_START_GRAPH) &&
            // Check if there is already a 'remote' for the other side.
            ten_engine_check_remote_is_duplicated(
                self, ten_msg_get_src_app_uri(msg))) {
          // Do not handle this 'start_graph' command, return a special
          // 'duplicate' result to the remote TEN app, so that it can close this
          // connection, and this TEN app could know that the closing of that
          // connection is normal (through the 'connect->duplicate' variable),
          // not an error condition, so does _not_ trigger the closing of the
          // whole engine.

          ten_connection_send_result_for_duplicate_connection(connection, msg);

          // The cmd result goes to the other side directly, so do not route
          // 'duplicate' cmd result to engine.
          continue;
        } else {
          if (ten_connection_attach_to(connection) !=
              TEN_CONNECTION_ATTACH_TO_REMOTE) {
            // If this connection doesn't attach to a remote, we need to create
            // a remote for this connection before the engine starting to
            // dispatch the message.
            ten_engine_link_connection_to_remote(self, connection,
                                                 ten_msg_get_src_app_uri(msg));
          }
        }
      }
    }

    if (ten_engine_is_ready_to_handle_msg(self)) {
      // Only trigger the engine to handle messages if it is ready.
      ten_engine_dispatch_msg(self, msg);
    } else {
      switch (ten_msg_get_type(msg)) {
        case TEN_MSG_TYPE_CMD_START_GRAPH:
        case TEN_MSG_TYPE_CMD_RESULT:
          // The only message types which can be handled before the engine is
          // ready is relevant to 'start_graph' command.
          ten_engine_dispatch_msg(self, msg);
          break;

        default:
          // Otherwise put back those messages to the original external commands
          // queue.
          ten_msg_dump(msg, NULL,
                       "Engine is unable to handle msg now, put back it: ^m");
          ten_list_push_smart_ptr_back(&put_back_msgs, msg);
          break;
      }
    }
  }

  ten_list_clear(&in_msgs_);

  // The commands in 'put back' queue should be in the front of in_msgs queue,
  // so that they can be handled first next time.
  ten_engine_prepend_to_in_msgs_queue(self, &put_back_msgs);
}

static void ten_engine_handle_in_msgs_task(void *engine_,
                                           TEN_UNUSED void *arg) {
  ten_engine_t *engine = (ten_engine_t *)engine_;
  TEN_ASSERT(engine && ten_engine_check_integrity(engine, true),
             "Should not happen.");

  ten_engine_handle_in_msgs_sync(engine);
}

void ten_engine_handle_in_msgs_async(ten_engine_t *self) {
  TEN_ASSERT(self &&
                 // TEN_NOLINTNEXTLINE(thread-check)
                 // thread-check: This function is intended to be called in
                 // different threads.
                 ten_engine_check_integrity(self, false),
             "Should not happen.");

  ten_runloop_post_task_tail(ten_engine_get_attached_runloop(self),
                             ten_engine_handle_in_msgs_task, self, NULL);
}

void ten_engine_append_to_in_msgs_queue(ten_engine_t *self,
                                        ten_shared_ptr_t *cmd) {
  TEN_ASSERT(self, "Invalid argument.");
  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is used to be called from threads other than
  // the engine thread.
  TEN_ASSERT(ten_engine_check_integrity(self, false),
             "Invalid use of engine %p.", self);

  TEN_ASSERT(cmd && ten_msg_is_cmd_and_result(cmd), "Should not happen.");

  ten_mutex_lock(self->in_msgs_lock);
  ten_list_push_smart_ptr_back(&self->in_msgs, cmd);
  ten_mutex_unlock(self->in_msgs_lock);

  ten_engine_handle_in_msgs_async(self);
}

void ten_engine_handle_msg(ten_engine_t *self, ten_shared_ptr_t *msg) {
  TEN_ASSERT(self && ten_engine_check_integrity(self, true),
             "Invalid argument.");
  TEN_ASSERT(msg && ten_msg_check_integrity(msg), "Should not happen.");

  if (ten_engine_is_closing(self) &&
      !ten_msg_type_to_handle_when_closing(msg)) {
    // Except some special commands, do not handle messages anymore if the
    // engine is closing.
    return;
  }

  if (ten_msg_is_cmd_and_result(msg)) {
    // Because the command ID is a critical information which is necessary for
    // the correct handling of all command-type messages, we need to assign a
    // command ID to messages which don't have one.
    ten_cmd_base_gen_cmd_id_if_empty(msg);
  }

  ten_error_t err;
  ten_error_init(&err);

  ten_msg_engine_handler_func_t engine_handler =
      ten_msg_info[ten_msg_get_type(msg)].engine_handler;
  if (engine_handler) {
    engine_handler(self, msg, &err);
  }

  ten_error_deinit(&err);
}

void ten_engine_dispatch_msg(ten_engine_t *self, ten_shared_ptr_t *msg) {
  TEN_ASSERT(self && ten_engine_check_integrity(self, true),
             "Should not happen.");
  TEN_ASSERT(msg && ten_msg_check_integrity(msg), "Should not happen.");
  TEN_ASSERT(ten_msg_get_dest_cnt(msg) == 1,
             "When this function is executed, there should be only one "
             "destination remaining in the message's dest.");

  if (ten_engine_is_closing(self)) {
    // Do not dispatch the message if the engine is closing.
    return;
  }

  ten_loc_t *dest_loc = ten_msg_get_first_dest_loc(msg);
  TEN_ASSERT(dest_loc && ten_loc_check_integrity(dest_loc),
             "Should not happen.");

  ten_app_t *app = self->app;
  TEN_ASSERT(app, "Invalid argument.");
  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: The engine might have its own thread, and it is different
  // from the app's thread. When the engine is still alive, the app must also be
  // alive. Furthermore, the app associated with the engine remains unchanged
  // throughout the engine's lifecycle, and the app fields accessed underneath
  // are constant once the app is initialized. Therefore, the use of the app
  // here is considered thread-safe.
  TEN_ASSERT(ten_app_check_integrity(app, false), "Invalid use of app %p.",
             app);

  if (!ten_string_is_equal(&dest_loc->app_uri, ten_app_get_uri(app))) {
    TEN_ASSERT(!ten_string_is_empty(&dest_loc->app_uri),
               "The uri of the app should not be empty.");

    // The message is _not_ for the current TEN app, so route the message to the
    // correct TEN app through the correct remote.
    ten_engine_route_msg_to_remote(self, msg);
  } else {
    // The destination of the message is the current TEN app.

    if (
        // It means asking the current TEN app to do something.
        ten_string_is_empty(&dest_loc->graph_name) ||
        // It means asking another engine in the same app to do something.
        !ten_string_is_equal(&dest_loc->graph_name, &self->graph_name)) {
      // Both of these 2 cases will need the current TEN app to dispatch the
      // message, and the threads of the TEN app and the current TEN engine
      // might be different, so push the message to the command queue of the
      // current TEN app.
      ten_app_push_to_in_msgs_queue(app, msg);
    } else {
      if (ten_string_is_empty(&dest_loc->extension_group_name)) {
        // It means the destination is the current engine, so ask the current
        // engine to handle this message.

        ten_engine_handle_msg(self, msg);
      } else {
        // Find the correct extension thread to handle this message.

        bool found = false;

        ten_list_foreach (&self->extension_context->extension_groups, iter) {
          ten_extension_group_t *extension_group =
              ten_ptr_listnode_get(iter.node);
          TEN_ASSERT(
              extension_group &&
                  // TEN_NOLINTNEXTLINE(thread-check)
                  // thread-check: We are in the engine thread, _not_ in the
                  // extension thread. However, before the engine is closed, the
                  // pointer of the extension group and the pointer of the
                  // extension thread will not be changed, and the closing of
                  // the entire engine must start from the engine, so the
                  // execution to this position means that the engine has not
                  // been closed, so there will be no thread safety issue.
                  ten_extension_group_check_integrity(extension_group, false),
              "Should not happen.");

          if (ten_string_is_equal(&extension_group->name,
                                  &dest_loc->extension_group_name)) {
            // Find the correct extension thread, ask it to handle the message.
            found = true;
            ten_extension_thread_handle_in_msg_async(
                extension_group->extension_thread, msg);
            break;
          }
        }

        if (!found) {
          ten_msg_dump(msg, NULL,
                       "Failed to find the destination extension thread for "
                       "the message ^m");

          ten_shared_ptr_t *status =
              ten_extension_group_create_invalid_dest_status(
                  msg, &dest_loc->extension_group_name);

          ten_engine_dispatch_msg(self, status);

          ten_shared_ptr_destroy(status);
        }
      }
    }
  }
}
