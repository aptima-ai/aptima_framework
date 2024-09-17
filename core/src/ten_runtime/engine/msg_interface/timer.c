//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/engine/msg_interface/timer.h"

#include "include_internal/ten_runtime/engine/engine.h"
#include "include_internal/ten_runtime/engine/msg_interface/common.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd/timeout/cmd.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd/timer/cmd.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "include_internal/ten_runtime/timer/timer.h"
#include "ten_runtime/msg/cmd_result/cmd_result.h"
#include "ten_runtime/timer/timer.h"
#include "ten_utils/macro/check.h"

static void ten_engine_timer_on_trigger(ten_timer_t *self,
                                        void *on_trigger_data) {
  ten_engine_t *engine = (ten_engine_t *)on_trigger_data;
  TEN_ASSERT(engine && ten_engine_check_integrity(engine, true) && self &&
                 ten_timer_check_integrity(self, true),
             "Should not happen.");

  ten_shared_ptr_t *cmd = ten_cmd_timeout_create(self->id);

  ten_msg_set_src_to_engine(cmd, engine);
  ten_msg_clear_and_set_dest_to_loc(cmd, &self->src_loc);

  ten_engine_dispatch_msg(engine, cmd);

  ten_shared_ptr_destroy(cmd);
}

// NOLINTNEXTLINE(misc-no-recursion)
void ten_engine_handle_cmd_timer(ten_engine_t *self, ten_shared_ptr_t *cmd,
                                 ten_error_t *err) {
  TEN_ASSERT(self && ten_engine_check_integrity(self, true) && cmd &&
                 ten_msg_get_type(cmd) == TEN_MSG_TYPE_CMD_TIMER,
             "Should not happen.");

  if (ten_engine_is_closing(self)) {
    TEN_LOGD("Engine is closing, do not setup timer.");
    return;
  }

  uint32_t timer_id = ten_cmd_timer_get_timer_id(cmd);
  ten_listnode_t *timer_node = ten_list_find_ptr_custom(
      &self->timers,
      // It's kind of a hack in the following line.
      // NOLINTNEXTLINE(performance-no-int-to-ptr)
      (void *)(uintptr_t)timer_id, ten_timer_is_id_equal_to);

  if (timer_node) {
    // Use an existed timer.
    ten_timer_t *timer = ten_ptr_listnode_get(timer_node);
    TEN_ASSERT(timer, "Should not happen.");

    if (ten_cmd_timer_get_times(cmd) == TEN_TIMER_CANCEL) {
      ten_timer_stop_async(timer);
      ten_timer_close_async(timer);

      // Return a cmd result for the timer cancel command this time.
      ten_shared_ptr_t *ret_cmd =
          ten_cmd_result_create_from_cmd(TEN_STATUS_CODE_OK, cmd);
      ten_msg_set_property(ret_cmd, "detail",
                           ten_value_create_string("Operation is success."),
                           NULL);
      ten_engine_dispatch_msg(self, ret_cmd);
      ten_shared_ptr_destroy(ret_cmd);
    } else {
      TEN_ASSERT(
          0 && "Should not happen, because if we can find the timer, the timer "
               "must be enabled.",
          "Should not happen.");
    }
  } else {
    // Create a new timer.
    if (ten_cmd_timer_get_times(cmd) != TEN_TIMER_CANCEL) {
      ten_timer_t *timer =
          ten_timer_create_with_cmd(cmd, ten_engine_get_attached_runloop(self));

      ten_timer_set_on_triggered(timer, ten_engine_timer_on_trigger, self);
      ten_timer_set_on_closed(timer, ten_engine_on_timer_closed, self);

      // Add the timer to the timer list. The engine will close all the recorded
      // timers in this timer list, and when the timer is closed, the timer
      // would 'destroy' itself, so we don't need to add a destroy function
      // below.
      ten_list_push_ptr_back(&self->timers, timer, NULL);

      ten_timer_enable(timer);

      ten_shared_ptr_t *ret_cmd =
          ten_cmd_result_create_from_cmd(TEN_STATUS_CODE_OK, cmd);
      ten_msg_set_property(ret_cmd, "detail",
                           ten_value_create_string("Operation is success."),
                           NULL);

      ten_engine_dispatch_msg(self, ret_cmd);
      ten_shared_ptr_destroy(ret_cmd);
    } else {
      ten_shared_ptr_t *ret_cmd =
          ten_cmd_result_create_from_cmd(TEN_STATUS_CODE_ERROR, cmd);
      ten_msg_set_property(
          ret_cmd, "detail",
          ten_value_create_string("Failed to cancel an un-existed timer."),
          NULL);
      ten_engine_dispatch_msg(self, ret_cmd);
      ten_shared_ptr_destroy(ret_cmd);
    }
  }
}
