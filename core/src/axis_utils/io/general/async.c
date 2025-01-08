//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/io/async.h"

#include <stdlib.h>

#include "include_internal/axis_utils/io/runloop.h"
#include "axis_utils/io/runloop.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/sanitizer/thread_check.h"

static bool axis_async_check_integrity(axis_async_t *self, bool check_thread) {
  axis_ASSERT(self, "Invalid argument.");
  if (axis_signature_get(&self->signature) != axis_ASYNC_SIGNATURE) {
    return false;
  }
  if (check_thread &&
      !axis_sanitizer_thread_check_do_check(&self->thread_check)) {
    return false;
  }
  return true;
}

static void axis_async_destroy(axis_async_t *self) {
  axis_ASSERT(self && axis_async_check_integrity(self, true),
             "Invalid argument.");

  axis_string_deinit(&self->name);
  axis_runloop_async_destroy(self->async);
  axis_runloop_async_destroy(self->async_for_close);

  axis_FREE(self);
}

static void async_cb_entry_point(axis_runloop_async_t *async) {
  axis_ASSERT(async && axis_runloop_async_check_integrity(async, true),
             "Invalid argument.");

  axis_async_t *self = (axis_async_t *)(async->data);
  axis_ASSERT(self && axis_async_check_integrity(self, true),
             "Invalid argument.");

  if (self->on_trigger) {
    self->on_trigger(self, self->on_trigger_data);
  }
}

static void close_cb_entry_point_for_close(axis_runloop_async_t *async) {
  axis_ASSERT(async && axis_runloop_async_check_integrity(async, true),
             "Invalid argument.");

  axis_async_t *self = (axis_async_t *)(async->data);
  axis_ASSERT(self && axis_async_check_integrity(self, true),
             "Invalid argument.");

  if (self->on_closed) {
    self->on_closed(self, self->on_closed_data);
  }

  axis_async_destroy(self);
}

static void close_cb_entry_point(axis_runloop_async_t *async) {
  axis_ASSERT(async && axis_runloop_async_check_integrity(async, true),
             "Invalid argument.");

  axis_async_t *self = (axis_async_t *)(async->data);
  axis_ASSERT(self && axis_async_check_integrity(self, true),
             "Invalid argument.");

  axis_runloop_async_close(self->async_for_close,
                          close_cb_entry_point_for_close);
}

static void async_cb_for_close(axis_runloop_async_t *async) {
  axis_ASSERT(async && axis_runloop_async_check_integrity(async, true),
             "Invalid argument.");

  axis_async_t *self = (axis_async_t *)(async->data);
  axis_ASSERT(self && axis_async_check_integrity(self, true),
             "Invalid argument.");

  axis_runloop_async_close(self->async, close_cb_entry_point);
}

axis_async_t *axis_async_create(const char *name, axis_runloop_t *loop,
                              void *on_trigger, void *on_trigger_data) {
  axis_ASSERT(name && loop && axis_runloop_check_integrity(loop, true),
             "Invalid argument.");

  axis_async_t *self = (axis_async_t *)axis_MALLOC(sizeof(axis_async_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_signature_set(&self->signature, axis_ASYNC_SIGNATURE);
  axis_sanitizer_thread_check_init_with_current_thread(&self->thread_check);

  self->loop = loop;

  axis_string_init_formatted(&self->name, "%s", name);

  self->on_trigger = on_trigger;
  self->on_trigger_data = on_trigger_data;

  axis_atomic_store(&self->close, 0);
  self->on_closed = NULL;
  self->on_closed_data = NULL;

  self->async = axis_runloop_async_create(NULL);
  axis_ASSERT(self->async, "Invalid argument.");
  self->async->data = self;

  axis_runloop_async_init(self->async, self->loop, async_cb_entry_point);

  self->async_for_close = axis_runloop_async_create(NULL);
  axis_ASSERT(self->async_for_close, "Invalid argument.");
  self->async_for_close->data = self;

  axis_runloop_async_init(self->async_for_close, self->loop, async_cb_for_close);

  return self;
}

void axis_async_trigger(axis_async_t *self) {
  axis_ASSERT(self &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: This function is intended to be called in any
                 // threads.
                 axis_async_check_integrity(self, false),
             "Invalid argument.");
  axis_runloop_async_notify(self->async);
}

void axis_async_close(axis_async_t *self) {
  axis_ASSERT(self && axis_async_check_integrity(self, true),
             "Invalid argument.");

  if (axis_atomic_bool_compare_swap(&self->close, 0, 1)) {
    axis_runloop_async_notify(self->async_for_close);
  }
}

void axis_async_set_on_closed(axis_async_t *self, void *on_closed,
                             void *on_closed_data) {
  axis_ASSERT(self && axis_async_check_integrity(self, true),
             "Invalid argument.");

  self->on_closed = on_closed;
  self->on_closed_data = on_closed_data;
}
