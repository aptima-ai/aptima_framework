//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/app/msg_interface/common.h"
#include "include_internal/axis_runtime/connection/connection.h"
#include "include_internal/axis_runtime/engine/engine.h"
#include "include_internal/axis_runtime/engine/msg_interface/stop_graph.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/remote/remote.h"
#include "axis_runtime/msg/cmd/stop_graph/cmd.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"

void axis_engine_handle_cmd_stop_graph(axis_engine_t *self, axis_shared_ptr_t *cmd,
                                      axis_UNUSED axis_error_t *err) {
  axis_ASSERT(self && axis_engine_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(cmd && axis_msg_get_type(cmd) == axis_MSG_TYPE_CMD_STOP_GRAPH,
             "Should not happen.");

  const char *graph_id = axis_cmd_stop_graph_get_graph_id(cmd);
  if (graph_id == NULL || !strlen(graph_id) ||
      axis_string_is_equal_c_str(&self->graph_id, graph_id)) {
    // Suicide. Store the stop_graph command temporarily, so that it can be
    // used to return the cmd_result when the engine is shut down later.
    self->cmd_stop_graph = axis_shared_ptr_clone(cmd);

    axis_engine_close_async(self);
  } else {
    // Close other engine. Send the command to app.

    axis_app_t *app = self->app;
    axis_ASSERT(app, "Invalid argument.");
    // axis_NOLINTNEXTLINE(thread-check)
    // thread-check: The engine might have its own thread, and it is different
    // from the app's thread. When the engine is still alive, the app must also
    // be alive. Furthermore, the app associated with the engine remains
    // unchanged throughout the engine's lifecycle, and the app fields accessed
    // underneath are constant once the app is initialized. Therefore, the use
    // of the app here is considered thread-safe.
    axis_ASSERT(axis_app_check_integrity(app, false), "Invalid use of app %p.",
               app);

    axis_app_push_to_in_msgs_queue(app, cmd);
  }
}
