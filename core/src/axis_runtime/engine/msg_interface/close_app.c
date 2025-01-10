//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/engine/msg_interface/close_app.h"

#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/app/close.h"
#include "include_internal/axis_runtime/app/msg_interface/common.h"
#include "include_internal/axis_runtime/engine/engine.h"
#include "include_internal/axis_runtime/engine/internal/thread.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "axis_runtime/app/app.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"

void axis_engine_handle_cmd_close_app(axis_engine_t *self, axis_shared_ptr_t *cmd,
                                     axis_UNUSED axis_error_t *err) {
  axis_ASSERT(self && axis_engine_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(cmd && axis_msg_get_type(cmd) == axis_MSG_TYPE_CMD_CLOSE_APP,
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

  axis_app_push_to_in_msgs_queue(app, cmd);
}
