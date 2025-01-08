//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "include_internal/axis_utils/io/runloop.h"
#include "axis_utils/container/list.h"
#include "axis_utils/lib/atomic.h"
#include "axis_utils/lib/mutex.h"

typedef struct axis_runloop_common_t {
  axis_runloop_t base;

  uint64_t state;
  int destroying;
  axis_list_t tasks;
  axis_mutex_t *lock;
  axis_runloop_async_t *task_available_signal;
  axis_atomic_t attach_other;

  void (*destroy)(axis_runloop_t *);
  void (*run)(axis_runloop_t *);
  void (*close)(axis_runloop_t *);
  void (*stop)(axis_runloop_t *);
  void *(*get_raw)(axis_runloop_t *);
  int (*alive)(axis_runloop_t *);

  /**
   * @brief The callback function which will be called when the whole stop()
   * operations are fully completed. This enables the runloop users to perform
   * some actions that can only be triggered when the runloop stops completely.
   */
  axis_runloop_on_stopped_func_t on_stopped;
  void *on_stopped_data;
} axis_runloop_common_t;

typedef struct axis_runloop_async_common_t {
  axis_runloop_async_t base;

  int (*init)(axis_runloop_async_t *, axis_runloop_t *,
              void (*)(axis_runloop_async_t *));
  void (*close)(axis_runloop_async_t *, void (*)(axis_runloop_async_t *));
  void (*destroy)(axis_runloop_async_t *);
  int (*notify)(axis_runloop_async_t *);
} axis_runloop_async_common_t;

typedef struct axis_runloop_timer_common_t {
  axis_runloop_timer_t base;

  void *start_data;
  void *stop_data;
  void *close_data;

  int (*start)(axis_runloop_timer_t *, axis_runloop_t *,
               void (*)(axis_runloop_timer_t *, void *));
  void (*stop)(axis_runloop_timer_t *, void (*)(axis_runloop_timer_t *, void *));
  void (*close)(axis_runloop_timer_t *, void (*)(axis_runloop_timer_t *, void *));
  void (*destroy)(axis_runloop_timer_t *);
} axis_runloop_timer_common_t;

typedef struct axis_runloop_task_t {
  axis_listnode_t node;

  void (*func)(void *, void *);
  void *from;
  void *arg;
} axis_runloop_task_t;
