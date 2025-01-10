//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include "aptima_utils/io/runloop.h"
#include "aptima_utils/lib/atomic.h"
#include "aptima_utils/lib/signature.h"
#include "aptima_utils/lib/string.h"
#include "aptima_utils/sanitizer/thread_check.h"

#define aptima_ASYNC_SIGNATURE 0xD4CD6DEDB7906C26U

typedef struct aptima_async_t {
  aptima_signature_t signature;
  aptima_sanitizer_thread_check_t thread_check;

  aptima_string_t name;
  aptima_atomic_t close;

  aptima_runloop_t *loop;
  aptima_runloop_async_t *async;
  aptima_runloop_async_t *async_for_close;

  void (*on_trigger)(struct aptima_async_t *, void *);
  void *on_trigger_data;

  void (*on_closed)(struct aptima_async_t *, void *);
  void *on_closed_data;
} aptima_async_t;

aptima_UTILS_API aptima_async_t *aptima_async_create(const char *name,
                                            aptima_runloop_t *loop,
                                            void *on_trigger,
                                            void *on_trigger_data);

aptima_UTILS_API void aptima_async_set_on_closed(aptima_async_t *self, void *on_closed,
                                           void *on_closed_data);

aptima_UTILS_API void aptima_async_trigger(aptima_async_t *self);

aptima_UTILS_API void aptima_async_close(aptima_async_t *self);
