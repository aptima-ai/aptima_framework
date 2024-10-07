//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/app/msg_interface/common.h"

#include "include_internal/ten_runtime/app/app.h"
#include "include_internal/ten_runtime/app/close.h"
#include "include_internal/ten_runtime/app/engine_interface.h"
#include "include_internal/ten_runtime/app/metadata.h"
#include "include_internal/ten_runtime/app/msg_interface/start_graph.h"
#include "include_internal/ten_runtime/app/predefined_graph.h"
#include "include_internal/ten_runtime/common/loc.h"
#include "include_internal/ten_runtime/connection/connection.h"
#include "include_internal/ten_runtime/connection/migration.h"
#include "include_internal/ten_runtime/engine/engine.h"
#include "include_internal/ten_runtime/engine/internal/migration.h"
#include "include_internal/ten_runtime/engine/msg_interface/common.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd/stop_graph/cmd.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd_base.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "include_internal/ten_runtime/protocol/protocol.h"
#include "ten_runtime/app/app.h"
#include "ten_runtime/msg/cmd_result/cmd_result.h"
#include "ten_runtime/msg/msg.h"
#include "ten_utils/container/list.h"
#include "ten_utils/container/list_node.h"
#include "ten_utils/io/runloop.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/lib/smart_ptr.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"
#include "ten_utils/value/value.h"

void ten_app_do_connection_migration_or_push_to_engine_queue(
    ten_connection_t *connection, ten_engine_t *engine, ten_shared_ptr_t *msg) {
  // The 'connection' maybe NULL if the msg comes from other engines.
  if (connection) {
    // TEN_NOLINTNEXTLINE(thread-check)
    // thread-check: This function is called in the app thread. If the
    // connection has been migrated, its belonging thread will be the engine's
    // thread, so we do not check thread integrity here.
    TEN_ASSERT(ten_connection_check_integrity(connection, false),
               "Invalid argument.");
  }

  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: We are in the app thread, and all the uses of the engine in
  // this function would not cause thread safety issues.
  TEN_ASSERT(engine && ten_engine_check_integrity(engine, false),
             "This function is called in the app thread.");

  if (connection && ten_connection_needs_to_migrate(connection, engine)) {
    ten_connection_migrate(connection, engine, msg);
  } else {
    ten_engine_append_to_in_msgs_queue(engine, msg);
  }
}

static bool ten_app_handle_msg_default_handler(ten_app_t *self,
                                               ten_connection_t *connection,
                                               ten_shared_ptr_t *msg,
                                               ten_error_t *err) {
  TEN_ASSERT(self && ten_app_check_integrity(self, true), "Should not happen.");
  TEN_ASSERT(
      msg && ten_msg_check_integrity(msg) && ten_msg_get_dest_cnt(msg) == 1,
      "Should not happen.");

  bool result = true;
  ten_string_t *dest_graph_id = &ten_msg_get_first_dest_loc(msg)->graph_id;

  if (ten_string_is_empty(dest_graph_id)) {
    // This means the destination is the app, however, currently, app doesn't
    // need to do anything, so just return.
    return true;
  }

  // Determine which engine the message should go to.
  ten_engine_t *engine =
      ten_app_get_engine_based_on_dest_graph_id_from_msg(self, msg);

  if (!engine) {
    // Failed to find the engine, check to see if the requested engine is a
    // _singleton_ prebuilt-graph engine, and start it.

    ten_predefined_graph_info_t *predefined_graph_info =
        ten_app_get_singleton_predefined_graph_info_based_on_dest_graph_id_from_msg(
            self, msg);

    if (predefined_graph_info) {
      if (!ten_app_start_predefined_graph(self, predefined_graph_info, err)) {
        TEN_ASSERT(0, "Should not happen.");
        return false;
      }

      engine = predefined_graph_info->engine;
      TEN_ASSERT(engine && ten_engine_check_integrity(engine, false),
                 "Should not happen.");
    }
  }

  if (engine) {
    // The target engine is determined, enable that engine to handle this
    // message.

    // Correct the 'graph_id' from prebuilt-graph-name to engine-graph-id.
    ten_msg_set_dest_engine_if_unspecified_or_predefined_graph_name(
        msg, engine, &self->predefined_graph_infos);

    ten_app_do_connection_migration_or_push_to_engine_queue(connection, engine,
                                                            msg);
  } else {
    // Could not find the engine, what we could do now is to return an error
    // message.

    ten_shared_ptr_t *resp =
        ten_cmd_result_create_from_cmd(TEN_STATUS_CODE_ERROR, msg);
    ten_msg_set_property(resp, "detail",
                         ten_value_create_string("Graph not found."), NULL);
    ten_msg_clear_and_set_dest_from_msg_src(resp, msg);

    if (connection) {
      // The following two functions are desired to be called in order --
      // call 'ten_connection_migration_state_reset_when_engine_not_found()'
      // first.
      //
      // Those two functions perform the following two actions:
      //
      // - ten_connection_migration_state_reset_when_engine_not_found()
      //   Sends a 'on_cleaned' event to the implementation protocol.
      //
      // - ten_connection_send_msg()
      //   Sends the result to the implementation protocol and then the result
      //   will be sent to the client.
      //
      // Supposes that the client sends a request to the app and closes the app
      // once it receives the result. Ex:
      //
      //   auto result = client.send_request_and_get_result();
      //   if (result) {
      //      app.close();
      //   }
      //
      // The closure of the app will send a 'close' event to the implementation
      // protocol. If those two functions are called reversely, the execution
      // sequence might be as follows:
      //
      //    [ client ]               [ app ]                [ protocol ]
      //  send request
      //                      ten_connection_send_msg()
      //                                                 send result to client
      //
      //   close app
      //                                                 receive 'close' event
      //                        reset_migration()
      //                                              receive 'on_cleaned' event
      //
      // We expect the implementation protocol to receive the 'on_cleaned' event
      // before the 'close' event, otherwise the 'close' event will be frozen as
      // the implementation protocol determines that the migration is not
      // completed yet.

      // The 'connection' is not NULL, which means that a message was sent from
      // the client side through an implementation protocol (ex: msgpack or
      // http). The implementation protocol only transfer one message to the app
      // even through it receives more than one message at once, as the
      // connection might need to be migrated and the migration must happen only
      // once. So all the events (ex: the closing event, and the messages) of
      // the implementation protocol will be frozen before the migration is
      // completed or reset.
      //
      // We could not find the engine for this message here, which means this
      // message is the first received by the 'connection', in other words, the
      // connection hasn't started doing migration yet. So we have to reset the
      // migration state, but not mark it as 'DONE', and unfreeze the
      // implementation protocol as it might has some pending tasks (ex: the
      // client disconnects, the implementation protocol needs to be closed).
      ten_connection_migration_state_reset_when_engine_not_found(connection);

      ten_connection_send_msg(connection, resp);
    } else {
      // The 'msg' might be sent from extension A in engine 1 to extension B in
      // engine 2, there is no 'connection' in this case, the cmd result should
      // be sent back to engine A1.
      //
      // So, this cmd result needs to be passed back to the app for further
      // processing.
      result = ten_app_handle_in_msg(self, NULL, resp, err);
    }

    ten_shared_ptr_destroy(resp);
  }

  return result;
}

static bool ten_app_handle_close_app_cmd(ten_app_t *self,
                                         ten_connection_t *connection,
                                         ten_error_t *err) {
  TEN_ASSERT(self && ten_app_check_integrity(self, true), "Should not happen.");

  if (connection) {
    TEN_ASSERT(ten_connection_check_integrity(connection, true),
               "Access across threads.");

    // This is the close_app command, so we do _not_ need to do any migration
    // tasks even if it should be done originally. We can declare that the
    // connection has already be migrated directly.
    ten_connection_upgrade_migration_state_to_done(connection, NULL);
  }

  ten_app_close(self, err);

  return true;
}

static bool ten_app_handle_stop_graph_cmd(ten_app_t *self,
                                          ten_shared_ptr_t *cmd,
                                          TEN_UNUSED ten_error_t *err) {
  TEN_ASSERT(self && ten_app_check_integrity(self, true), "Should not happen.");
  TEN_ASSERT(cmd && ten_cmd_base_check_integrity(cmd), "Should not happen.");
  TEN_ASSERT(ten_msg_get_type(cmd) == TEN_MSG_TYPE_CMD_STOP_GRAPH,
             "Should not happen.");
  TEN_ASSERT(ten_msg_get_dest_cnt(cmd) == 1, "Should not happen.");

  ten_string_t *dest_graph_id = ten_cmd_stop_graph_get_graph_id(cmd);
  TEN_ASSERT(!ten_string_is_empty(dest_graph_id), "Should not happen.");

  ten_engine_t *dest_engine = NULL;

  // Find the engine based on the 'dest_graph_id' in the 'cmd'.
  ten_list_foreach (&self->engines, iter) {
    ten_engine_t *engine = ten_ptr_listnode_get(iter.node);

    if (ten_string_is_equal(&engine->graph_id, dest_graph_id)) {
      dest_engine = engine;
      break;
    }
  }

  if (dest_engine == NULL) {
    // Failed to find the engine by graph_id, send back an error message.
    ten_shared_ptr_t *ret_cmd =
        ten_cmd_result_create_from_cmd(TEN_STATUS_CODE_ERROR, cmd);
    ten_msg_set_property(
        ret_cmd, "detail",
        ten_value_create_string("Failed to find the engine to be shut down."),
        NULL);

    ten_app_push_to_in_msgs_queue(self, ret_cmd);

    ten_shared_ptr_destroy(ret_cmd);
    return true;
  }

  // The engine is found, set the graph_id to the dest loc and send the 'cmd'
  // to the engine.
  ten_list_foreach (ten_msg_get_dest(cmd), iter) {
    ten_loc_t *dest_loc = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(dest_loc && ten_loc_check_integrity(dest_loc),
               "Should not happen.");

    ten_string_set_formatted(&dest_loc->graph_id, "%s",
                             ten_string_get_raw_str(&dest_engine->graph_id));
  }

  ten_engine_append_to_in_msgs_queue(dest_engine, cmd);

  return true;
}

bool ten_app_handle_out_msg(ten_app_t *self, ten_shared_ptr_t *msg,
                            ten_error_t *err) {
  // The source of the out message is the current app.
  ten_msg_set_src_to_app(msg, self);

  ten_loc_t *dest_loc = ten_msg_get_first_dest_loc(msg);
  TEN_ASSERT(dest_loc && ten_loc_check_integrity(dest_loc) &&
                 ten_msg_get_dest_cnt(msg) == 1,
             "Should not happen.");
  TEN_ASSERT(!ten_string_is_empty(&dest_loc->app_uri),
             "App URI should not be empty.");

  if (!ten_string_is_equal_c_str(&dest_loc->app_uri, ten_app_get_uri(self))) {
    TEN_ASSERT(0, "Handle this condition.");
  } else {
    if (ten_string_is_empty(&dest_loc->graph_id)) {
      // It means asking the app to do something.

      ten_app_push_to_in_msgs_queue(self, msg);
      ten_shared_ptr_destroy(msg);
    } else {
      TEN_ASSERT(0, "Handle this condition.");
    }
  }

  return true;
}

bool ten_app_handle_in_msg(ten_app_t *self, ten_connection_t *connection,
                           ten_shared_ptr_t *msg, ten_error_t *err) {
  TEN_ASSERT(self && ten_app_check_integrity(self, true), "Should not happen.");
  TEN_ASSERT(msg && ten_msg_check_integrity(msg), "Should not happen.");

  if (connection) {
    // If there is a 'connection', then it's possible that the connection might
    // need to be migrated, and if the connection is in the migration phase, we
    // can _not_ throw new messages to it. Therefore, we will control the
    // messages flow, to ensure that there will be only one message sent to the
    // app before the migration is completed.
    TEN_ASSERT(ten_connection_check_integrity(connection, true),
               "Access across threads.");

    TEN_CONNECTION_MIGRATION_STATE migration_state =
        ten_connection_get_migration_state(connection);
    TEN_ASSERT(migration_state == TEN_CONNECTION_MIGRATION_STATE_FIRST_MSG ||
                   migration_state == TEN_CONNECTION_MIGRATION_STATE_DONE,
               "Should not happen.");
  }

  switch (ten_msg_get_type(msg)) {
    case TEN_MSG_TYPE_CMD_START_GRAPH:
      return ten_app_handle_start_graph_cmd(self, connection, msg, err);

    case TEN_MSG_TYPE_CMD_CLOSE_APP:
      return ten_app_handle_close_app_cmd(self, connection, err);

    case TEN_MSG_TYPE_CMD_STOP_GRAPH:
      return ten_app_handle_stop_graph_cmd(self, msg, err);

    default:
      return ten_app_handle_msg_default_handler(self, connection, msg, err);
  }
}

static void ten_app_handle_in_msgs_sync(ten_app_t *self) {
  TEN_ASSERT(self && ten_app_check_integrity(self, true), "Should not happen.");

  ten_error_t err;
  ten_error_init(&err);

  ten_list_t in_msgs_ = TEN_LIST_INIT_VAL;

  TEN_UNUSED int rc = ten_mutex_lock(self->in_msgs_lock);
  TEN_ASSERT(!rc, "Should not happen.");

  ten_list_swap(&in_msgs_, &self->in_msgs);

  rc = ten_mutex_unlock(self->in_msgs_lock);
  TEN_ASSERT(!rc, "Should not happen.");

  ten_list_foreach (&in_msgs_, iter) {
    ten_shared_ptr_t *msg = ten_smart_ptr_listnode_get(iter.node);
    TEN_ASSERT(msg && ten_msg_check_integrity(msg) &&
                   !ten_msg_src_is_empty(msg) &&
                   (ten_msg_get_dest_cnt(msg) == 1),
               "Invalid argument.");

    // The 'ten_app_on_external_cmds()' is called in the following two scenarios
    // now.
    //
    // - Some cmds are sent from the extensions in the engine, and the receiver
    //   is the app, ex: the 'close_app' cmd. The value of the cmd's
    //   'original_connection' field is NULL in this case.
    //
    // - Some cmds are sent from one engine, and the receiver is another engine
    //   in the app. The value of the cmd's 'origin_connection' field might or
    //   might not be NULL in this case.
    //
    //   * If the 'cmd' is sent from the extension, the 'origin_connection' is
    //     NULL. Ex: send the following cmd from extension in engine whose
    //     'graph_id' is A.
    //
    //     ten_env.send_json(
    //      {
    //        "_ten": {
    //          "name": "demo",
    //          "dest": [
    //            {
    //              "app": "localhost",
    //              "graph": "graph_id_of_another_engine"
    //            }
    //          ]
    //        }
    //      }
    //    )
    //
    //   * If the 'cmd' is sent to another engine (whose 'graph_id' is B) from
    //     the client side, after the physical connection between the client and
    //     current engine (whose 'graph_id' is A) has been established, the
    //     'origin_connection' is not NULL. Ex: after the client sends the
    //     'start_graph' cmd to the engine whose 'graph_id' is A with the
    //     msgpack protocol, send a cmd to another engine, then the cmd will be
    //     received by engine A firstly.
    //
    //     The 'origin_connection' must belong to the remote in the engine who
    //     receives the cmd from the client side, no matter whether current
    //     engine is the expect receiver or not. Ex: the belonging of the
    //     'origin_connection' in this case is the remote in the engine A.
    //
    // Briefly, the above two scenarios are expected to handle the cmd out of
    // the scope of the engine which the cmd's 'origin_connection' belongs to.
    // And the following function -- 'ten_app_on_msg()' will do connection
    // migration if needed. So the cmd's 'origin_connection' _must_ _not_ be
    // passed to the 'ten_app_on_msg()' function here.
    ten_app_handle_in_msg(self, NULL, msg, &err);
  }

  ten_list_clear(&in_msgs_);

  ten_error_deinit(&err);
}

static void ten_app_handle_in_msgs_task(void *app_, TEN_UNUSED void *arg) {
  ten_app_t *app = (ten_app_t *)app_;
  TEN_ASSERT(app && ten_app_check_integrity(app, true), "Should not happen.");

  ten_app_handle_in_msgs_sync(app);
}

static void ten_app_handle_in_msgs_async(ten_app_t *self) {
  TEN_ASSERT(self &&
                 // TEN_NOLINTNEXTLINE(thread-check)
                 // thread-check: This function is intended to be called outside
                 // of the TEN app thread.
                 ten_app_check_integrity(self, false),
             "Should not happen.");

  ten_runloop_post_task_tail(ten_app_get_attached_runloop(self),
                             ten_app_handle_in_msgs_task, self, NULL);
}

void ten_app_push_to_in_msgs_queue(ten_app_t *self, ten_shared_ptr_t *msg) {
  TEN_ASSERT(self && ten_app_check_integrity(self, false),
             "Should not happen.");
  TEN_ASSERT(msg && ten_msg_is_cmd_and_result(msg), "Invalid argument.");
  TEN_ASSERT(!ten_cmd_base_cmd_id_is_empty(msg), "Invalid argument.");
  TEN_ASSERT(
      ten_msg_get_src_app_uri(msg) && strlen(ten_msg_get_src_app_uri(msg)),
      "Invalid argument.");
  TEN_ASSERT((ten_msg_get_dest_cnt(msg) == 1), "Invalid argument.");

  TEN_UNUSED bool rc = ten_mutex_lock(self->in_msgs_lock);
  TEN_ASSERT(!rc, "Failed to lock.");

  ten_list_push_smart_ptr_back(&self->in_msgs, msg);

  rc = ten_mutex_unlock(self->in_msgs_lock);
  TEN_ASSERT(!rc, "Failed to unlock.");

  ten_app_handle_in_msgs_async(self);
}

ten_connection_t *ten_app_find_src_connection_for_msg(ten_app_t *self,
                                                      ten_shared_ptr_t *msg) {
  TEN_ASSERT(self && ten_app_check_integrity(self, false),
             "Should not happen.");
  TEN_ASSERT(msg && ten_msg_check_integrity(msg), "Invalid argument.");

  const char *src_uri = ten_msg_get_src_app_uri(msg);
  if (strlen(src_uri)) {
    ten_list_foreach (&self->orphan_connections, iter) {
      ten_connection_t *connection = ten_ptr_listnode_get(iter.node);
      TEN_ASSERT(connection && ten_connection_check_integrity(connection, true),
                 "Should not happen.");

      if (ten_string_is_equal_c_str(&connection->protocol->uri, src_uri)) {
        return connection;
      }
    }
  }

  return NULL;
}
