//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/engine/engine.h"

#include <stdlib.h>

#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/common/loc.h"
#include "include_internal/axis_runtime/connection/connection.h"
#include "include_internal/axis_runtime/engine/internal/extension_interface.h"
#include "include_internal/axis_runtime/engine/internal/thread.h"
#include "include_internal/axis_runtime/engine/msg_interface/common.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/start_graph/cmd.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/protocol/protocol.h"
#include "include_internal/axis_runtime/remote/remote.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "include_internal/axis_utils/log/log.h"
#include "axis_runtime/app/app.h"
#include "axis_utils/container/hash_table.h"
#include "axis_utils/container/list.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/atomic.h"
#include "axis_utils/lib/event.h"
#include "axis_utils/lib/mutex.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/lib/uuid.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/sanitizer/thread_check.h"

bool axis_engine_check_integrity(axis_engine_t *self, bool check_thread) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_ENGINE_SIGNATURE) {
    return false;
  }

  if (check_thread) {
    return axis_sanitizer_thread_check_do_check(&self->thread_check);
  }

  return true;
}

void axis_engine_destroy(axis_engine_t *self) {
  axis_ASSERT(
      self &&
          // axis_NOLINTNEXTLINE(thread-check)
          // thread-check: The belonging thread of the 'engine' is ended when
          // this function is called, so we can not check thread integrity here.
          axis_engine_check_integrity(self, false),
      "Should not happen.");
  axis_ASSERT(axis_app_check_integrity(self->app, true), "Should not happen.");

  // The engine could only be destroyed when there is no extension threads,
  // no prev/next remote apps (connections), no timers associated with it.
  axis_ASSERT(
      (self->extension_context == NULL) && axis_list_is_empty(&self->timers),
      "Should not happen.");

  axis_env_destroy(self->axis_env);

  axis_signature_set(&self->signature, 0);

  axis_hashtable_deinit(&self->remotes);
  axis_list_clear(&self->weak_remotes);

  axis_mutex_destroy(self->extension_msgs_lock);
  axis_list_clear(&self->extension_msgs);

  axis_mutex_destroy(self->in_msgs_lock);
  axis_list_clear(&self->in_msgs);

  if (self->has_own_loop) {
    axis_event_destroy(self->engine_thread_ready_for_migration);
    axis_event_destroy(self->belonging_thread_is_set);

    axis_runloop_destroy(self->loop);
    self->loop = NULL;
  }

  if (self->original_start_graph_cmd_of_enabling_engine) {
    axis_shared_ptr_destroy(self->original_start_graph_cmd_of_enabling_engine);
    self->original_start_graph_cmd_of_enabling_engine = NULL;
  }

  if (self->cmd_stop_graph) {
    axis_shared_ptr_destroy(self->cmd_stop_graph);
    self->cmd_stop_graph = NULL;
  }

  axis_string_deinit(&self->graph_id);

  axis_path_table_destroy(self->path_table);

  axis_sanitizer_thread_check_deinit(&self->thread_check);

  axis_FREE(self);
}

// graph_id is the identify of one graph, so the graph_id in all related
// engines MUST be the same. graph_id will be generated in the first app, and
// will transfer with the message to the next app.
static void axis_engine_set_graph_id(axis_engine_t *self, axis_shared_ptr_t *cmd) {
  axis_ASSERT(self && axis_engine_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(cmd && axis_msg_get_type(cmd) == axis_MSG_TYPE_CMD_START_GRAPH,
             "Should not happen.");

  const axis_string_t *src_uri = &axis_msg_get_src_loc(cmd)->app_uri;
  const axis_string_t *src_graph_id = &axis_msg_get_src_loc(cmd)->graph_id;

  // One app could not have two engines with the same graph_id, so only when
  // the command is from another app, we can use the graph_id attached in that
  // command to be the graph_id of the newly created engine.
  if (!axis_string_is_equal(src_uri, &self->app->uri) &&
      !axis_string_is_empty(src_graph_id)) {
    axis_LOGD("[%s] Inherit engine's name from previous node.",
             axis_string_get_raw_str(src_graph_id));
    axis_string_init_formatted(&self->graph_id, "%s",
                              axis_string_get_raw_str(src_graph_id));
  } else {
    axis_string_t graph_id_str;
    axis_string_init(&graph_id_str);
    axis_uuid4_gen_string(&graph_id_str);

    // Set the newly created graph_id to the engine.
    axis_string_init_formatted(&self->graph_id, "%s",
                              axis_string_get_raw_str(&graph_id_str));

    // Set the newly created graph_id to the 'start_graph' command.
    axis_list_foreach (axis_msg_get_dest(cmd), iter) {
      axis_loc_t *dest_loc = axis_ptr_listnode_get(iter.node);
      axis_ASSERT(dest_loc && axis_loc_check_integrity(dest_loc),
                 "Should not happen.");

      axis_string_set_formatted(&dest_loc->graph_id, "%s",
                               axis_string_get_raw_str(&graph_id_str));
    }

    axis_string_deinit(&graph_id_str);
  }

  // Got graph_id, update the graph_id field of all the extensions_info that
  // this start_graph command has.
  axis_cmd_start_graph_fill_loc_info(cmd, axis_app_get_uri(self->app),
                                    axis_engine_get_id(self, true));
}

bool axis_engine_is_ready_to_handle_msg(axis_engine_t *self) {
  axis_ASSERT(self && axis_engine_check_integrity(self, true),
             "Should not happen.");

  return self->is_ready_to_handle_msg;
}

axis_engine_t *axis_engine_create(axis_app_t *app, axis_shared_ptr_t *cmd) {
  axis_ASSERT(app && axis_app_check_integrity(app, true) && cmd &&
                 axis_cmd_base_check_integrity(cmd),
             "Should not happen.");

  axis_LOGD("Create engine.");

  axis_engine_t *self = (axis_engine_t *)axis_MALLOC(sizeof(axis_engine_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_signature_set(&self->signature, (axis_signature_t)axis_ENGINE_SIGNATURE);
  axis_sanitizer_thread_check_init_with_current_thread(&self->thread_check);

  axis_atomic_store(&self->is_closing, 0);
  self->on_closed = NULL;
  self->on_closed_data = NULL;

  self->app = app;
  self->extension_context = NULL;

  self->loop = NULL;
  self->engine_thread_ready_for_migration = NULL;
  self->belonging_thread_is_set = NULL;
  self->is_ready_to_handle_msg = false;

  axis_hashtable_init(&self->remotes,
                     offsetof(axis_remote_t, hh_in_remote_table));
  axis_list_init(&self->weak_remotes);

  axis_list_init(&self->timers);
  self->path_table =
      axis_path_table_create(axis_PATH_TABLE_ATTACH_TO_ENGINE, self);

  self->extension_msgs_lock = axis_mutex_create();
  axis_list_init(&self->extension_msgs);

  self->in_msgs_lock = axis_mutex_create();
  axis_list_init(&self->in_msgs);

  self->original_start_graph_cmd_of_enabling_engine = NULL;
  self->cmd_stop_graph = NULL;

  self->axis_env = NULL;

  self->long_running_mode = axis_cmd_start_graph_get_long_running_mode(cmd);

  // This is a workaround as the 'close_trigger_gc' in the axis_remote_t is
  // removed.
  //
  // TODO(Liu):
  // 1. Replace 'long_running_mode' in engine with 'cascade_close_upward'.
  // 2. Provide a policy to customize the behavior of 'cascade_close_upward', as
  //    there might be many remotes in the same engine.
  if (!self->long_running_mode) {
    axis_protocol_t *endpoint = app->endpoint_protocol;
    if (endpoint) {
      self->long_running_mode = !axis_protocol_cascade_close_upward(endpoint);
    }
  }

  axis_engine_set_graph_id(self, cmd);

  axis_engine_init_individual_eventloop_relevant_vars(self, app);
  if (self->has_own_loop) {
    axis_engine_create_its_own_thread(self);
  } else {
    // Since the engine does not have its own run loop, it means that it will
    // reuse the app's run loop. Therefore, the current app thread is also the
    // engine thread, allowing us to create the axis_env object here.
    self->axis_env = axis_env_create_for_engine(self);
  }

  return self;
}

axis_runloop_t *axis_engine_get_attached_runloop(axis_engine_t *self) {
  axis_ASSERT(self &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: This function is intended to be called in
                 // different threads.
                 axis_engine_check_integrity(self, false),
             "Should not happen.");

  if (self->has_own_loop) {
    return self->loop;
  } else {
    return axis_app_get_attached_runloop(self->app);
  }
}

const char *axis_engine_get_id(axis_engine_t *self, bool check_thread) {
  axis_ASSERT(self && axis_engine_check_integrity(self, check_thread),
             "Should not happen.");

  return axis_string_get_raw_str(&self->graph_id);
}
