//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/extension_group/msg_interface/common.h"

#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/app/msg_interface/common.h"
#include "include_internal/axis_runtime/engine/engine.h"
#include "include_internal/axis_runtime/engine/msg_interface/common.h"
#include "include_internal/axis_runtime/extension_context/extension_context.h"
#include "include_internal/axis_runtime/extension_group/extension_group.h"
#include "include_internal/axis_runtime/msg/msg.h"

bool axis_extension_group_dispatch_msg(axis_extension_group_t *self,
                                      axis_shared_ptr_t *msg, axis_error_t *err) {
  axis_msg_set_src_to_extension_group(msg, self);

  axis_loc_t *dest_loc = axis_msg_get_first_dest_loc(msg);
  axis_ASSERT(dest_loc && axis_loc_check_integrity(dest_loc) &&
                 axis_msg_get_dest_cnt(msg) == 1,
             "Should not happen.");
  axis_ASSERT(!axis_string_is_empty(&dest_loc->app_uri),
             "App URI should not be empty.");

  axis_extension_context_t *extension_context = self->extension_context;
  axis_ASSERT(extension_context && axis_extension_context_check_integrity(
                                      extension_context, false),
             "Invalid argument.");

  axis_engine_t *engine = extension_context->engine;
  axis_ASSERT(engine && axis_engine_check_integrity(engine, false),
             "Invalid argument.");

  axis_app_t *app = engine->app;
  axis_ASSERT(app && axis_app_check_integrity(app, false), "Invalid argument.");

  if (!axis_string_is_equal_c_str(&dest_loc->app_uri, axis_app_get_uri(app))) {
    // Send to other apps.
    axis_ASSERT(0, "Handle this condition.");
  } else {
    if (axis_string_is_empty(&dest_loc->graph_id)) {
      // It means asking the app to do something.
      axis_ASSERT(0, "Handle this condition.");
    } else if (axis_string_is_equal_c_str(&dest_loc->graph_id,
                                         axis_engine_get_id(engine, false))) {
      if (axis_string_is_empty(&dest_loc->extension_group_name)) {
        // It means asking the engine to do something.
        axis_engine_append_to_in_msgs_queue(engine, msg);
      } else {
        // Send to other extension group in the same engine.
        axis_ASSERT(0, "Handle this condition.");
      }
    } else {
      // Send to other engines in the same app. The message should not be
      // handled in this engine, so ask the app to handle this message.
      axis_app_push_to_in_msgs_queue(app, msg);
    }
  }

  return true;
}
