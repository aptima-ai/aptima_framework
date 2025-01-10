//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/timer/timer.h"

#include <stdint.h>
#include <stdlib.h>

#include "include_internal/axis_runtime/common/loc.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/timer/cmd.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "axis_runtime/timer/timer.h"
#include "axis_utils/io/runloop.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/log/log.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"

static bool axis_timer_is_closing(axis_timer_t *self) {
  axis_ASSERT(self && axis_timer_check_integrity(self, true),
             "Should not happen.");
  return axis_atomic_load(&self->is_closing) == 1;
}

static bool axis_timer_could_be_close(axis_timer_t *self) {
  axis_ASSERT(self && axis_timer_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(axis_runloop_check_integrity(self->runloop, true),
             "Should not happen.");

  if (self->backend) {
    return false;
  }
  return true;
}

static void axis_timer_do_close(axis_timer_t *self) {
  axis_ASSERT(self && axis_timer_check_integrity(self, true) &&
                 axis_runloop_check_integrity(self->runloop, true),
             "Should not happen.");

  if (self->on_closed) {
    self->on_closed(self, self->on_closed_data);
  }

  // All the necessary steps in the timer closing flow are done, so it's
  // safe to destroy the timer now to prevent memory leaks.
  axis_timer_destroy(self);
}

static void axis_timer_on_close(axis_timer_t *self) {
  axis_ASSERT(self && axis_timer_check_integrity(self, true) &&
                 axis_runloop_check_integrity(self->runloop, true),
             "Should not happen.");

  if (!axis_timer_could_be_close(self)) {
    axis_LOGV("Could not close alive timer.");
    return;
  }
  axis_LOGV("Close timer.");

  axis_timer_do_close(self);
}

static void axis_runloop_timer_on_closed(axis_UNUSED axis_runloop_timer_t *timer,
                                        void *arg) {
  axis_timer_t *self = arg;

  axis_ASSERT(self && axis_timer_check_integrity(self, true) &&
                 axis_runloop_check_integrity(self->runloop, true),
             "Should not happen.");

  // The runloop timer is closed, so it's safe to destroy the runloop timer now
  // to prevent memory leaks.
  axis_runloop_timer_destroy(self->backend);
  self->backend = NULL;

  if (axis_timer_is_closing(self)) {
    axis_timer_on_close(self);
  }
}

static void axis_timer_close(axis_timer_t *self) {
  axis_ASSERT(self && axis_timer_check_integrity(self, true) &&
                 axis_runloop_check_integrity(self->runloop, true),
             "Should not happen.");

  axis_runloop_timer_t *timer = self->backend;
  axis_runloop_timer_close(timer, axis_runloop_timer_on_closed, self);
}

static void axis_runloop_timer_on_stopped(axis_UNUSED axis_runloop_timer_t *timer,
                                         axis_UNUSED void *arg) {}

static void axis_timer_stop(axis_timer_t *self) {
  axis_ASSERT(self && axis_timer_check_integrity(self, true) &&
                 axis_runloop_check_integrity(self->runloop, true),
             "Should not happen.");

  axis_runloop_timer_t *timer = self->backend;
  axis_runloop_timer_stop(timer, axis_runloop_timer_on_stopped, self);
}

bool axis_timer_check_integrity(axis_timer_t *self, bool check_thread) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_TIMER_SIGNATURE) {
    return false;
  }

  if (check_thread &&
      !axis_sanitizer_thread_check_do_check(&self->thread_check)) {
    return false;
  }

  return true;
}

static void axis_timer_on_trigger(axis_timer_t *self,
                                 axis_UNUSED void *on_trigger_data) {
  axis_ASSERT(self && axis_timer_check_integrity(self, true) &&
                 axis_runloop_check_integrity(self->runloop, true),
             "Should not happen.");

  self->times++;

  if ((self->requested_times == axis_TIMER_INFINITE) ||
      (self->times <= self->requested_times)) {
    if (self->on_trigger) {
      self->on_trigger(self, self->on_trigger_data);
    }

    if (!self->auto_restart) {
      // If the timer is _not_ auto_restart, it will not automatically start the
      // next round of timing or close itself after each timeout trigger.
      // Instead, it will only do so when the user manually enables the timer
      // again.
      return;
    }

    if (self->requested_times == axis_TIMER_INFINITE ||
        self->times < self->requested_times) {
      // Setup the next timeout.
      axis_timer_enable(self);
    } else {
      axis_timer_stop_async(self);
      axis_timer_close_async(self);
    }
  } else {
    axis_ASSERT(0, "Should not happen.");
  }
}

static axis_timer_t *axis_timer_create_internal(axis_runloop_t *runloop) {
  axis_ASSERT(runloop && axis_runloop_check_integrity(runloop, true),
             "Should not happen.");

  axis_LOGV("Create a timer.");

  axis_timer_t *self = (axis_timer_t *)axis_MALLOC(sizeof(axis_timer_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_signature_set(&self->signature, (axis_signature_t)axis_TIMER_SIGNATURE);
  axis_sanitizer_thread_check_init_with_current_thread(&self->thread_check);

  self->runloop = runloop;

  self->id = 0;
  self->times = 0;
  self->auto_restart = true;

  axis_loc_init_empty(&self->src_loc);

  self->on_trigger = NULL;
  self->on_trigger_data = NULL;

  axis_atomic_store(&self->is_closing, 0);
  self->on_closed = NULL;
  self->on_closed_data = NULL;

  self->backend = axis_runloop_timer_create(NULL, 0, 0);
  if (!self->backend) {
    axis_LOGE("No valid backend for timer.");
    goto error;
  }

  return self;

error:
  if (self) {
    if (self->backend) {
      axis_runloop_timer_destroy(self->backend);
    }

    axis_FREE(self);
  }
  return NULL;
}

axis_timer_t *axis_timer_create(axis_runloop_t *runloop, uint64_t timeout_in_us,
                              int32_t requested_times, bool auto_restart) {
  axis_ASSERT(runloop && axis_runloop_check_integrity(runloop, true),
             "Should not happen.");

  axis_timer_t *self = axis_timer_create_internal(runloop);
  if (!self) {
    return NULL;
  }

  self->timeout_in_us = timeout_in_us;
  self->requested_times = requested_times;
  self->auto_restart = auto_restart;

  return self;
}

axis_timer_t *axis_timer_create_with_cmd(axis_shared_ptr_t *cmd,
                                       axis_runloop_t *runloop) {
  axis_ASSERT(cmd && axis_msg_get_type(cmd) == axis_MSG_TYPE_CMD_TIMER &&
                 runloop && axis_runloop_check_integrity(runloop, true),
             "Should not happen.");

  axis_timer_t *self = axis_timer_create_internal(runloop);
  if (!self) {
    return NULL;
  }

  self->id = axis_cmd_timer_get_timer_id(cmd);
  self->timeout_in_us = axis_cmd_timer_get_timeout_in_us(cmd);
  self->requested_times = axis_cmd_timer_get_times(cmd);
  axis_loc_set_from_loc(&self->src_loc, axis_msg_get_src_loc(cmd));

  return self;
}

void axis_timer_destroy(axis_timer_t *self) {
  axis_ASSERT(self && axis_timer_check_integrity(self, true) &&
                 axis_runloop_check_integrity(self->runloop, true) &&
                 axis_timer_could_be_close(self),
             "Should not happen.");

  axis_LOGV("Destroy a timer.");

  axis_sanitizer_thread_check_deinit(&self->thread_check);
  axis_signature_set(&self->signature, 0);
  axis_loc_deinit(&self->src_loc);

  axis_FREE(self);
}

static void axis_runloop_timer_on_triggered(
    axis_UNUSED axis_runloop_timer_t *timer, void *arg) {
  axis_timer_t *self = arg;
  axis_ASSERT(self && axis_timer_check_integrity(self, true) &&
                 axis_runloop_check_integrity(self->runloop, true),
             "Should not happen.");

  axis_timer_on_trigger(self, self->on_trigger_data);
}

void axis_timer_enable(axis_timer_t *self) {
  axis_ASSERT(self && axis_timer_check_integrity(self, true) &&
                 axis_runloop_check_integrity(self->runloop, true),
             "Should not happen.");

  if (self->requested_times != axis_TIMER_INFINITE &&
      self->times >= self->requested_times) {
    // The timer has ended, so it should not be enabled again.
    axis_timer_stop_async(self);
    axis_timer_close_async(self);
    return;
  }

  axis_runloop_timer_set_timeout(self->backend, self->timeout_in_us / 1000, 0);

  axis_runloop_timer_start(self->backend, self->runloop,
                          axis_runloop_timer_on_triggered, self);
}

static void axis_timer_stop_(void *timer_, axis_UNUSED void *arg) {
  axis_timer_t *timer = (axis_timer_t *)timer_;
  axis_ASSERT(timer && axis_timer_check_integrity(timer, true) &&
                 axis_runloop_check_integrity(timer->runloop, true),
             "Should not happen.");

  axis_timer_stop(timer);
}

void axis_timer_stop_async(axis_timer_t *self) {
  axis_ASSERT(self && axis_timer_check_integrity(self, true),
             "Should not happen.");

  axis_LOGV("Stop a timer.");

  axis_runloop_post_task_tail(self->runloop, axis_timer_stop_, self, NULL);
}

void axis_timer_set_on_closed(axis_timer_t *self,
                             axis_timer_on_closed_func_t on_closed,
                             void *on_closed_data) {
  axis_ASSERT(self && axis_timer_check_integrity(self, true) &&
                 axis_runloop_check_integrity(self->runloop, true),
             "Should not happen.");

  self->on_closed = on_closed;
  self->on_closed_data = on_closed_data;
}

void axis_timer_set_on_triggered(axis_timer_t *self,
                                axis_timer_on_trigger_func_t on_trigger,
                                void *on_trigger_data) {
  axis_ASSERT(self && axis_timer_check_integrity(self, true) &&
                 axis_runloop_check_integrity(self->runloop, true),
             "Should not happen.");

  self->on_trigger = on_trigger;
  self->on_trigger_data = on_trigger_data;
}

static void axis_timer_close_(void *timer_, axis_UNUSED void *arg) {
  axis_timer_t *timer = (axis_timer_t *)timer_;
  axis_ASSERT(timer && axis_timer_check_integrity(timer, true),
             "Should not happen.");

  axis_timer_close(timer);
}

void axis_timer_close_async(axis_timer_t *self) {
  axis_ASSERT(self && axis_timer_check_integrity(self, true),
             "Should not happen.");

  if (axis_atomic_bool_compare_swap(&self->is_closing, 0, 1)) {
    axis_LOGV("Try to close a timer.");

    axis_runloop_post_task_tail(self->runloop, axis_timer_close_, self, NULL);
  }
}

bool axis_timer_is_id_equal_to(axis_timer_t *self, uint32_t id) {
  axis_ASSERT(self && axis_timer_check_integrity(self, true) && id &&
                 axis_runloop_check_integrity(self->runloop, true),
             "Should not happen.");

  return (self->id == id) ? true : false;
}
