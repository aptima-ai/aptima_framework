//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/engine/internal/remote_interface.h"

#include <stdlib.h>

#include "include_internal/axis_runtime/addon/protocol/protocol.h"
#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/common/loc.h"
#include "include_internal/axis_runtime/connection/connection.h"
#include "include_internal/axis_runtime/connection/migration.h"
#include "include_internal/axis_runtime/engine/engine.h"
#include "include_internal/axis_runtime/engine/internal/thread.h"
#include "include_internal/axis_runtime/engine/msg_interface/common.h"
#include "include_internal/axis_runtime/engine/msg_interface/start_graph.h"
#include "include_internal/axis_runtime/extension_context/extension_context.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/protocol/protocol.h"
#include "include_internal/axis_runtime/remote/remote.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "axis_runtime/app/app.h"
#include "axis_runtime/common/status_code.h"
#include "axis_runtime/msg/cmd_result/cmd_result.h"
#include "axis_utils/container/list_node.h"
#include "axis_utils/container/list_node_ptr.h"
#include "axis_utils/container/list_ptr.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/field.h"
#include "axis_utils/macro/mark.h"
#include "axis_utils/macro/memory.h"

static bool axis_engine_del_weak_remote(axis_engine_t *self,
                                       axis_remote_t *remote) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_engine_check_integrity(self, true),
             "Invalid use of engine %p.", self);

  axis_ASSERT(remote, "Invalid argument.");
  axis_ASSERT(axis_remote_check_integrity(remote, true),
             "Invalid use of remote %p.", remote);

  bool success = axis_list_remove_ptr(&self->weak_remotes, remote);

  axis_LOGV("Delete remote %p from weak list: %s", remote,
           success ? "success." : "failed.");

  return success;
}

static size_t axis_engine_weak_remotes_cnt_in_specified_uri(axis_engine_t *self,
                                                           const char *uri) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_engine_check_integrity(self, true),
             "Invalid use of engine %p.", self);

  size_t cnt = axis_list_find_ptr_cnt_custom(&self->weak_remotes, uri,
                                            axis_remote_is_uri_equal_to);

  axis_LOGV("weak remote cnt for %s: %zu", uri, cnt);

  return cnt;
}

static axis_engine_on_protocol_created_ctx_t *
axis_engine_on_protocol_created_ctx_create(axis_engine_on_remote_created_cb_t cb,
                                          void *user_data) {
  axis_engine_on_protocol_created_ctx_t *self =
      (axis_engine_on_protocol_created_ctx_t *)axis_MALLOC(
          sizeof(axis_engine_on_protocol_created_ctx_t));

  self->cb = cb;
  self->user_data = user_data;

  return self;
}

static void axis_engine_on_protocol_created_ctx_destroy(
    axis_engine_on_protocol_created_ctx_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_FREE(self);
}

void axis_engine_on_remote_closed(axis_remote_t *remote, void *on_closed_data) {
  axis_ASSERT(remote && on_closed_data, "Should not happen.");

  axis_engine_t *self = (axis_engine_t *)on_closed_data;
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_engine_check_integrity(self, true),
             "Invalid use of engine %p.", self);

  axis_ASSERT(axis_engine_weak_remotes_cnt_in_specified_uri(
                 self, axis_string_get_raw_str(&remote->uri)) <= 1,
             "There should be at most 1 weak remote of the specified uri.");

  bool is_weak = axis_engine_del_weak_remote(self, remote);
  if (is_weak) {
    // The closing of a weak remote is a normal case which should not trigger
    // the closing of the engine. Therefore, we just destroy the remote.
    axis_remote_destroy(remote);
  } else {
    bool found_in_remotes = false;

    axis_hashhandle_t *connected_remote_hh = axis_hashtable_find_string(
        &self->remotes, axis_string_get_raw_str(&remote->uri));
    if (connected_remote_hh) {
      axis_remote_t *connected_remote = CONTAINER_OF_FROM_FIELD(
          connected_remote_hh, axis_remote_t, hh_in_remote_table);
      axis_ASSERT(connected_remote, "Invalid argument.");
      axis_ASSERT(axis_remote_check_integrity(connected_remote, true),
                 "Invalid use of remote %p.", connected_remote);

      if (connected_remote == remote) {
        found_in_remotes = true;

        // The remote is in the 'remotes' list, we just remove it.
        axis_hashtable_del(&self->remotes, connected_remote_hh);
      } else {
        // Search the engine's remotes using the URI and find that there is
        // already another remote instance present. This situation can occur in
        // the case of a duplicated remote.
      }
    }

    if (!found_in_remotes) {
      axis_LOGI("The remote %p is not found in the 'remotes' list.", remote);

      // The remote is not in the 'remotes' list, we just destroy it.
      axis_remote_destroy(remote);
      return;
    }
  }

  if (axis_engine_is_closing(self)) {
    // Proceed to close the engine.
    axis_engine_on_close(self);
  } else {
    if (!is_weak && !self->long_running_mode) {
      // The closing of any remote would trigger the closing of the engine. If
      // we don't want this behavior, comment out the following line.
      axis_engine_close_async(self);
    }
  }
}

static void axis_engine_add_remote(axis_engine_t *self, axis_remote_t *remote) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_engine_check_integrity(self, true),
             "Invalid use of engine %p.", self);

  axis_ASSERT(remote, "Invalid argument.");
  axis_ASSERT(axis_remote_check_integrity(remote, true),
             "Invalid use of remote %p.", remote);

  axis_LOGD("[%s] Add %s (%p) as remote.", axis_app_get_uri(self->app),
           axis_string_get_raw_str(&remote->uri), remote);

  axis_hashtable_add_string(&self->remotes, &remote->hh_in_remote_table,
                           axis_string_get_raw_str(&remote->uri),
                           axis_remote_destroy);
}

static void axis_engine_add_weak_remote(axis_engine_t *self,
                                       axis_remote_t *remote) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_engine_check_integrity(self, true),
             "Invalid use of engine %p.", self);

  axis_ASSERT(remote, "Invalid argument.");
  axis_ASSERT(axis_remote_check_integrity(remote, true),
             "Invalid use of remote %p.", remote);

  axis_LOGD("[%s] Add %s (%p) as weak remote.", axis_app_get_uri(self->app),
           axis_string_get_raw_str(&remote->uri), remote);

  axis_UNUSED axis_listnode_t *found = axis_list_find_ptr_custom(
      &self->weak_remotes, axis_string_get_raw_str(&remote->uri),
      axis_remote_is_uri_equal_to);
  axis_ASSERT(!found, "There should be at most 1 weak remote of %s.",
             axis_string_get_raw_str(&remote->uri));

  // Do not set 'axis_remote_destroy' as the destroy function, because we might
  // _move_ a weak remote out of 'weak_remotes' list when we ensure it is not
  // duplicated.
  axis_list_push_ptr_back(&self->weak_remotes, remote, NULL);
}

void axis_engine_upgrade_weak_remote_to_normal_remote(axis_engine_t *self,
                                                     axis_remote_t *remote) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_engine_check_integrity(self, true),
             "Invalid use of engine %p.", self);

  axis_ASSERT(remote, "Invalid argument.");
  axis_ASSERT(axis_remote_check_integrity(remote, true),
             "Invalid use of remote %p.", remote);

  axis_engine_del_weak_remote(self, remote);
  axis_engine_add_remote(self, remote);
}

static axis_remote_t *axis_engine_find_remote(axis_engine_t *self,
                                            const char *uri) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_engine_check_integrity(self, true),
             "Invalid use of engine %p.", self);

  axis_ASSERT(uri, "Should not happen.");

  axis_hashhandle_t *hh = axis_hashtable_find_string(&self->remotes, uri);
  if (hh) {
    return CONTAINER_OF_FROM_FIELD(hh, axis_remote_t, hh_in_remote_table);
  }

  return NULL;
}

void axis_engine_link_connection_to_remote(axis_engine_t *self,
                                          axis_connection_t *connection,
                                          const char *uri) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_engine_check_integrity(self, true),
             "Invalid use of engine %p.", self);

  axis_ASSERT(connection, "Invalid argument.");
  axis_ASSERT(axis_connection_check_integrity(connection, true),
             "Invalid use of engine %p.", connection);

  axis_ASSERT(uri, "Invalid argument.");

  axis_remote_t *remote = axis_engine_find_remote(self, uri);
  axis_ASSERT(
      !remote,
      "The relationship of remote and connection should be 1-1 mapping.");

  remote = axis_remote_create_for_engine(uri, self, connection);
  axis_engine_add_remote(self, remote);
}

static void axis_engine_on_remote_protocol_created(axis_env_t *axis_env,
                                                  axis_protocol_t *protocol,
                                                  void *cb_data) {
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_engine_t *self = axis_env_get_attached_engine(axis_env);
  axis_ASSERT(self && axis_engine_check_integrity(self, true),
             "Should not happen.");

  axis_engine_on_protocol_created_ctx_t *ctx =
      (axis_engine_on_protocol_created_ctx_t *)cb_data;

  axis_connection_t *connection = axis_connection_create(protocol);
  axis_ASSERT(connection, "Should not happen.");

  // This is in the 'connect_to' stage, the 'connection' already attaches to the
  // engine, no migration is needed.
  axis_connection_set_migration_state(connection,
                                     axis_CONNECTION_MIGRATION_STATE_DONE);

  axis_remote_t *remote = axis_remote_create_for_engine(
      axis_string_get_raw_str(&protocol->uri), self, connection);
  axis_ASSERT(remote, "Should not happen.");

  if (ctx->cb) {
    ctx->cb(self, remote, ctx->user_data);
  }

  axis_engine_on_protocol_created_ctx_destroy(ctx);
}

static bool axis_engine_create_remote_async(
    axis_engine_t *self, const char *uri,
    axis_engine_on_remote_created_cb_t on_remote_created_cb, void *cb_data) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_engine_check_integrity(self, true),
             "Invalid use of engine %p.", self);
  axis_ASSERT(on_remote_created_cb, "Invalid argument.");
  axis_ASSERT(uri, "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  axis_engine_on_protocol_created_ctx_t *ctx =
      axis_engine_on_protocol_created_ctx_create(on_remote_created_cb, cb_data);
  axis_ASSERT(ctx, "Failed to allocate memory.");

  bool rc = axis_addon_create_protocol_with_uri(
      self->axis_env, uri, axis_PROTOCOL_ROLE_OUT_DEFAULT,
      axis_engine_on_remote_protocol_created, ctx, &err);
  if (!rc) {
    axis_LOGE("Failed to create protocol for %s. err: %s", uri,
             axis_error_errmsg(&err));
    axis_error_deinit(&err);
    axis_engine_on_protocol_created_ctx_destroy(ctx);
    return false;
  }

  axis_error_deinit(&err);

  return true;
}

/**
 * @brief The remote is connected successfully, it's time to send out the
 * message which was going to be sent originally.
 */
static void axis_engine_on_graph_remote_connected(axis_remote_t *self,
                                                 axis_shared_ptr_t *cmd) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_remote_check_integrity(self, true),
             "Invalid use of remote %p.", self);

  axis_ASSERT(self->connection && axis_connection_attach_to(self->connection) ==
                                     axis_CONNECTION_ATTACH_TO_REMOTE,
             "Should not happen.");

  axis_ASSERT(self->connection->protocol &&
                 axis_protocol_attach_to(self->connection->protocol) ==
                     axis_PROTOCOL_ATTACH_TO_CONNECTION,
             "Should not happen.");

  axis_ASSERT(cmd && axis_msg_check_integrity(cmd), "Invalid argument.");

  axis_protocol_send_msg(self->connection->protocol, cmd);

  axis_shared_ptr_destroy(cmd);
  self->on_server_connected_cmd = NULL;
}

static void axis_engine_on_graph_remote_connect_error(axis_remote_t *self,
                                                     axis_shared_ptr_t *cmd) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_remote_check_integrity(self, true),
             "Invalid use of remote %p.", self);

  axis_ASSERT(cmd && axis_msg_check_integrity(cmd), "Invalid argument.");

  // Failed to connect to remote, we must to delete (dereference) the message
  // which was going to be sent originally to prevent from memory leakage.
  axis_shared_ptr_destroy(cmd);
  self->on_server_connected_cmd = NULL;

  if (self->engine->original_start_graph_cmd_of_enabling_engine) {
    // Before starting the extension system, the only reason to establish a
    // connection is to handle the 'start_graph' command, and this variable
    // enables us to know it is this case.
    axis_engine_return_error_for_cmd_start_graph(
        self->engine, self->engine->original_start_graph_cmd_of_enabling_engine,
        "Failed to connect to %s", axis_string_get_raw_str(&self->uri));
  } else {
    // The remote fails to connect to the target, so it's time to close it.
    axis_remote_close(self);
  }
}

static void axis_engine_connect_to_remote_after_remote_is_created(
    axis_engine_t *engine, axis_remote_t *remote, void *user_data) {
  axis_ASSERT(engine && axis_engine_check_integrity(engine, true),
             "Invalid argument.");

  axis_shared_ptr_t *start_graph_cmd = (axis_shared_ptr_t *)user_data;
  axis_ASSERT(start_graph_cmd && axis_msg_check_integrity(start_graph_cmd),
             "Invalid argument.");

  if (!remote) {
    if (engine->original_start_graph_cmd_of_enabling_engine) {
      axis_engine_return_error_for_cmd_start_graph(
          engine, engine->original_start_graph_cmd_of_enabling_engine,
          "Failed to create remote for %s",
          axis_msg_get_first_dest_uri(start_graph_cmd));

      axis_shared_ptr_destroy(start_graph_cmd);
    }
    return;
  }

  axis_ASSERT(remote && axis_remote_check_integrity(remote, true),
             "Invalid use of remote %p.", remote);

  if (axis_engine_check_remote_is_duplicated(
          engine, axis_string_get_raw_str(&remote->uri))) {
    // Since the remote_t creation is asynchronous, the engine may have already
    // established a new connection with the remote during the creation process.
    // If it is found that a connection is about to be duplicated, the remote_t
    // object can be directly destroyed as the physical connection has not
    // actually been established yet.
    // Additionally, there is no need to send the 'start_graph' command to the
    // remote, as the graph must have already been started on the remote side.
    axis_LOGD("Destroy remote %p for %s because it's duplicated.", remote,
             axis_string_get_raw_str(&remote->uri));

    if (engine->original_start_graph_cmd_of_enabling_engine) {
      axis_engine_return_ok_for_cmd_start_graph(
          engine, engine->original_start_graph_cmd_of_enabling_engine);
    }

    axis_remote_close(remote);
    axis_shared_ptr_destroy(start_graph_cmd);
    return;
  }

  // This channel might be duplicated with other channels between this APTIMA app
  // and the remote APTIMA app. This situation may appear in a graph which
  // contains loops.
  //
  //                   ------->
  //  ----> APTIMA app 1            APTIMA app 2 <-----
  //                   <-------
  //
  // If it's this case, this 'channel' would be destroyed later, so we put
  // this 'channel' (i.e., the 'remote' here) to a tmp list (weak remotes)
  // first to prevent any messages from flowing through this channel.
  axis_engine_add_weak_remote(engine, remote);

  axis_remote_connect_to(remote, axis_engine_on_graph_remote_connected,
                        start_graph_cmd,
                        axis_engine_on_graph_remote_connect_error);

  axis_shared_ptr_destroy(start_graph_cmd);
}

bool axis_engine_connect_to_graph_remote(axis_engine_t *self, const char *uri,
                                        axis_shared_ptr_t *cmd) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_engine_check_integrity(self, true),
             "Invalid use of engine %p.", self);
  axis_ASSERT(uri, "Invalid argument.");
  axis_ASSERT(cmd && axis_msg_get_type(cmd) == axis_MSG_TYPE_CMD_START_GRAPH,
             "Should not happen.");

  axis_LOGD("Trying to connect to %s inside graph.", uri);

  return axis_engine_create_remote_async(
      self, uri, axis_engine_connect_to_remote_after_remote_is_created, cmd);
}

void axis_engine_route_msg_to_remote(axis_engine_t *self, axis_shared_ptr_t *msg) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_engine_check_integrity(self, true),
             "Invalid use of engine %p.", self);

  axis_ASSERT(
      msg && axis_msg_check_integrity(msg) && axis_msg_get_dest_cnt(msg) == 1,
      "Should not happen.");

  const char *dest_uri = axis_msg_get_first_dest_uri(msg);
  axis_remote_t *remote = axis_engine_find_remote(self, dest_uri);

  if (remote) {
    axis_remote_send_msg(remote, msg);
  } else {
    axis_LOGW("Could not find suitable remote based on uri: %s", dest_uri);
  }

  // It's unnecessary to search weak remotes, because weak remotes are not
  // ready to transfer messages.
}

axis_remote_t *axis_engine_check_remote_is_existed(axis_engine_t *self,
                                                 const char *uri) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_engine_check_integrity(self, true),
             "Invalid use of engine %p.", self);

  axis_ASSERT(uri, "Should not happen.");

  axis_remote_t *remote = NULL;

  // 1. Check if the remote is in the 'remotes' list.
  axis_hashhandle_t *hh = axis_hashtable_find_string(&self->remotes, uri);
  if (hh) {
#if defined(_DEBUG)
    size_t weak_remote_cnt = axis_list_find_ptr_cnt_custom(
        &self->weak_remotes, uri, axis_remote_is_uri_equal_to);
    // A remote might appear in 'remote' and 'weak_remote' once when the graph
    // contains a loop. This is the case of 'duplicate' connection.
    axis_ASSERT(weak_remote_cnt <= 1, "Invalid numbers of weak remotes");
#endif

    remote = CONTAINER_OF_FROM_FIELD(hh, axis_remote_t, hh_in_remote_table);
    axis_ASSERT(remote, "Invalid argument.");
    axis_ASSERT(axis_remote_check_integrity(remote, true),
               "Invalid use of remote %p.", remote);

    axis_LOGD("remote %p for uri '%s' is found in 'remotes' list.", remote, uri);

    return remote;
  }

  // 2. Check if the remote is in the 'weak_remotes' list.
  axis_listnode_t *found = axis_list_find_ptr_custom(&self->weak_remotes, uri,
                                                   axis_remote_is_uri_equal_to);

  if (found) {
    remote = axis_ptr_listnode_get(found);
    axis_ASSERT(remote, "Invalid argument.");
    axis_ASSERT(axis_remote_check_integrity(remote, true),
               "Invalid use of remote %p.", remote);
  }

  axis_LOGD("remote %p for uri '%s' is%s in 'weak_remotes' list.", remote, uri,
           remote ? "" : " not");

  return remote;
}

// This function is used to solve the connection duplication problem. If there
// are two physical connections between two APTIMA apps, the connection which
// connects a APTIMA app with a smaller URI to a APTIMA app with a larger URI would be
// kept, and the other connection would be dropped.
//
//                   ------->
//  ----> APTIMA app 1            APTIMA app 2 <----
//                   <-------
bool axis_engine_check_remote_is_duplicated(axis_engine_t *self,
                                           const char *uri) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_engine_check_integrity(self, true),
             "Invalid use of engine %p.", self);

  axis_ASSERT(uri, "Should not happen.");

  axis_remote_t *remote = axis_engine_check_remote_is_existed(self, uri);
  if (remote) {
    axis_LOGW("Found a remote %s (%p), checking duplication...", uri, remote);

    if (axis_c_string_is_equal_or_smaller(uri, axis_app_get_uri(self->app))) {
      axis_LOGW(" > Remote %s (%p) is smaller, this channel is duplicated.", uri,
               remote);
      return true;
    } else {
      axis_LOGW(" > Remote %s (%p) is larger, keep this channel.", uri, remote);
    }
  }

  return false;
}

bool axis_engine_check_remote_is_weak(axis_engine_t *self, axis_remote_t *remote) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_engine_check_integrity(self, true),
             "Invalid use of engine %p.", self);

  axis_ASSERT(remote, "Invalid argument.");
  axis_ASSERT(axis_remote_check_integrity(remote, true),
             "Invalid use of remote %p.", remote);

  axis_listnode_t *found = axis_list_find_ptr(&self->weak_remotes, remote);

  axis_LOGD("remote %p is%s weak.", remote, found ? "" : " not");

  return found != NULL;
}

bool axis_engine_receive_msg_from_remote(axis_remote_t *remote,
                                        axis_shared_ptr_t *msg,
                                        axis_UNUSED void *user_data) {
  axis_ASSERT(remote && axis_remote_check_integrity(remote, true),
             "Should not happen.");

  axis_engine_t *engine = remote->engine;
  axis_ASSERT(engine, "Invalid argument.");
  axis_ASSERT(axis_engine_check_integrity(engine, true),
             "Invalid use of engine %p.", engine);

  // Assign the current engine as the message _source_ if there is none, so
  // that if this message traverse to another graph, the result could find the
  // way home.
  axis_msg_set_src_engine_if_unspecified(msg, engine);

  if (!axis_loc_is_empty(&remote->explicit_dest_loc)) {
    // If APTIMA runtime has explicitly setup the destination location where all
    // the messages coming from this remote should go, adjust the destination of
    // the message according to this.

    axis_msg_clear_and_set_dest_to_loc(msg, &remote->explicit_dest_loc);
  } else {
    // The default destination engine would be the engine where this remote
    // attached to, if the message doesn't specify one.
    axis_msg_set_dest_engine_if_unspecified_or_predefined_graph_name(
        msg, engine, &engine->app->predefined_graph_infos);
  }

  if (axis_engine_is_ready_to_handle_msg(engine)) {
    axis_engine_dispatch_msg(engine, msg);
  } else {
    switch (axis_msg_get_type(msg)) {
      case axis_MSG_TYPE_CMD_START_GRAPH: {
        // The 'start_graph' command could only be handled once in a graph.
        // Therefore, if we receive a new 'start_graph' command after the graph
        // has been established, just ignore this 'start_graph' command.

        axis_shared_ptr_t *cmd_result =
            axis_cmd_result_create_from_cmd(axis_STATUS_CODE_ERROR, msg);
        axis_msg_set_property(
            cmd_result, "detail",
            axis_value_create_string(
                "Receive a start_graph cmd after graph is built."),
            NULL);
        axis_connection_send_msg(remote->connection, cmd_result);
        axis_shared_ptr_destroy(cmd_result);
        break;
      }

      case axis_MSG_TYPE_CMD_RESULT:
        axis_engine_dispatch_msg(engine, msg);
        break;

      default:
        axis_engine_append_to_in_msgs_queue(engine, msg);
        break;
    }
  }

  return true;
}
