//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "include_internal/ten_runtime/extension/close.h"

#include <stdbool.h>

#include "include_internal/ten_runtime/extension/extension.h"
#include "include_internal/ten_runtime/extension_thread/extension_thread.h"
#include "include_internal/ten_runtime/extension_thread/on_xxx.h"
#include "include_internal/ten_runtime/timer/timer.h"

static bool ten_extension_could_be_closed(ten_extension_t *self) {
  TEN_ASSERT(self && ten_extension_check_integrity(self, true),
             "Should not happen.");

  // Check if all the path timers are closed.
  return ten_list_is_empty(&self->path_timers);
}

// After all the path timers are closed, the closing flow could be proceed.
static void ten_extension_do_close(ten_extension_t *self) {
  TEN_ASSERT(self && ten_extension_check_integrity(self, true),
             "Should not happen.");

  ten_extension_thread_t *extension_thread = self->extension_thread;
  TEN_ASSERT(extension_thread &&
                 ten_extension_thread_check_integrity(extension_thread, true),
             "Should not happen.");

  ten_extension_set_state(self, TEN_EXTENSION_STATE_CLOSING);

  ten_runloop_post_task_tail(ten_extension_get_attached_runloop(self),
                             ten_extension_thread_on_extension_set_closing_flag,
                             extension_thread, self);
}

void ten_extension_on_timer_closed(ten_timer_t *timer, void *on_closed_data) {
  TEN_ASSERT(timer && ten_timer_check_integrity(timer, true),
             "Should not happen.");

  ten_extension_t *extension = (ten_extension_t *)on_closed_data;
  TEN_ASSERT(extension && ten_extension_check_integrity(extension, true),
             "Should not happen.");

  ten_list_remove_ptr(&extension->path_timers, timer);

  if (ten_extension_could_be_closed(extension)) {
    ten_extension_do_close(extension);
  }
}

void ten_extension_do_pre_close_action(ten_extension_t *self) {
  TEN_ASSERT(self && ten_extension_check_integrity(self, true) &&
                 self->extension_thread,
             "Should not happen.");

  // Close the timers of the path tables.
  ten_list_foreach (&self->path_timers, iter) {
    ten_timer_t *timer = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(timer, "Should not happen.");

    ten_timer_stop_async(timer);
    ten_timer_close_async(timer);
  }

  if (ten_extension_could_be_closed(self)) {
    ten_extension_do_close(self);
  }
}
