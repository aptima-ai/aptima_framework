//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/extension_thread/msg_interface/common.h"

#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/app/msg_interface/common.h"
#include "include_internal/axis_runtime/common/loc.h"
#include "include_internal/axis_runtime/engine/engine.h"
#include "include_internal/axis_runtime/engine/internal/extension_interface.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension/msg_handling.h"
#include "include_internal/axis_runtime/extension_context/extension_context.h"
#include "include_internal/axis_runtime/extension_group/extension_group.h"
#include "include_internal/axis_runtime/extension_group/metadata.h"
#include "include_internal/axis_runtime/extension_store/extension_store.h"
#include "include_internal/axis_runtime/extension_thread/extension_thread.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_utils/value/value.h"
#include "axis_runtime/app/app.h"
#include "axis_runtime/msg/cmd_result/cmd_result.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_utils/io/runloop.h"
#include "axis_utils/lib/event.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"

void axis_extension_thread_handle_start_msg_task(void *self_,
                                                axis_UNUSED void *arg) {
  axis_extension_thread_t *self = (axis_extension_thread_t *)self_;
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  axis_ASSERT(self->extension_group, "Should not happen.");

  axis_extension_group_load_metadata(self->extension_group);
}

static void axis_extension_thread_handle_in_msg_sync(
    axis_extension_thread_t *self, axis_shared_ptr_t *msg) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);
  axis_ASSERT(axis_msg_get_dest_cnt(msg) == 1,
             "When this function is executed, there should be only one "
             "destination remaining in the message's dest.");

  // Find the extension according to 'loc'.
  axis_loc_t *dest_loc = axis_msg_get_first_dest_loc(msg);
  axis_extension_t *extension = axis_extension_store_find_extension(
      self->extension_store, axis_string_get_raw_str(&dest_loc->extension_name),
      self->in_lock_mode ? false : true);
  if (!extension) {
    // axis_msg_dump(msg, NULL,
    //              "Failed to find destination extension %s for msg ^m in %s",
    //              axis_string_get_raw_str(&dest_loc->extension_name),
    //              axis_string_get_raw_str(&self->extension_group->name));

    // Return a result, so that the sender can know what's going on.
    if (axis_msg_get_type(msg) == axis_MSG_TYPE_CMD) {
      axis_shared_ptr_t *status =
          axis_cmd_result_create_from_cmd(axis_STATUS_CODE_ERROR, msg);
      axis_msg_set_property(
          status, "detail",
          axis_value_create_vstring(
              "The extension[%s] is invalid.",
              axis_string_get_raw_str(&dest_loc->extension_name)),
          NULL);

      axis_extension_thread_dispatch_msg(self, status);

      axis_shared_ptr_destroy(status);
    } else {
#if defined(_DEBUG)
      axis_ASSERT(0, "Should not happen.");
#endif
    }

    return;
  } else {
    if (extension->extension_thread != self) {
      // axis_msg_dump(msg, NULL, "Unexpected msg ^m for extension %s",
      //              axis_string_get_raw_str(&extension->name));

      axis_ASSERT(0, "Should not happen.");
    }

    axis_extension_handle_in_msg(extension, msg);
  }
}

static void axis_extension_thread_handle_in_msg_task(void *self_, void *arg) {
  axis_extension_thread_t *self = (axis_extension_thread_t *)self_;
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  axis_shared_ptr_t *msg = (axis_shared_ptr_t *)arg;
  axis_ASSERT(msg && axis_msg_check_integrity(msg), "Invalid argument.");
  axis_ASSERT(axis_msg_get_dest_cnt(msg) == 1, "Should not happen.");

  switch (axis_extension_thread_get_state(self)) {
    case axis_EXTENSION_THREAD_STATE_INIT:
    case axis_EXTENSION_THREAD_STATE_CREATING_EXTENSIONS:
#if defined(_DEBUG)
      // axis_msg_dump(msg, NULL,
      //              "A message (^m) comes when extension thread (%p) is in "
      //              "state (%d)",
      //              self, axis_extension_thread_get_state(self));
#endif

      // At this stage, the extensions have not been created yet, so any
      // received messages are placed into a `pending_msgs` list. Once the
      // extensions are created, the messages will be delivered to the
      // corresponding extensions.
      axis_list_push_smart_ptr_back(&self->pending_msgs, msg);
      break;

    case axis_EXTENSION_THREAD_STATE_NORMAL:
    case axis_EXTENSION_THREAD_STATE_PREPARE_TO_CLOSE:
      axis_extension_thread_handle_in_msg_sync(self, msg);
      break;

    case axis_EXTENSION_THREAD_STATE_CLOSED:
      // All extensions are removed from this extension thread, so the only
      // thing we can do is to discard this cmd result.
      break;

    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }

  axis_shared_ptr_destroy(msg);
}

static void axis_extension_thread_process_release_lock_mode_task(
    void *self_, axis_UNUSED void *arg) {
  axis_extension_thread_t *self = (axis_extension_thread_t *)self_;
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  // Unset `in_lock_mode` to reflect the effect of the below `axis_mutex_unlock`
  // releasing the block on the extension thread.
  self->in_lock_mode = false;

  int rc = axis_mutex_unlock(self->lock_mode_lock);
  axis_ASSERT(!rc, "Should not happen.");
}

// This task is used to allow the outer thread to wait for the extension thread
// to reach a certain point in time. Subsequently, the extension thread will
// be blocked in this function.
void axis_extension_thread_process_acquire_lock_mode_task(void *self_,
                                                         void *arg) {
  axis_extension_thread_t *self = (axis_extension_thread_t *)self_;
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  axis_acquire_lock_mode_result_t *acquire_result =
      (axis_acquire_lock_mode_result_t *)arg;
  axis_ASSERT(acquire_result, "Invalid argument.");

  // Because the extension thread is about to acquire the lock mode lock to
  // prevent the outer thread from directly using the TEN world, a task to
  // release the lock mode is inserted, allowing the extension thread to exit
  // this mode and giving the outer thread a chance to acquire the lock mode
  // lock.
  int rc = axis_runloop_post_task_tail(
      self->runloop, axis_extension_thread_process_release_lock_mode_task, self,
      NULL);
  axis_ASSERT(!rc, "Should not happen.");

  // Set `in_lock_mode` to reflect the effect of the below `axis_mutex_lock`
  // blocking the extension thread.
  self->in_lock_mode = true;

  // Inform the outer thread that the extension thread has also entered the
  // lock mode.
  axis_event_set(acquire_result->completed);

  rc = axis_mutex_lock(self->lock_mode_lock);
  axis_ASSERT(!rc, "Should not happen.");
}

void axis_extension_thread_handle_in_msg_async(axis_extension_thread_t *self,
                                              axis_shared_ptr_t *msg) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_thread_check_integrity(self, false),
             "Invalid use of extension %p.", self);
  axis_ASSERT(msg && (axis_msg_get_dest_cnt(msg) == 1),
             "When this function is executed, there should be only one "
             "destination remaining in the message's dest.");

  // This function would be called from threads other than the specified
  // extension thread. However, because the runloop relevant functions called in
  // this function have thread-safety protection of mutex in them, we do not
  // need to use any further locking mechanisms in this function to do any
  // protection.

  if (axis_runloop_task_queue_size(self->runloop) >=
      EXTENSION_THREAD_QUEUE_SIZE) {
    if (!axis_msg_is_cmd_and_result(msg)) {
      axis_LOGW(
          "Discard a data-like message (%s) because extension thread input "
          "buffer is full.",
          axis_msg_get_name(msg));
      return;
    }
  }

  msg = axis_shared_ptr_clone(msg);

  axis_runloop_post_task_tail(
      self->runloop, axis_extension_thread_handle_in_msg_task, self, msg);
}

void axis_extension_thread_dispatch_msg(axis_extension_thread_t *self,
                                       axis_shared_ptr_t *msg) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);
  axis_ASSERT(msg && (axis_msg_get_dest_cnt(msg) == 1),
             "When this function is executed, there should be only one "
             "destination remaining in the message's dest.");

  axis_loc_t *dest_loc = axis_msg_get_first_dest_loc(msg);
  axis_ASSERT(dest_loc && axis_loc_check_integrity(dest_loc),
             "Should not happen.");

  axis_extension_group_t *extension_group = self->extension_group;
  axis_ASSERT(extension_group &&
                 axis_extension_group_check_integrity(extension_group, true),
             "Should not happen.");

  axis_engine_t *engine = self->extension_context->engine;
  axis_ASSERT(engine && axis_engine_check_integrity(engine, false),
             "Should not happen.");

  axis_app_t *app = engine->app;
  axis_ASSERT(app && axis_app_check_integrity(app, false), "Should not happen.");

  if (!axis_string_is_equal_c_str(&dest_loc->app_uri, axis_app_get_uri(app))) {
    axis_ASSERT(!axis_string_is_empty(&dest_loc->app_uri), "Should not happen.");

    // Because the remote might be added or deleted at runtime, so ask the
    // engine to route the message to the specified remote to keep thread
    // safety.
    axis_engine_push_to_extension_msgs_queue(engine, msg);
  } else {
    if (
        // It means asking the current app to do something.
        axis_string_is_empty(&dest_loc->graph_id) ||
        // It means asking another engine in the same app to do something.
        !axis_string_is_equal(&dest_loc->graph_id, &engine->graph_id)) {
      // The message should not be handled in this engine, so ask the app to
      // handle this message.

      axis_app_push_to_in_msgs_queue(app, msg);
    } else {
      if (axis_string_is_empty(&dest_loc->extension_group_name)) {
        // Because the destination is the current engine, so ask the engine to
        // handle this message.

        axis_engine_push_to_extension_msgs_queue(engine, msg);
      } else {
        if (!axis_string_is_equal(&dest_loc->extension_group_name,
                                 &extension_group->name)) {
          // Find the correct extension thread to handle this message.

          axis_engine_push_to_extension_msgs_queue(engine, msg);
        } else {
          // The message should be handled in the current extension thread, so
          // dispatch the message to the current extension thread.
          axis_extension_thread_handle_in_msg_sync(self, msg);
        }
      }
    }
  }
}
