//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/app/engine_interface.h"

#include "include_internal/ten_runtime/app/app.h"
#include "include_internal/ten_runtime/app/close.h"
#include "include_internal/ten_runtime/app/predefined_graph.h"
#include "include_internal/ten_runtime/connection/connection.h"
#include "include_internal/ten_runtime/engine/engine.h"
#include "include_internal/ten_runtime/engine/internal/thread.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd_base.h"
#include "include_internal/ten_utils/log/log.h"
#include "ten_utils/io/runloop.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"

static void ten_app_check_termination_when_engine_closed_(void *app_,
                                                          void *engine_) {
  ten_app_t *app = (ten_app_t *)app_;
  TEN_ASSERT(app && ten_app_check_integrity(app, true), "Should not happen.");

  ten_engine_t *engine = (ten_engine_t *)engine_;

  ten_app_check_termination_when_engine_closed(app, engine);
}

static void ten_app_check_termination_when_engine_closed_async(
    ten_app_t *self, ten_engine_t *engine) {
  TEN_ASSERT(self &&
                 // TEN_NOLINTNEXTLINE(thread-check)
                 // thread-check: This function is intended to be called in
                 // different threads.
                 ten_app_check_integrity(self, false),
             "Should not happen.");

  ten_runloop_post_task_tail(ten_app_get_attached_runloop(self),
                             ten_app_check_termination_when_engine_closed_,
                             self, engine);
}

// This function is called in the engine thread.
static void ten_app_on_engine_closed(ten_engine_t *engine,
                                     TEN_UNUSED void *on_close_data) {
  TEN_ASSERT(engine && ten_engine_check_integrity(engine, true),
             "Should not happen.");

  ten_app_t *app = engine->app;
  TEN_ASSERT(app, "Invalid argument.");

  if (engine->has_own_loop) {
    // This is in the engine thread.
    TEN_ASSERT(ten_engine_check_integrity(engine, true), "Should not happen.");

    // Because the app needs some information of an engine to do some
    // clean-up, so we can not destroy the engine now. We have to push the
    // engine to the specified list to wait for clean-up from the app.
    ten_app_check_termination_when_engine_closed_async(app, engine);
  } else {
    // This is in the app+thread thread.
    TEN_ASSERT(ten_app_check_integrity(app, true), "Should not happen.");
    TEN_ASSERT(ten_engine_check_integrity(engine, true), "Should not happen.");

    ten_app_check_termination_when_engine_closed(app, engine);
  }
}

static void ten_app_add_engine(ten_app_t *self, ten_engine_t *engine) {
  TEN_ASSERT(self && ten_app_check_integrity(self, true), "Invalid argument.");
  TEN_ASSERT(engine, "Should not happen.");

  ten_list_push_ptr_back(&self->engines, engine, NULL);
  ten_engine_set_on_closed(engine, ten_app_on_engine_closed, NULL);
}

ten_engine_t *ten_app_create_engine(ten_app_t *self, ten_shared_ptr_t *cmd) {
  TEN_ASSERT(self && ten_app_check_integrity(self, true), "Should not happen.");
  TEN_ASSERT(cmd && ten_cmd_base_check_integrity(cmd), "Should not happen.");

  TEN_LOGD("[%s] App creates an engine.", ten_app_get_uri(self));

  ten_engine_t *engine = ten_engine_create(self, cmd);
  TEN_ASSERT(engine && ten_engine_check_integrity(engine, false),
             "Should not happen.");

  ten_app_add_engine(self, engine);

  return engine;
}

void ten_app_del_engine(ten_app_t *self, ten_engine_t *engine) {
  TEN_ASSERT(self && ten_app_check_integrity(self, true) && engine,
             "Should not happen.");

  TEN_LOGD("[%s] Remove engine from app.", ten_app_get_uri(self));

  // Always perform this operation in the main thread, so we don't need to use
  // lock to protect this operation.
  ten_list_remove_ptr(&self->engines, engine);
}

static ten_engine_t *ten_app_get_engine_by_graph_id(ten_app_t *self,
                                                    const char *graph_id) {
  TEN_ASSERT(self && ten_app_check_integrity(self, true) && graph_id,
             "Should not happen.");

  ten_list_foreach (&self->engines, iter) {
    ten_engine_t *engine = ten_ptr_listnode_get(iter.node);

    if (ten_c_string_is_equal(ten_string_get_raw_str(&engine->graph_id),
                              graph_id)) {
      return engine;
    }
  }

  return NULL;
}

ten_predefined_graph_info_t *
ten_app_get_singleton_predefined_graph_info_based_on_dest_graph_id_from_msg(
    ten_app_t *self, ten_shared_ptr_t *msg) {
  TEN_ASSERT(self && ten_app_check_integrity(self, true), "Should not happen.");
  TEN_ASSERT(msg && ten_cmd_base_check_integrity(msg), "Should not happen.");
  TEN_ASSERT(ten_msg_get_dest_cnt(msg) == 1, "Should not happen.");

  ten_string_t *dest_graph_id = &ten_msg_get_first_dest_loc(msg)->graph_id;

  if (ten_string_is_empty(dest_graph_id)) {
    // There are no destination information in the message, so we don't know
    // which engine this message should go.
    return NULL;
  }

  return ten_app_get_singleton_predefined_graph_info_by_name(
      self, ten_string_get_raw_str(dest_graph_id));
}

ten_engine_t *ten_app_get_engine_based_on_dest_graph_id_from_msg(
    ten_app_t *self, ten_shared_ptr_t *msg) {
  TEN_ASSERT(self && ten_app_check_integrity(self, true), "Invalid argument.");
  TEN_ASSERT(msg && ten_cmd_base_check_integrity(msg) &&
                 (ten_msg_get_dest_cnt(msg) == 1),
             "Invalid argument.");

  ten_string_t *dest_graph_id = &ten_msg_get_first_dest_loc(msg)->graph_id;

  if (ten_string_is_empty(dest_graph_id)) {
    // There are no destination information in the message, so we don't know
    // which engine this message should go.
    return NULL;
  }

  if (ten_string_is_uuid4(dest_graph_id)) {
    return ten_app_get_engine_by_graph_id(
        self, ten_string_get_raw_str(dest_graph_id));
  }

  // As a last resort, since there can only be one instance of a singleton graph
  // within a process, the engine instance of the singleton graph can be found
  // through the graph_name.
  return ten_app_get_singleton_predefined_graph_engine_by_name(
      self, ten_string_get_raw_str(dest_graph_id));
}
