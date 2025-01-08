//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>
#include <stdint.h>

#include "include_internal/axis_runtime/common/loc.h"
#include "axis_utils/io/runloop.h"
#include "axis_utils/lib/atomic.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/smart_ptr.h"

#define axis_TIMER_SIGNATURE 0x94DA8B352E91919DU

typedef struct axis_timer_t axis_timer_t;

typedef void (*axis_timer_on_trigger_func_t)(axis_timer_t *self,
                                            void *on_trigger_data);

typedef void (*axis_timer_on_closed_func_t)(axis_timer_t *self,
                                           void *on_closed_data);

struct axis_timer_t {
  axis_signature_t signature;
  axis_sanitizer_thread_check_t thread_check;

  axis_atomic_t is_closing;

  axis_timer_on_closed_func_t on_closed;
  void *on_closed_data;

  axis_timer_on_trigger_func_t on_trigger;
  void *on_trigger_data;

  uint32_t id;
  uint64_t timeout_in_us;
  int32_t requested_times;  // axis_TIMER_INFINITE means "forever"
  int32_t times;

  // If the auto_restart flag is set to be 'false', it will __not__
  // automatically restart timing after each timeout. Instead, the user needs to
  // manually restart the timer (axis_timer_enable). When the number of timeouts
  // exceeds the specified times, the timer will automatically close.
  //
  // Conversely, if auto_restart is set to be 'true' (by default), the timer
  // will automatically decide whether to restart timing or close the timer
  // based on its policy after each timeout.
  bool auto_restart;

  axis_loc_t src_loc;

  axis_runloop_timer_t *backend;
  axis_runloop_t *runloop;
};

axis_RUNTIME_PRIVATE_API bool axis_timer_check_integrity(axis_timer_t *self,
                                                       bool check_thread);

axis_RUNTIME_PRIVATE_API axis_timer_t *axis_timer_create_with_cmd(
    axis_shared_ptr_t *cmd, axis_runloop_t *runloop);

axis_RUNTIME_PRIVATE_API axis_timer_t *axis_timer_create(axis_runloop_t *runloop,
                                                      uint64_t timeout_in_us,
                                                      int32_t requested_times,
                                                      bool auto_restart);

axis_RUNTIME_PRIVATE_API void axis_timer_destroy(axis_timer_t *self);

axis_RUNTIME_PRIVATE_API void axis_timer_enable(axis_timer_t *self);

axis_RUNTIME_PRIVATE_API void axis_timer_stop_async(axis_timer_t *self);

axis_RUNTIME_PRIVATE_API void axis_timer_set_on_triggered(
    axis_timer_t *self, axis_timer_on_trigger_func_t on_trigger,
    void *on_trigger_data);

axis_RUNTIME_PRIVATE_API void axis_timer_set_on_closed(
    axis_timer_t *self, axis_timer_on_closed_func_t on_closed,
    void *on_closed_data);

axis_RUNTIME_PRIVATE_API void axis_timer_close_async(axis_timer_t *self);

axis_RUNTIME_PRIVATE_API bool axis_timer_is_id_equal_to(axis_timer_t *self,
                                                      uint32_t id);
