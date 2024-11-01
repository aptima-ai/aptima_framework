//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/engine/engine.h"

#include <stdlib.h>

#include "include_internal/ten_runtime/app/app.h"
#include "include_internal/ten_runtime/common/loc.h"
#include "include_internal/ten_runtime/connection/connection.h"
#include "include_internal/ten_runtime/engine/internal/extension_interface.h"
#include "include_internal/ten_runtime/engine/internal/thread.h"
#include "include_internal/ten_runtime/engine/msg_interface/common.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd/start_graph/cmd.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "include_internal/ten_runtime/protocol/protocol.h"
#include "include_internal/ten_runtime/remote/remote.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "include_internal/ten_utils/log/log.h"
#include "ten_runtime/app/app.h"
#include "ten_utils/container/hash_table.h"
#include "ten_utils/container/list.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/atomic.h"
#include "ten_utils/lib/event.h"
#include "ten_utils/lib/mutex.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/lib/uuid.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/sanitizer/thread_check.h"

bool ten_engine_check_integrity(ten_engine_t *self, bool check_thread) {
  TEN_ASSERT(self, "Should not happen.");

  if (ten_signature_get(&self->signature) !=
      (ten_signature_t)TEN_ENGINE_SIGNATURE) {
    return false;
  }

  if (check_thread) {
    return ten_sanitizer_thread_check_do_check(&self->thread_check);
  }

  return true;
}

void ten_engine_destroy(ten_engine_t *self) {
  TEN_ASSERT(
      self &&
          // TEN_NOLINTNEXTLINE(thread-check)
          // thread-check: The belonging thread of the 'engine' is ended when
          // this function is called, so we can not check thread integrity here.
          ten_engine_check_integrity(self, false),
      "Should not happen.");
  TEN_ASSERT(ten_app_check_integrity(self->app, true), "Should not happen.");

  // The engine could only be destroyed when there is no extension threads,
  // no prev/next remote apps (connections), no timers associated with it.
  TEN_ASSERT(
      (self->extension_context == NULL) && ten_list_is_empty(&self->timers),
      "Should not happen.");

  ten_env_destroy(self->ten_env);

  ten_signature_set(&self->signature, 0);

  ten_hashtable_deinit(&self->remotes);
  ten_list_clear(&self->weak_remotes);

  ten_mutex_destroy(self->extension_msgs_lock);
  ten_list_clear(&self->extension_msgs);

  ten_mutex_destroy(self->in_msgs_lock);
  ten_list_clear(&self->in_msgs);

  if (self->has_own_loop) {
    ten_event_destroy(self->engine_thread_ready_for_migration);
    ten_event_destroy(self->belonging_thread_is_set);

    ten_runloop_destroy(self->loop);
    self->loop = NULL;
  }

  if (self->original_start_graph_cmd_of_enabling_engine) {
    ten_shared_ptr_destroy(self->original_start_graph_cmd_of_enabling_engine);
    self->original_start_graph_cmd_of_enabling_engine = NULL;
  }

  if (self->cmd_stop_graph) {
    ten_shared_ptr_destroy(self->cmd_stop_graph);
    self->cmd_stop_graph = NULL;
  }

  ten_string_deinit(&self->graph_id);

  ten_path_table_destroy(self->path_table);

  ten_sanitizer_thread_check_deinit(&self->thread_check);

  TEN_FREE(self);
}

// graph_id is the identify of one graph, so the graph_id in all related
// engines MUST be the same. graph_id will be generated in the first app, and
// will transfer with the message to the next app.
static void ten_engine_set_graph_id(ten_engine_t *self, ten_shared_ptr_t *cmd) {
  TEN_ASSERT(self && ten_engine_check_integrity(self, true),
             "Should not happen.");
  TEN_ASSERT(cmd && ten_msg_get_type(cmd) == TEN_MSG_TYPE_CMD_START_GRAPH,
             "Should not happen.");

  const ten_string_t *src_uri = &ten_msg_get_src_loc(cmd)->app_uri;
  const ten_string_t *src_graph_id = &ten_msg_get_src_loc(cmd)->graph_id;

  // One app could not have two engines with the same graph_id, so only when
  // the command is from another app, we can use the graph_id attached in that
  // command to be the graph_id of the newly created engine.
  if (!ten_string_is_equal(src_uri, &self->app->uri) &&
      !ten_string_is_empty(src_graph_id)) {
    TEN_LOGD("[%s] Inherit engine's name from previous node.",
             ten_string_get_raw_str(src_graph_id));
    ten_string_init_formatted(&self->graph_id, "%s",
                              ten_string_get_raw_str(src_graph_id));
  } else {
    ten_string_t graph_id_str;
    ten_string_init(&graph_id_str);
    ten_uuid4_gen_string(&graph_id_str);

    // Set the newly created graph_id to the engine.
    ten_string_init_formatted(&self->graph_id, "%s",
                              ten_string_get_raw_str(&graph_id_str));

    // Set the newly created graph_id to the 'start_graph' command.
    ten_list_foreach (ten_msg_get_dest(cmd), iter) {
      ten_loc_t *dest_loc = ten_ptr_listnode_get(iter.node);
      TEN_ASSERT(dest_loc && ten_loc_check_integrity(dest_loc),
                 "Should not happen.");

      ten_string_set_formatted(&dest_loc->graph_id, "%s",
                               ten_string_get_raw_str(&graph_id_str));
    }

    ten_string_deinit(&graph_id_str);
  }

  // Got graph_id, update the graph_id field of all the extensions_info that
  // this start_graph command has.
  ten_cmd_start_graph_fill_loc_info(cmd, ten_app_get_uri(self->app),
                                    ten_engine_get_id(self, true));
}

bool ten_engine_is_ready_to_handle_msg(ten_engine_t *self) {
  TEN_ASSERT(self && ten_engine_check_integrity(self, true),
             "Should not happen.");

  return self->is_ready_to_handle_msg;
}

ten_engine_t *ten_engine_create(ten_app_t *app, ten_shared_ptr_t *cmd) {
  TEN_ASSERT(app && ten_app_check_integrity(app, true) && cmd &&
                 ten_cmd_base_check_integrity(cmd),
             "Should not happen.");

  TEN_LOGD("Create engine.");

  ten_engine_t *self = (ten_engine_t *)TEN_MALLOC(sizeof(ten_engine_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  ten_signature_set(&self->signature, (ten_signature_t)TEN_ENGINE_SIGNATURE);
  ten_sanitizer_thread_check_init_with_current_thread(&self->thread_check);

  ten_atomic_store(&self->is_closing, 0);
  self->on_closed = NULL;
  self->on_closed_data = NULL;

  self->app = app;
  self->extension_context = NULL;

  self->loop = NULL;
  self->engine_thread_ready_for_migration = NULL;
  self->belonging_thread_is_set = NULL;
  self->is_ready_to_handle_msg = false;

  ten_hashtable_init(&self->remotes,
                     offsetof(ten_remote_t, hh_in_remote_table));
  ten_list_init(&self->weak_remotes);

  ten_list_init(&self->timers);
  self->path_table =
      ten_path_table_create(TEN_PATH_TABLE_ATTACH_TO_ENGINE, self);

  self->extension_msgs_lock = ten_mutex_create();
  ten_list_init(&self->extension_msgs);

  self->in_msgs_lock = ten_mutex_create();
  ten_list_init(&self->in_msgs);

  self->original_start_graph_cmd_of_enabling_engine = NULL;
  self->cmd_stop_graph = NULL;

  self->ten_env = NULL;

  self->long_running_mode = ten_cmd_start_graph_get_long_running_mode(cmd);

  // This is a workaround as the 'close_trigger_gc' in the ten_remote_t is
  // removed.
  //
  // TODO(Liu):
  // 1. Replace 'long_running_mode' in engine with 'cascade_close_upward'.
  // 2. Provide a policy to customize the behavior of 'cascade_close_upward', as
  //    there might be many remotes in the same engine.
  if (!self->long_running_mode) {
    ten_protocol_t *endpoint = app->endpoint_protocol;
    if (endpoint) {
      self->long_running_mode = !ten_protocol_cascade_close_upward(endpoint);
    }
  }

  ten_engine_set_graph_id(self, cmd);

  ten_engine_init_individual_eventloop_relevant_vars(self, app);
  if (self->has_own_loop) {
    ten_engine_create_its_own_thread(self);
  } else {
    // Since the engine does not have its own run loop, it means that it will
    // reuse the app's run loop. Therefore, the current app thread is also the
    // engine thread, allowing us to create the ten_env object here.
    self->ten_env = ten_env_create_for_engine(self);
  }

  return self;
}

ten_runloop_t *ten_engine_get_attached_runloop(ten_engine_t *self) {
  TEN_ASSERT(self &&
                 // TEN_NOLINTNEXTLINE(thread-check)
                 // thread-check: This function is intended to be called in
                 // different threads.
                 ten_engine_check_integrity(self, false),
             "Should not happen.");

  if (self->has_own_loop) {
    return self->loop;
  } else {
    return ten_app_get_attached_runloop(self->app);
  }
}

const char *ten_engine_get_id(ten_engine_t *self, bool check_thread) {
  TEN_ASSERT(self && ten_engine_check_integrity(self, check_thread),
             "Should not happen.");

  return ten_string_get_raw_str(&self->graph_id);
}
