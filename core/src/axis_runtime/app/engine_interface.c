//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/app/engine_interface.h"

#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/app/close.h"
#include "include_internal/axis_runtime/app/predefined_graph.h"
#include "include_internal/axis_runtime/connection/connection.h"
#include "include_internal/axis_runtime/engine/engine.h"
#include "include_internal/axis_runtime/engine/internal/thread.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd_base.h"
#include "include_internal/axis_utils/log/log.h"
#include "axis_utils/io/runloop.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"

static void axis_app_check_termination_when_engine_closed_(void *app_,
                                                          void *engine_) {
  axis_app_t *app = (axis_app_t *)app_;
  axis_ASSERT(app && axis_app_check_integrity(app, true), "Should not happen.");

  axis_engine_t *engine = (axis_engine_t *)engine_;

  axis_app_check_termination_when_engine_closed(app, engine);
}

static void axis_app_check_termination_when_engine_closed_async(
    axis_app_t *self, axis_engine_t *engine) {
  axis_ASSERT(self &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: This function is intended to be called in
                 // different threads.
                 axis_app_check_integrity(self, false),
             "Should not happen.");

  axis_runloop_post_task_tail(axis_app_get_attached_runloop(self),
                             axis_app_check_termination_when_engine_closed_,
                             self, engine);
}

// This function is called in the engine thread.
static void axis_app_on_engine_closed(axis_engine_t *engine,
                                     axis_UNUSED void *on_close_data) {
  axis_ASSERT(engine && axis_engine_check_integrity(engine, true),
             "Should not happen.");

  axis_app_t *app = engine->app;
  axis_ASSERT(app, "Invalid argument.");

  if (engine->has_own_loop) {
    // This is in the engine thread.
    axis_ASSERT(axis_engine_check_integrity(engine, true), "Should not happen.");

    // Because the app needs some information of an engine to do some
    // clean-up, so we can not destroy the engine now. We have to push the
    // engine to the specified list to wait for clean-up from the app.
    axis_app_check_termination_when_engine_closed_async(app, engine);
  } else {
    // This is in the app+thread thread.
    axis_ASSERT(axis_app_check_integrity(app, true), "Should not happen.");
    axis_ASSERT(axis_engine_check_integrity(engine, true), "Should not happen.");

    axis_app_check_termination_when_engine_closed(app, engine);
  }
}

static void axis_app_add_engine(axis_app_t *self, axis_engine_t *engine) {
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Invalid argument.");
  axis_ASSERT(engine, "Should not happen.");

  axis_list_push_ptr_back(&self->engines, engine, NULL);
  axis_engine_set_on_closed(engine, axis_app_on_engine_closed, NULL);
}

axis_engine_t *axis_app_create_engine(axis_app_t *self, axis_shared_ptr_t *cmd) {
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Should not happen.");
  axis_ASSERT(cmd && axis_cmd_base_check_integrity(cmd), "Should not happen.");

  axis_LOGD("[%s] App creates an engine.", axis_app_get_uri(self));

  axis_engine_t *engine = axis_engine_create(self, cmd);
  axis_ASSERT(engine && axis_engine_check_integrity(engine, false),
             "Should not happen.");

  axis_app_add_engine(self, engine);

  return engine;
}

void axis_app_del_engine(axis_app_t *self, axis_engine_t *engine) {
  axis_ASSERT(self && axis_app_check_integrity(self, true) && engine,
             "Should not happen.");

  axis_LOGD("[%s] Remove engine from app.", axis_app_get_uri(self));

  // Always perform this operation in the main thread, so we don't need to use
  // lock to protect this operation.
  axis_list_remove_ptr(&self->engines, engine);
}

static axis_engine_t *axis_app_get_engine_by_graph_id(axis_app_t *self,
                                                    const char *graph_id) {
  axis_ASSERT(self && axis_app_check_integrity(self, true) && graph_id,
             "Should not happen.");

  axis_list_foreach (&self->engines, iter) {
    axis_engine_t *engine = axis_ptr_listnode_get(iter.node);

    if (axis_c_string_is_equal(axis_string_get_raw_str(&engine->graph_id),
                              graph_id)) {
      return engine;
    }
  }

  return NULL;
}

axis_predefined_graph_info_t *
axis_app_get_singleton_predefined_graph_info_based_on_dest_graph_id_from_msg(
    axis_app_t *self, axis_shared_ptr_t *msg) {
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Should not happen.");
  axis_ASSERT(msg && axis_cmd_base_check_integrity(msg), "Should not happen.");
  axis_ASSERT(axis_msg_get_dest_cnt(msg) == 1, "Should not happen.");

  axis_string_t *dest_graph_id = &axis_msg_get_first_dest_loc(msg)->graph_id;

  if (axis_string_is_empty(dest_graph_id)) {
    // There are no destination information in the message, so we don't know
    // which engine this message should go.
    return NULL;
  }

  return axis_app_get_singleton_predefined_graph_info_by_name(
      self, axis_string_get_raw_str(dest_graph_id));
}

axis_engine_t *axis_app_get_engine_based_on_dest_graph_id_from_msg(
    axis_app_t *self, axis_shared_ptr_t *msg) {
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Invalid argument.");
  axis_ASSERT(msg && axis_cmd_base_check_integrity(msg) &&
                 (axis_msg_get_dest_cnt(msg) == 1),
             "Invalid argument.");

  axis_string_t *dest_graph_id = &axis_msg_get_first_dest_loc(msg)->graph_id;

  if (axis_string_is_empty(dest_graph_id)) {
    // There are no destination information in the message, so we don't know
    // which engine this message should go.
    return NULL;
  }

  if (axis_string_is_uuid4(dest_graph_id)) {
    return axis_app_get_engine_by_graph_id(
        self, axis_string_get_raw_str(dest_graph_id));
  }

  // As a last resort, since there can only be one instance of a singleton graph
  // within a process, the engine instance of the singleton graph can be found
  // through the graph_name.
  return axis_app_get_singleton_predefined_graph_engine_by_name(
      self, axis_string_get_raw_str(dest_graph_id));
}
