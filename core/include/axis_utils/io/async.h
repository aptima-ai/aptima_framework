//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include "axis_utils/io/runloop.h"
#include "axis_utils/lib/atomic.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/sanitizer/thread_check.h"

#define axis_ASYNC_SIGNATURE 0xD4CD6DEDB7906C26U

typedef struct axis_async_t {
  axis_signature_t signature;
  axis_sanitizer_thread_check_t thread_check;

  axis_string_t name;
  axis_atomic_t close;

  axis_runloop_t *loop;
  axis_runloop_async_t *async;
  axis_runloop_async_t *async_for_close;

  void (*on_trigger)(struct axis_async_t *, void *);
  void *on_trigger_data;

  void (*on_closed)(struct axis_async_t *, void *);
  void *on_closed_data;
} axis_async_t;

axis_UTILS_API axis_async_t *axis_async_create(const char *name,
                                            axis_runloop_t *loop,
                                            void *on_trigger,
                                            void *on_trigger_data);

axis_UTILS_API void axis_async_set_on_closed(axis_async_t *self, void *on_closed,
                                           void *on_closed_data);

axis_UTILS_API void axis_async_trigger(axis_async_t *self);

axis_UTILS_API void axis_async_close(axis_async_t *self);
