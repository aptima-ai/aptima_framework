//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/extension_thread/msg_interface/common.h"

#include "include_internal/ten_runtime/app/app.h"
#include "include_internal/ten_runtime/app/msg_interface/common.h"
#include "include_internal/ten_runtime/common/loc.h"
#include "include_internal/ten_runtime/engine/engine.h"
#include "include_internal/ten_runtime/engine/internal/extension_interface.h"
#include "include_internal/ten_runtime/extension/extension.h"
#include "include_internal/ten_runtime/extension/msg_handling.h"
#include "include_internal/ten_runtime/extension_context/extension_context.h"
#include "include_internal/ten_runtime/extension_group/extension_group.h"
#include "include_internal/ten_runtime/extension_group/metadata.h"
#include "include_internal/ten_runtime/extension_store/extension_store.h"
#include "include_internal/ten_runtime/extension_thread/extension_thread.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "include_internal/ten_utils/value/value.h"
#include "ten_runtime/app/app.h"
#include "ten_runtime/msg/cmd_result/cmd_result.h"
#include "ten_runtime/ten_env/ten_env.h"
#include "ten_utils/io/runloop.h"
#include "ten_utils/lib/event.h"
#include "ten_utils/lib/smart_ptr.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"

void ten_extension_thread_handle_start_msg_task(void *self_,
                                                TEN_UNUSED void *arg) {
  ten_extension_thread_t *self = (ten_extension_thread_t *)self_;
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  TEN_ASSERT(self->extension_group, "Should not happen.");

  ten_extension_group_load_metadata(self->extension_group);
}

static void ten_extension_thread_handle_in_msg_sync(
    ten_extension_thread_t *self, ten_shared_ptr_t *msg) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);
  TEN_ASSERT(ten_msg_get_dest_cnt(msg) == 1,
             "When this function is executed, there should be only one "
             "destination remaining in the message's dest.");

  // Find the extension according to 'loc'.
  ten_loc_t *dest_loc = ten_msg_get_first_dest_loc(msg);
  ten_extension_t *extension = ten_extension_store_find_extension(
      self->extension_store, ten_string_get_raw_str(&dest_loc->extension_name),
      self->in_lock_mode ? false : true);
  if (!extension) {
    // ten_msg_dump(msg, NULL,
    //              "Failed to find destination extension %s for msg ^m in %s",
    //              ten_string_get_raw_str(&dest_loc->extension_name),
    //              ten_string_get_raw_str(&self->extension_group->name));

    // Return a result, so that the sender can know what's going on.
    if (ten_msg_get_type(msg) == TEN_MSG_TYPE_CMD) {
      ten_shared_ptr_t *status =
          ten_cmd_result_create_from_cmd(TEN_STATUS_CODE_ERROR, msg);
      ten_msg_set_property(
          status, "detail",
          ten_value_create_vstring(
              "The extension[%s] is invalid.",
              ten_string_get_raw_str(&dest_loc->extension_name)),
          NULL);

      ten_extension_thread_dispatch_msg(self, status);

      ten_shared_ptr_destroy(status);
    } else {
#if defined(_DEBUG)
      TEN_ASSERT(0, "Should not happen.");
#endif
    }

    return;
  } else {
    if (extension->extension_thread != self) {
      // ten_msg_dump(msg, NULL, "Unexpected msg ^m for extension %s",
      //              ten_string_get_raw_str(&extension->name));

      TEN_ASSERT(0, "Should not happen.");
    }

    ten_extension_handle_in_msg(extension, msg);
  }
}

static void ten_extension_thread_handle_in_msg_task(void *self_, void *arg) {
  ten_extension_thread_t *self = (ten_extension_thread_t *)self_;
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  ten_shared_ptr_t *msg = (ten_shared_ptr_t *)arg;
  TEN_ASSERT(msg && ten_msg_check_integrity(msg), "Invalid argument.");
  TEN_ASSERT(ten_msg_get_dest_cnt(msg) == 1, "Should not happen.");

  switch (ten_extension_thread_get_state(self)) {
    case TEN_EXTENSION_THREAD_STATE_INIT:
    case TEN_EXTENSION_THREAD_STATE_CREATING_EXTENSIONS:
#if defined(_DEBUG)
      // ten_msg_dump(msg, NULL,
      //              "A message (^m) comes when extension thread (%p) is in "
      //              "state (%d)",
      //              self, ten_extension_thread_get_state(self));
#endif

      // At this stage, the extensions have not been created yet, so any
      // received messages are placed into a `pending_msgs` list. Once the
      // extensions are created, the messages will be delivered to the
      // corresponding extensions.
      ten_list_push_smart_ptr_back(&self->pending_msgs, msg);
      break;

    case TEN_EXTENSION_THREAD_STATE_NORMAL:
    case TEN_EXTENSION_THREAD_STATE_PREPARE_TO_CLOSE:
      ten_extension_thread_handle_in_msg_sync(self, msg);
      break;

    case TEN_EXTENSION_THREAD_STATE_CLOSED:
      // All extensions are removed from this extension thread, so the only
      // thing we can do is to discard this cmd result.
      break;

    default:
      TEN_ASSERT(0, "Should not happen.");
      break;
  }

  ten_shared_ptr_destroy(msg);
}

static void ten_extension_thread_process_release_lock_mode_task(
    void *self_, TEN_UNUSED void *arg) {
  ten_extension_thread_t *self = (ten_extension_thread_t *)self_;
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  // Unset `in_lock_mode` to reflect the effect of the below `ten_mutex_unlock`
  // releasing the block on the extension thread.
  self->in_lock_mode = false;

  int rc = ten_mutex_unlock(self->lock_mode_lock);
  TEN_ASSERT(!rc, "Should not happen.");
}

// This task is used to allow the outer thread to wait for the extension thread
// to reach a certain point in time. Subsequently, the extension thread will
// be blocked in this function.
void ten_extension_thread_process_acquire_lock_mode_task(void *self_,
                                                         void *arg) {
  ten_extension_thread_t *self = (ten_extension_thread_t *)self_;
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  ten_acquire_lock_mode_result_t *acquire_result =
      (ten_acquire_lock_mode_result_t *)arg;
  TEN_ASSERT(acquire_result, "Invalid argument.");

  // Because the extension thread is about to acquire the lock mode lock to
  // prevent the outer thread from directly using the TEN world, a task to
  // release the lock mode is inserted, allowing the extension thread to exit
  // this mode and giving the outer thread a chance to acquire the lock mode
  // lock.
  int rc = ten_runloop_post_task_tail(
      self->runloop, ten_extension_thread_process_release_lock_mode_task, self,
      NULL);
  TEN_ASSERT(!rc, "Should not happen.");

  // Set `in_lock_mode` to reflect the effect of the below `ten_mutex_lock`
  // blocking the extension thread.
  self->in_lock_mode = true;

  // Inform the outer thread that the extension thread has also entered the
  // lock mode.
  ten_event_set(acquire_result->completed);

  rc = ten_mutex_lock(self->lock_mode_lock);
  TEN_ASSERT(!rc, "Should not happen.");
}

void ten_extension_thread_handle_in_msg_async(ten_extension_thread_t *self,
                                              ten_shared_ptr_t *msg) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(self, false),
             "Invalid use of extension %p.", self);
  TEN_ASSERT(msg && (ten_msg_get_dest_cnt(msg) == 1),
             "When this function is executed, there should be only one "
             "destination remaining in the message's dest.");

  // This function would be called from threads other than the specified
  // extension thread. However, because the runloop relevant functions called in
  // this function have thread-safety protection of mutex in them, we do not
  // need to use any further locking mechanisms in this function to do any
  // protection.

  if (ten_runloop_task_queue_size(self->runloop) >=
      EXTENSION_THREAD_QUEUE_SIZE) {
    if (!ten_msg_is_cmd_and_result(msg)) {
      TEN_LOGW(
          "Discard a data-like message (%s) because extension thread input "
          "buffer is full.",
          ten_msg_get_name(msg));
      return;
    }
  }

  msg = ten_shared_ptr_clone(msg);

  ten_runloop_post_task_tail(
      self->runloop, ten_extension_thread_handle_in_msg_task, self, msg);
}

void ten_extension_thread_dispatch_msg(ten_extension_thread_t *self,
                                       ten_shared_ptr_t *msg) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);
  TEN_ASSERT(msg && (ten_msg_get_dest_cnt(msg) == 1),
             "When this function is executed, there should be only one "
             "destination remaining in the message's dest.");

  ten_loc_t *dest_loc = ten_msg_get_first_dest_loc(msg);
  TEN_ASSERT(dest_loc && ten_loc_check_integrity(dest_loc),
             "Should not happen.");

  ten_extension_group_t *extension_group = self->extension_group;
  TEN_ASSERT(extension_group &&
                 ten_extension_group_check_integrity(extension_group, true),
             "Should not happen.");

  ten_engine_t *engine = self->extension_context->engine;
  TEN_ASSERT(engine && ten_engine_check_integrity(engine, false),
             "Should not happen.");

  ten_app_t *app = engine->app;
  TEN_ASSERT(app && ten_app_check_integrity(app, false), "Should not happen.");

  if (!ten_string_is_equal_c_str(&dest_loc->app_uri, ten_app_get_uri(app))) {
    TEN_ASSERT(!ten_string_is_empty(&dest_loc->app_uri), "Should not happen.");

    // Because the remote might be added or deleted at runtime, so ask the
    // engine to route the message to the specified remote to keep thread
    // safety.
    ten_engine_push_to_extension_msgs_queue(engine, msg);
  } else {
    if (
        // It means asking the current app to do something.
        ten_string_is_empty(&dest_loc->graph_id) ||
        // It means asking another engine in the same app to do something.
        !ten_string_is_equal(&dest_loc->graph_id, &engine->graph_id)) {
      // The message should not be handled in this engine, so ask the app to
      // handle this message.

      ten_app_push_to_in_msgs_queue(app, msg);
    } else {
      if (ten_string_is_empty(&dest_loc->extension_group_name)) {
        // Because the destination is the current engine, so ask the engine to
        // handle this message.

        ten_engine_push_to_extension_msgs_queue(engine, msg);
      } else {
        if (!ten_string_is_equal(&dest_loc->extension_group_name,
                                 &extension_group->name)) {
          // Find the correct extension thread to handle this message.

          ten_engine_push_to_extension_msgs_queue(engine, msg);
        } else {
          // The message should be handled in the current extension thread, so
          // dispatch the message to the current extension thread.
          ten_extension_thread_handle_in_msg_sync(self, msg);
        }
      }
    }
  }
}
