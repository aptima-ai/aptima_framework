//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/engine/internal/close.h"

#include <stddef.h>

#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/app/engine_interface.h"
#include "include_internal/axis_runtime/engine/engine.h"
#include "include_internal/axis_runtime/engine/internal/thread.h"
#include "include_internal/axis_runtime/extension_context/extension_context.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/remote/remote.h"
#include "include_internal/axis_runtime/timer/timer.h"
#include "axis_utils/container/hash_handle.h"
#include "axis_utils/container/hash_table.h"
#include "axis_utils/container/list.h"
#include "axis_utils/io/runloop.h"
#include "axis_utils/lib/atomic.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/field.h"
#include "axis_utils/macro/mark.h"

static void axis_engine_close_sync(axis_engine_t *self) {
  axis_ASSERT(self && axis_engine_check_integrity(self, true),
             "Should not happen.");

  axis_LOGD("[%s] Try to close engine.", axis_app_get_uri(self->app));

  bool nothing_to_do = true;

  axis_list_foreach (&self->timers, iter) {
    axis_timer_t *timer = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(timer && axis_timer_check_integrity(timer, true),
               "Should not happen.");

    axis_timer_stop_async(timer);
    axis_timer_close_async(timer);

    nothing_to_do = false;
  }

  if (self->extension_context) {
    axis_extension_context_close(self->extension_context);

    nothing_to_do = false;
  }

  axis_hashtable_foreach(&self->remotes, iter) {
    axis_hashhandle_t *hh = iter.node;
    axis_remote_t *remote =
        CONTAINER_OF_FROM_OFFSET(hh, self->remotes.hh_offset);
    axis_ASSERT(remote && axis_remote_check_integrity(remote, true),
               "Should not happen.");

    axis_remote_close(remote);

    nothing_to_do = false;
  }

  axis_list_foreach (&self->weak_remotes, iter) {
    axis_remote_t *remote = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(remote, "Invalid argument.");
    axis_ASSERT(axis_remote_check_integrity(remote, true),
               "Invalid use of remote %p.", remote);

    axis_remote_close(remote);

    nothing_to_do = false;
  }

  if (nothing_to_do && axis_engine_is_closing(self)) {
    axis_engine_on_close(self);
  }
}

static void axis_engine_close_task(void *engine_, axis_UNUSED void *arg) {
  axis_engine_t *engine = (axis_engine_t *)engine_;
  axis_ASSERT(engine && axis_engine_check_integrity(engine, true),
             "Invalid argument.");

  axis_engine_close_sync(engine);
}

// The closing of the engine should always be an _async_ operation, so that
// the later operations in the current control flow where calls the function to
// close the engine could still access all the resources correspond to the
// engine.
//
// Ex: If the protocol of a remote is an integrated protocol, that remote
// might be closed directly in 'axis_engine_close_sync()', and the later
// operations in the current control flow might still access that remote, and
// it will result an error.
//
// One solution to this is to make the closing of a remote is always
// _asynced_, however, it can _not_ solve if the later operations in the
// current control flow might still access the other resources correspond to
// the engine, so it's better to enable the closing of the engine is always
// _asynced_ to make the closing operation is a top-level operation in the
// current thread.
//
// i.e.:
//
//    -------------------------
//    | some function call    |
//    -------------------------
//    | some function call    | -> the later operations after
//    |                       |    'axis_engine_close_sync()' might still need
//    |                       |    to access the destroyed resources.
//    -------------------------
//    | axis_engine_close_sync |
//    -------------------------
//    | ...                   |
//    -------------------------
//    | detroy some resources |
//    -------------------------
//
//    --------------------------
//    | axis_engine_close_async | -> make the closing operation is a top-level
//    |                        |    frame in the current thread.
//    --------------------------
//    | ...                    |
//    --------------------------
//    | detroy some resources  |
//    --------------------------
void axis_engine_close_async(axis_engine_t *self) {
  axis_ASSERT(self &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: This function is intended to be called in
                 // different threads.
                 axis_engine_check_integrity(self, false),
             "Should not happen.");

  // The closing flow should be triggered only once.
  //
  // TODO(Wei): Move the following atomic check and set to the engine thread.
  if (!axis_atomic_bool_compare_swap(&self->is_closing, 0, 1)) {
    return;
  }

  axis_runloop_post_task_tail(axis_engine_get_attached_runloop(self),
                             axis_engine_close_task, self, NULL);
}

bool axis_engine_is_closing(axis_engine_t *self) {
  axis_ASSERT(self && axis_engine_check_integrity(self, true),
             "Should not happen.");
  return axis_atomic_load(&self->is_closing) == 1;
}

static size_t axis_engine_unclosed_remotes_cnt(axis_engine_t *self) {
  axis_ASSERT(self && axis_engine_check_integrity(self, true),
             "Should not happen.");

  size_t unclosed_remotes = 0;

  axis_hashtable_foreach(&self->remotes, iter) {
    axis_hashhandle_t *hh = iter.node;
    axis_remote_t *remote =
        CONTAINER_OF_FROM_OFFSET(hh, self->remotes.hh_offset);
    axis_ASSERT(remote && axis_remote_check_integrity(remote, true),
               "Should not happen.");

    if (!remote->is_closed) {
      unclosed_remotes++;
    }
  }

  axis_list_foreach (&self->weak_remotes, iter) {
    axis_remote_t *remote = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(remote, "Invalid argument.");
    axis_ASSERT(axis_remote_check_integrity(remote, true),
               "Invalid use of remote %p.", remote);

    if (!remote->is_closed) {
      unclosed_remotes++;
    }
  }

  return unclosed_remotes;
}

static bool axis_engine_could_be_close(axis_engine_t *self) {
  axis_ASSERT(self && axis_engine_check_integrity(self, true),
             "Should not happen.");

  size_t unclosed_remotes = axis_engine_unclosed_remotes_cnt(self);

  axis_LOGD(
      "[%s] engine liveness: %zu remotes, %zu timers, "
      "extension_context %p",
      axis_app_get_uri(self->app), unclosed_remotes,
      axis_list_size(&self->timers), self->extension_context);

  if (unclosed_remotes == 0 && axis_list_is_empty(&self->timers) &&
      (self->extension_context == NULL)) {
    return true;
  } else {
    return false;
  }
}

static void axis_engine_do_close(axis_engine_t *self) {
  axis_ASSERT(self && axis_engine_check_integrity(self, true),
             "Should not happen.");

  if (self->has_own_loop) {
    // Stop the event loop belong to this engine. The on_close would be called
    // after the event loop has been stopped, and the engine would be
    // destroyed at that time, too.
    axis_runloop_stop(self->loop);

    // The 'on_close' callback will be called when the runloop is ended.
  } else {
    if (self->on_closed) {
      // Call the registered on_close callback if exists.
      self->on_closed(self, self->on_closed_data);
    }
  }
}

void axis_engine_on_close(axis_engine_t *self) {
  axis_ASSERT(self && axis_engine_check_integrity(self, true),
             "Should not happen.");

  if (!axis_engine_could_be_close(self)) {
    axis_LOGD("Could not close alive engine.");
    return;
  }
  axis_LOGD("Close engine.");

  axis_engine_do_close(self);
}

void axis_engine_on_timer_closed(axis_timer_t *timer, void *on_closed_data) {
  axis_ASSERT(timer && axis_timer_check_integrity(timer, true),
             "Should not happen.");

  axis_engine_t *engine = (axis_engine_t *)on_closed_data;
  axis_ASSERT(engine && axis_engine_check_integrity(engine, true),
             "Should not happen.");

  // Remove the timer from the timer list.
  axis_list_remove_ptr(&engine->timers, timer);

  if (axis_engine_is_closing(engine)) {
    axis_engine_on_close(engine);
  }
}

void axis_engine_on_extension_context_closed(
    axis_extension_context_t *extension_context, void *on_closed_data) {
  axis_ASSERT(extension_context, "Invalid argument.");
  axis_ASSERT(axis_extension_context_check_integrity(extension_context, true),
             "Invalid use of extension_context %p.", extension_context);

  axis_engine_t *engine = (axis_engine_t *)on_closed_data;
  axis_ASSERT(engine && axis_engine_check_integrity(engine, true),
             "Should not happen.");

  engine->extension_context = NULL;

  if (axis_engine_is_closing(engine)) {
    axis_engine_on_close(engine);
  }
}

void axis_engine_set_on_closed(axis_engine_t *self,
                              axis_engine_on_closed_func_t on_closed,
                              void *on_closed_data) {
  axis_ASSERT(self && axis_engine_check_integrity(self, false),
             "Should not happen.");
  axis_ASSERT(axis_app_check_integrity(self->app, true), "Should not happen.");

  self->on_closed = on_closed;
  self->on_closed_data = on_closed_data;
}
