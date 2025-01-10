//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/engine/msg_interface/timer.h"

#include "include_internal/axis_runtime/engine/engine.h"
#include "include_internal/axis_runtime/engine/msg_interface/common.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/timeout/cmd.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/timer/cmd.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/timer/timer.h"
#include "axis_runtime/msg/cmd_result/cmd_result.h"
#include "axis_runtime/timer/timer.h"
#include "axis_utils/log/log.h"
#include "axis_utils/macro/check.h"

static void axis_engine_timer_on_trigger(axis_timer_t *self,
                                        void *on_trigger_data) {
  axis_engine_t *engine = (axis_engine_t *)on_trigger_data;
  axis_ASSERT(engine && axis_engine_check_integrity(engine, true) && self &&
                 axis_timer_check_integrity(self, true),
             "Should not happen.");

  axis_shared_ptr_t *cmd = axis_cmd_timeout_create(self->id);

  axis_msg_set_src_to_engine(cmd, engine);
  axis_msg_clear_and_set_dest_to_loc(cmd, &self->src_loc);

  axis_engine_dispatch_msg(engine, cmd);

  axis_shared_ptr_destroy(cmd);
}

// NOLINTNEXTLINE(misc-no-recursion)
void axis_engine_handle_cmd_timer(axis_engine_t *self, axis_shared_ptr_t *cmd,
                                 axis_error_t *err) {
  axis_ASSERT(self && axis_engine_check_integrity(self, true) && cmd &&
                 axis_msg_get_type(cmd) == axis_MSG_TYPE_CMD_TIMER,
             "Should not happen.");

  if (axis_engine_is_closing(self)) {
    axis_LOGD("Engine is closing, do not setup timer.");
    return;
  }

  uint32_t timer_id = axis_cmd_timer_get_timer_id(cmd);
  axis_listnode_t *timer_node = axis_list_find_ptr_custom(
      &self->timers,
      // It's kind of a hack in the following line.
      // NOLINTNEXTLINE(performance-no-int-to-ptr)
      (void *)(uintptr_t)timer_id, axis_timer_is_id_equal_to);

  if (timer_node) {
    // Use an existed timer.
    axis_timer_t *timer = axis_ptr_listnode_get(timer_node);
    axis_ASSERT(timer, "Should not happen.");

    if (axis_cmd_timer_get_times(cmd) == axis_TIMER_CANCEL) {
      axis_timer_stop_async(timer);
      axis_timer_close_async(timer);

      // Return a cmd result for the timer cancel command this time.
      axis_shared_ptr_t *ret_cmd =
          axis_cmd_result_create_from_cmd(axis_STATUS_CODE_OK, cmd);
      axis_msg_set_property(ret_cmd, "detail",
                           axis_value_create_string("Operation is success."),
                           NULL);
      axis_engine_dispatch_msg(self, ret_cmd);
      axis_shared_ptr_destroy(ret_cmd);
    } else {
      axis_ASSERT(
          0 && "Should not happen, because if we can find the timer, the timer "
               "must be enabled.",
          "Should not happen.");
    }
  } else {
    // Create a new timer.
    if (axis_cmd_timer_get_times(cmd) != axis_TIMER_CANCEL) {
      axis_timer_t *timer =
          axis_timer_create_with_cmd(cmd, axis_engine_get_attached_runloop(self));

      axis_timer_set_on_triggered(timer, axis_engine_timer_on_trigger, self);
      axis_timer_set_on_closed(timer, axis_engine_on_timer_closed, self);

      // Add the timer to the timer list. The engine will close all the recorded
      // timers in this timer list, and when the timer is closed, the timer
      // would 'destroy' itself, so we don't need to add a destroy function
      // below.
      axis_list_push_ptr_back(&self->timers, timer, NULL);

      axis_timer_enable(timer);

      axis_shared_ptr_t *ret_cmd =
          axis_cmd_result_create_from_cmd(axis_STATUS_CODE_OK, cmd);
      axis_msg_set_property(ret_cmd, "detail",
                           axis_value_create_string("Operation is success."),
                           NULL);

      axis_engine_dispatch_msg(self, ret_cmd);
      axis_shared_ptr_destroy(ret_cmd);
    } else {
      axis_shared_ptr_t *ret_cmd =
          axis_cmd_result_create_from_cmd(axis_STATUS_CODE_ERROR, cmd);
      axis_msg_set_property(
          ret_cmd, "detail",
          axis_value_create_string("Failed to cancel an un-existed timer."),
          NULL);
      axis_engine_dispatch_msg(self, ret_cmd);
      axis_shared_ptr_destroy(ret_cmd);
    }
  }
}
