//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "include_internal/ten_runtime/engine/internal/close.h"

#include <stddef.h>

#include "include_internal/ten_runtime/app/app.h"
#include "include_internal/ten_runtime/app/engine_interface.h"
#include "include_internal/ten_runtime/engine/engine.h"
#include "include_internal/ten_runtime/engine/internal/thread.h"
#include "include_internal/ten_runtime/extension_context/extension_context.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "include_internal/ten_runtime/remote/remote.h"
#include "include_internal/ten_runtime/timer/timer.h"
#include "ten_utils/container/hash_handle.h"
#include "ten_utils/container/hash_table.h"
#include "ten_utils/container/list.h"
#include "ten_utils/io/runloop.h"
#include "ten_utils/lib/atomic.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/field.h"
#include "ten_utils/macro/mark.h"

static void ten_engine_close_sync(ten_engine_t *self) {
  TEN_ASSERT(self && ten_engine_check_integrity(self, true),
             "Should not happen.");

  TEN_LOGD("[%s] Try to close engine.",
           ten_string_get_raw_str(ten_app_get_uri(self->app)));

  bool nothing_to_do = true;

  ten_list_foreach (&self->timers, iter) {
    ten_timer_t *timer = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(timer && ten_timer_check_integrity(timer, true),
               "Should not happen.");

    ten_timer_stop_async(timer);
    ten_timer_close_async(timer);

    nothing_to_do = false;
  }

  if (self->extension_context) {
    ten_extension_context_close(self->extension_context);

    nothing_to_do = false;
  }

  ten_hashtable_foreach(&self->remotes, iter) {
    ten_hashhandle_t *hh = iter.node;
    ten_remote_t *remote =
        CONTAINER_OF_FROM_OFFSET(hh, self->remotes.hh_offset);
    TEN_ASSERT(remote && ten_remote_check_integrity(remote, true),
               "Should not happen.");

    ten_remote_close(remote);

    nothing_to_do = false;
  }

  ten_list_foreach (&self->weak_remotes, iter) {
    ten_remote_t *remote = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(remote, "Invalid argument.");
    TEN_ASSERT(ten_remote_check_integrity(remote, true),
               "Invalid use of remote %p.", remote);

    ten_remote_close(remote);

    nothing_to_do = false;
  }

  if (nothing_to_do && ten_engine_is_closing(self)) {
    ten_engine_on_close(self);
  }
}

static void ten_engine_close_task(void *engine_, TEN_UNUSED void *arg) {
  ten_engine_t *engine = (ten_engine_t *)engine_;
  TEN_ASSERT(engine && ten_engine_check_integrity(engine, true),
             "Invalid argument.");

  ten_engine_close_sync(engine);
}

// The closing of the engine should always be an _async_ operation, so that
// the later operations in the current control flow where calls the function to
// close the engine could still access all the resources correspond to the
// engine.
//
// Ex: If the protocol of a remote is an integrated protocol, that remote
// might be closed directly in 'ten_engine_close_sync()', and the later
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
//    |                       |    'ten_engine_close_sync()' might still need
//    |                       |    to access the destroyed resources.
//    -------------------------
//    | ten_engine_close_sync |
//    -------------------------
//    | ...                   |
//    -------------------------
//    | detroy some resources |
//    -------------------------
//
//    --------------------------
//    | ten_engine_close_async | -> make the closing operation is a top-level
//    |                        |    frame in the current thread.
//    --------------------------
//    | ...                    |
//    --------------------------
//    | detroy some resources  |
//    --------------------------
void ten_engine_close_async(ten_engine_t *self) {
  TEN_ASSERT(self &&
                 // TEN_NOLINTNEXTLINE(thread-check)
                 // thread-check: This function is intended to be called in
                 // different threads.
                 ten_engine_check_integrity(self, false),
             "Should not happen.");

  // The closing flow should be triggered only once.
  //
  // TODO(Wei): Move the following atomic check and set to the engine thread.
  if (!ten_atomic_bool_compare_swap(&self->is_closing, 0, 1)) {
    return;
  }

  ten_runloop_post_task_tail(ten_engine_get_attached_runloop(self),
                             ten_engine_close_task, self, NULL);
}

bool ten_engine_is_closing(ten_engine_t *self) {
  TEN_ASSERT(self && ten_engine_check_integrity(self, true),
             "Should not happen.");
  return ten_atomic_load(&self->is_closing) == 1;
}

static size_t ten_engine_unclosed_remotes_cnt(ten_engine_t *self) {
  TEN_ASSERT(self && ten_engine_check_integrity(self, true),
             "Should not happen.");

  size_t unclosed_remotes = 0;

  ten_hashtable_foreach(&self->remotes, iter) {
    ten_hashhandle_t *hh = iter.node;
    ten_remote_t *remote =
        CONTAINER_OF_FROM_OFFSET(hh, self->remotes.hh_offset);
    TEN_ASSERT(remote && ten_remote_check_integrity(remote, true),
               "Should not happen.");

    if (!remote->is_closed) {
      unclosed_remotes++;
    }
  }

  ten_list_foreach (&self->weak_remotes, iter) {
    ten_remote_t *remote = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(remote, "Invalid argument.");
    TEN_ASSERT(ten_remote_check_integrity(remote, true),
               "Invalid use of remote %p.", remote);

    if (!remote->is_closed) {
      unclosed_remotes++;
    }
  }

  return unclosed_remotes;
}

static bool ten_engine_could_be_close(ten_engine_t *self) {
  TEN_ASSERT(self && ten_engine_check_integrity(self, true),
             "Should not happen.");

  size_t unclosed_remotes = ten_engine_unclosed_remotes_cnt(self);

  TEN_LOGD(
      "[%s] engine liveness: %zu remotes, %zu timers, "
      "extension_context %p",
      ten_string_get_raw_str(ten_app_get_uri(self->app)), unclosed_remotes,
      ten_list_size(&self->timers), self->extension_context);

  if (unclosed_remotes == 0 && ten_list_is_empty(&self->timers) &&
      (self->extension_context == NULL)) {
    return true;
  } else {
    return false;
  }
}

static void ten_engine_do_close(ten_engine_t *self) {
  TEN_ASSERT(self && ten_engine_check_integrity(self, true),
             "Should not happen.");

  if (self->has_own_loop) {
    // Stop the event loop belong to this engine. The on_close would be called
    // after the event loop has been stopped, and the engine would be
    // destroyed at that time, too.
    ten_runloop_stop(self->loop);

    // The 'on_close' callback will be called when the runloop is ended.
  } else {
    if (self->on_closed) {
      // Call the registered on_close callback if exists.
      self->on_closed(self, self->on_closed_data);
    }
  }
}

void ten_engine_on_close(ten_engine_t *self) {
  TEN_ASSERT(self && ten_engine_check_integrity(self, true),
             "Should not happen.");

  if (!ten_engine_could_be_close(self)) {
    TEN_LOGD("Could not close alive engine.");
    return;
  }
  TEN_LOGD("Close engine.");

  ten_engine_do_close(self);
}

void ten_engine_on_timer_closed(ten_timer_t *timer, void *on_closed_data) {
  TEN_ASSERT(timer && ten_timer_check_integrity(timer, true),
             "Should not happen.");

  ten_engine_t *engine = (ten_engine_t *)on_closed_data;
  TEN_ASSERT(engine && ten_engine_check_integrity(engine, true),
             "Should not happen.");

  // Remove the timer from the timer list.
  ten_list_remove_ptr(&engine->timers, timer);

  if (ten_engine_is_closing(engine)) {
    ten_engine_on_close(engine);
  }
}

void ten_engine_on_extension_context_closed(
    ten_extension_context_t *extension_context, void *on_closed_data) {
  TEN_ASSERT(extension_context, "Invalid argument.");
  TEN_ASSERT(ten_extension_context_check_integrity(extension_context, true),
             "Invalid use of extension_context %p.", extension_context);

  ten_engine_t *engine = (ten_engine_t *)on_closed_data;
  TEN_ASSERT(engine && ten_engine_check_integrity(engine, true),
             "Should not happen.");

  engine->extension_context = NULL;

  if (ten_engine_is_closing(engine)) {
    ten_engine_on_close(engine);
  }
}

void ten_engine_set_on_closed(ten_engine_t *self,
                              ten_engine_on_closed_func_t on_closed,
                              void *on_closed_data) {
  TEN_ASSERT(self && ten_engine_check_integrity(self, false),
             "Should not happen.");
  TEN_ASSERT(ten_app_check_integrity(self->app, true), "Should not happen.");

  self->on_closed = on_closed;
  self->on_closed_data = on_closed_data;
}
