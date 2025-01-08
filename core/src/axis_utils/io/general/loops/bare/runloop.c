//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/io/runloop.h"

#include <stdlib.h>
#include <string.h>

#include "axis_utils/io/general/loops/runloop.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"
#include "axis_utils/macro/memory.h"

typedef struct axis_runloop_bare_t {
  axis_runloop_common_t common;
} axis_runloop_bare_t;

typedef struct axis_runloop_async_bare_t {
  axis_runloop_async_common_t common;
  void (*notify_callback)(axis_runloop_async_t *);
  void (*close_callback)(axis_runloop_async_t *);
} axis_runloop_async_bare_t;

typedef struct axis_runloop_timer_bare_t {
  axis_runloop_timer_common_t common;

  bool initted;
  void (*notify_callback)(axis_runloop_timer_t *, void *);
  void (*stop_callback)(axis_runloop_timer_t *, void *);
  void (*close_callback)(axis_runloop_timer_t *, void *);
} axis_runloop_timer_bare_t;

static void axis_runloop_bare_destroy(axis_runloop_t *loop);
static void axis_runloop_bare_run(axis_runloop_t *loop);
static void *axis_runloop_bare_get_raw(axis_runloop_t *loop);
static void axis_runloop_bare_close(axis_runloop_t *loop);
static void axis_runloop_bare_stop(axis_runloop_t *loop);
static int axis_runloop_async_bare_init(axis_runloop_async_t *base,
                                       axis_runloop_t *loop,
                                       void (*callback)(axis_runloop_async_t *));
static void axis_runloop_async_bare_close(
    axis_runloop_async_t *base, void (*close_cb)(axis_runloop_async_t *));
static int axis_runloop_async_bare_notify(axis_runloop_async_t *base);
static void axis_runloop_async_bare_destroy(axis_runloop_async_t *base);
static int axis_runloop_bare_alive(axis_runloop_t *loop);

axis_runloop_common_t *axis_runloop_create_bare_common(axis_UNUSED void *raw) {
  axis_runloop_bare_t *impl =
      (axis_runloop_bare_t *)axis_MALLOC(sizeof(axis_runloop_bare_t));
  axis_ASSERT(impl, "Failed to allocate memory.");
  if (!impl) {
    return NULL;
  }

  memset(impl, 0, sizeof(axis_runloop_bare_t));

  impl->common.base.impl = axis_strdup(axis_RUNLOOP_BARE);

  impl->common.destroy = axis_runloop_bare_destroy;
  impl->common.run = axis_runloop_bare_run;
  impl->common.get_raw = axis_runloop_bare_get_raw;
  impl->common.close = axis_runloop_bare_close;
  impl->common.stop = axis_runloop_bare_stop;
  impl->common.alive = axis_runloop_bare_alive;

  return &impl->common;
}

axis_runloop_common_t *axis_runloop_create_bare(void) {
  return axis_runloop_create_bare_common(NULL);
}

axis_runloop_common_t *axis_runloop_attach_bare(void *raw) {
  return axis_runloop_create_bare_common(raw);
}

axis_runloop_async_common_t *axis_runloop_async_create_bare(void) {
  axis_runloop_async_bare_t *impl =
      (axis_runloop_async_bare_t *)axis_MALLOC(sizeof(axis_runloop_async_bare_t));
  axis_ASSERT(impl, "Failed to allocate memory.");
  if (!impl) {
    return NULL;
  }

  memset(impl, 0, sizeof(axis_runloop_async_bare_t));

  impl->common.base.impl = axis_strdup(axis_RUNLOOP_BARE);
  impl->common.init = axis_runloop_async_bare_init;
  impl->common.close = axis_runloop_async_bare_close;
  impl->common.destroy = axis_runloop_async_bare_destroy;
  impl->common.notify = axis_runloop_async_bare_notify;

  return &impl->common;
}

static void axis_runloop_bare_destroy(axis_runloop_t *loop) {
  axis_runloop_bare_t *impl = (axis_runloop_bare_t *)loop;

  if (!loop || strcmp(loop->impl, axis_RUNLOOP_BARE) != 0) {
    return;
  }

  axis_sanitizer_thread_check_deinit(&loop->thread_check);

  axis_FREE(impl->common.base.impl);
  axis_FREE(impl);
}

static void axis_runloop_bare_run(axis_runloop_t *loop) {
  axis_runloop_bare_t *impl = (axis_runloop_bare_t *)loop;

  if (!loop || strcmp(loop->impl, axis_RUNLOOP_BARE) != 0) {
    return;
  }

  // no-op.
}

static void *axis_runloop_bare_get_raw(axis_runloop_t *loop) {
  axis_runloop_bare_t *impl = (axis_runloop_bare_t *)loop;

  if (!loop || strcmp(loop->impl, axis_RUNLOOP_BARE) != 0) {
    return NULL;
  }

  return NULL;
}

static void axis_runloop_bare_close(axis_runloop_t *loop) {
  axis_runloop_bare_t *impl = (axis_runloop_bare_t *)loop;

  if (!loop || strcmp(loop->impl, axis_RUNLOOP_BARE) != 0) {
    return;
  }
}

static void axis_runloop_bare_stop(axis_runloop_t *loop) {
  axis_runloop_bare_t *impl = (axis_runloop_bare_t *)loop;

  if (!loop || strcmp(loop->impl, axis_RUNLOOP_BARE) != 0) {
    return;
  }

  // In bare runloop, the runloop is stopped completely here, call the
  // on_stopped callback if the user registered one before.
  if (impl->common.on_stopped) {
    impl->common.on_stopped((axis_runloop_t *)impl,
                            impl->common.on_stopped_data);
  }
}

static int axis_runloop_bare_alive(axis_runloop_t *loop) {
  axis_runloop_bare_t *impl = (axis_runloop_bare_t *)loop;

  if (!loop || strcmp(loop->impl, axis_RUNLOOP_BARE) != 0) {
    return 0;
  }

  return 1;
}

static int axis_runloop_async_bare_init(
    axis_runloop_async_t *base, axis_runloop_t *loop,
    void (*notify_callback)(axis_runloop_async_t *)) {
  axis_runloop_async_bare_t *async_impl = (axis_runloop_async_bare_t *)base;
  axis_runloop_bare_t *loop_impl = (axis_runloop_bare_t *)loop;

  if (!base || strcmp(base->impl, axis_RUNLOOP_BARE) != 0) {
    return -1;
  }

  if (!loop || strcmp(loop->impl, axis_RUNLOOP_BARE) != 0) {
    return -1;
  }

  async_impl->notify_callback = notify_callback;

  return 0;
}

static void axis_runloop_async_bare_close(
    axis_runloop_async_t *base, void (*close_cb)(axis_runloop_async_t *)) {
  axis_runloop_async_bare_t *async_impl = (axis_runloop_async_bare_t *)base;

  if (!base || strcmp(base->impl, axis_RUNLOOP_BARE) != 0) {
    return;
  }

  close_cb(base);
}

static void axis_runloop_async_bare_destroy(axis_runloop_async_t *base) {
  axis_runloop_async_bare_t *async_impl = (axis_runloop_async_bare_t *)base;

  if (!base || strcmp(base->impl, axis_RUNLOOP_BARE) != 0) {
    return;
  }

  axis_sanitizer_thread_check_deinit(&base->thread_check);

  axis_FREE(async_impl->common.base.impl);
  axis_FREE(async_impl);
}

static int axis_runloop_async_bare_notify(axis_runloop_async_t *base) {
  axis_runloop_async_bare_t *async_impl = (axis_runloop_async_bare_t *)base;

  if (!base || strcmp(base->impl, axis_RUNLOOP_BARE) != 0) {
    return -1;
  }

  return 0;
}

static int axis_runloop_timer_bare_start(
    axis_runloop_timer_t *base, axis_runloop_t *loop,
    void (*notify_callback)(axis_runloop_timer_t *, void *)) {
  axis_runloop_timer_bare_t *timer_impl = (axis_runloop_timer_bare_t *)base;
  axis_runloop_bare_t *loop_impl = (axis_runloop_bare_t *)loop;

  if (!base || strcmp(base->impl, axis_RUNLOOP_BARE) != 0) {
    return -1;
  }

  if (!loop || strcmp(loop->impl, axis_RUNLOOP_BARE) != 0) {
    return -1;
  }

  timer_impl->notify_callback = notify_callback;
  if (timer_impl->initted == false) {
    timer_impl->initted = true;
  }

  return 0;
}

static void axis_runloop_timer_bare_stop(axis_runloop_timer_t *base,
                                        void (*stop_cb)(axis_runloop_timer_t *,
                                                        void *)) {
  axis_runloop_timer_bare_t *timer_impl = (axis_runloop_timer_bare_t *)base;

  if (!base || strcmp(base->impl, axis_RUNLOOP_BARE) != 0) {
    return;
  }

  timer_impl->stop_callback = stop_cb;

  if (timer_impl->stop_callback) {
    timer_impl->stop_callback(&timer_impl->common.base,
                              timer_impl->common.stop_data);
  }
}

static void axis_runloop_timer_bare_close(axis_runloop_timer_t *base,
                                         void (*close_cb)(axis_runloop_timer_t *,
                                                          void *)) {
  axis_runloop_timer_bare_t *timer_impl = (axis_runloop_timer_bare_t *)base;

  if (!base || strcmp(base->impl, axis_RUNLOOP_BARE) != 0) {
    return;
  }

  close_cb(base, NULL);
}

static void axis_runloop_timer_bare_destroy(axis_runloop_timer_t *base) {
  axis_runloop_timer_bare_t *timer_impl = (axis_runloop_timer_bare_t *)base;

  if (!base || strcmp(base->impl, axis_RUNLOOP_BARE) != 0) {
    return;
  }

  axis_sanitizer_thread_check_deinit(&base->thread_check);

  axis_FREE(timer_impl->common.base.impl);
  axis_FREE(timer_impl);
}

axis_runloop_timer_common_t *axis_runloop_timer_create_bare(void) {
  axis_runloop_timer_bare_t *impl =
      (axis_runloop_timer_bare_t *)axis_MALLOC(sizeof(axis_runloop_timer_bare_t));
  axis_ASSERT(impl, "Failed to allocate memory.");
  if (!impl) {
    return NULL;
  }

  memset(impl, 0, sizeof(axis_runloop_timer_bare_t));

  impl->initted = false;
  impl->common.base.impl = axis_strdup(axis_RUNLOOP_BARE);
  impl->common.start = axis_runloop_timer_bare_start;
  impl->common.stop = axis_runloop_timer_bare_stop;
  impl->common.close = axis_runloop_timer_bare_close;
  impl->common.destroy = axis_runloop_timer_bare_destroy;

  return &impl->common;
}
