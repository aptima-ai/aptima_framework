//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/extension_thread/extension_thread.h"

#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>

#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/common/loc.h"
#include "include_internal/axis_runtime/engine/engine.h"
#include "include_internal/axis_runtime/engine/internal/thread.h"
#include "include_internal/axis_runtime/engine/msg_interface/common.h"
#include "include_internal/axis_runtime/engine/on_xxx.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension_context/extension_context.h"
#include "include_internal/axis_runtime/extension_group/extension_group.h"
#include "include_internal/axis_runtime/extension_group/on_xxx.h"
#include "include_internal/axis_runtime/extension_store/extension_store.h"
#include "include_internal/axis_runtime/extension_thread/msg_interface/common.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "include_internal/axis_utils/sanitizer/thread_check.h"
#include "axis_runtime/extension/extension.h"
#include "axis_runtime/msg/cmd/stop_graph/cmd.h"
#include "axis_runtime/msg/cmd_result/cmd_result.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_utils/container/list.h"
#include "axis_utils/io/runloop.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/event.h"
#include "axis_utils/lib/mutex.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/lib/thread.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"
#include "axis_utils/sanitizer/thread_check.h"

bool axis_extension_thread_check_integrity_if_in_lock_mode(
    axis_extension_thread_t *self) {
  axis_ASSERT(self, "Should not happen.");

  if (self->in_lock_mode) {
    return true;
  }

  return false;
}

bool axis_extension_thread_check_integrity(axis_extension_thread_t *self,
                                          bool check_thread) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_EXTENSION_THREAD_SIGNATURE) {
    axis_ASSERT(0,
               "Failed to pass extension_thread signature checking: %" PRId64,
               self->signature);
    return false;
  }

  if (check_thread) {
    if (axis_extension_thread_check_integrity_if_in_lock_mode(self)) {
      return true;
    }

    if (!axis_sanitizer_thread_check_do_check(&self->thread_check)) {
      axis_ASSERT(0, "Failed to pass extension_thread thread safety checking.");
      return false;
    }
  }

  return true;
}

axis_extension_thread_t *axis_extension_thread_create(void) {
  axis_extension_thread_t *self =
      (axis_extension_thread_t *)axis_MALLOC(sizeof(axis_extension_thread_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_signature_set(&self->signature,
                    (axis_signature_t)axis_EXTENSION_THREAD_SIGNATURE);

  self->state = axis_EXTENSION_THREAD_STATE_INIT;
  self->is_close_triggered = false;

  self->extension_context = NULL;
  self->extension_group = NULL;
  self->extension_store = axis_extension_store_create(
      offsetof(axis_extension_t, hh_in_extension_store));

  axis_list_init(&self->extensions);
  self->extensions_cnt_of_deleted = 0;

  axis_list_init(&self->pending_msgs);

  self->in_lock_mode = false;
  self->lock_mode_lock = axis_mutex_create();

  axis_sanitizer_thread_check_init(&self->thread_check);

  self->runloop = NULL;
  self->runloop_is_ready_to_use = axis_event_create(0, 0);

  return self;
}

static void axis_extension_thread_attach_to_group(
    axis_extension_thread_t *self, axis_extension_group_t *extension_group) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_thread_check_integrity(self, false),
             "Invalid use of extension_thread %p.", self);
  axis_ASSERT(extension_group, "Should not happen.");

  self->extension_group = extension_group;
}

void axis_extension_thread_attach_to_context_and_group(
    axis_extension_thread_t *self, axis_extension_context_t *extension_context,
    axis_extension_group_t *extension_group) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_thread_check_integrity(self, false),
             "Invalid use of extension_thread %p.", self);
  axis_ASSERT(extension_context && extension_group, "Should not happen.");

  self->extension_context = extension_context;
  axis_extension_thread_attach_to_group(self, extension_group);
}

void axis_extension_thread_destroy(axis_extension_thread_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_thread_check_integrity(self, false),
             "Invalid use of extension_thread %p.", self);

  // All the Extensions should have been destroyed.
  axis_ASSERT(axis_list_is_empty(&self->extensions), "Should not happen.");

  axis_signature_set(&self->signature, 0);

  axis_list_clear(&self->pending_msgs);

  if (self->runloop) {
    axis_runloop_destroy(self->runloop);
    self->runloop = NULL;
  }

  axis_event_destroy(self->runloop_is_ready_to_use);

  axis_sanitizer_thread_check_deinit(&self->thread_check);
  axis_extension_store_destroy(self->extension_store);

  axis_mutex_destroy(self->lock_mode_lock);
  self->lock_mode_lock = NULL;

  axis_FREE(self);
}

void axis_extension_thread_remove_from_extension_context(
    axis_extension_thread_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);
  axis_ASSERT(axis_engine_check_integrity(self->extension_context->engine, true),
             "Should not happen.");

  self->extension_group->extension_thread = NULL;

  axis_extension_thread_destroy(self);
}

// Notify extension context (engine) that we (extension thread) are closed, so
// that engine can join this thread to prevent memory leak.
static void axis_extension_thread_notify_engine_we_are_closed(
    axis_extension_thread_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  axis_engine_t *engine = self->extension_context->engine;
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: In the closing flow, the closing of the engine is always
  // after the closing of the extension thread, so its thread safe to access the
  // runloop of the engine here.
  axis_ASSERT(engine && axis_engine_check_integrity(engine, false),
             "Should not happen.");

  axis_runloop_t *engine_loop = axis_engine_get_attached_runloop(engine);
  axis_ASSERT(engine_loop, "Should not happen.");

  axis_extension_thread_set_state(self, axis_EXTENSION_THREAD_STATE_CLOSED);

  axis_runloop_post_task_tail(engine_loop, axis_engine_on_extension_thread_closed,
                             engine, self);
}

axis_runloop_t *axis_extension_thread_get_attached_runloop(
    axis_extension_thread_t *self) {
  axis_ASSERT(self &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: This function is intended to be called in
                 // threads other than the extension thread itself.
                 axis_extension_thread_check_integrity(self, false),
             "Should not happen.");

  return self->runloop;
}

static void axis_extension_thread_inherit_thread_ownership(
    axis_extension_thread_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: The correct threading ownership will be setup
  // soon, so we can _not_ check thread safety here.
  axis_ASSERT(axis_extension_thread_check_integrity(self, false),
             "Invalid use extension thread %p.", self);

  // Move the ownership of the extension thread relevant resources to the
  // belonging extension thread.
  axis_sanitizer_thread_check_init_with_current_thread(&self->thread_check);
  axis_sanitizer_thread_check_inherit_from(&self->extension_store->thread_check,
                                          &self->thread_check);

  axis_extension_group_t *extension_group = self->extension_group;
  axis_ASSERT(extension_group, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: The correct threading ownership will be setup
  // soon, so we can _not_ check thread safety here.
  axis_ASSERT(axis_extension_group_check_integrity(extension_group, false),
             "Invalid use extension group %p.", extension_group);

  axis_sanitizer_thread_check_inherit_from(&extension_group->thread_check,
                                          &self->thread_check);
  axis_sanitizer_thread_check_inherit_from(
      &extension_group->axis_env->thread_check, &self->thread_check);
}

void *axis_extension_thread_main_actual(axis_extension_thread_t *self) {
  axis_LOGD("Extension thread is started");

  axis_ASSERT(self &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: The correct threading ownership will be setup
                 // soon, so we can _not_ check thread safety here.
                 axis_extension_thread_check_integrity(self, false),
             "Should not happen.");

  axis_extension_thread_inherit_thread_ownership(self);

  // The runloop should be created in its own thread.
  self->runloop = axis_runloop_create(NULL);
  axis_ASSERT(self->runloop, "Should not happen.");

  axis_runloop_post_task_tail(
      self->runloop, axis_extension_thread_handle_start_msg_task, self, NULL);

  // Before actually starting the extension thread's runloop, first notify the
  // engine (extension_context) that the extension thread's runloop is ready for
  // use.
  axis_event_set(self->runloop_is_ready_to_use);

  // Run the extension thread event loop.
  axis_runloop_run(self->runloop);

  axis_extension_thread_notify_engine_we_are_closed(self);

  axis_LOGD("Extension thread is stopped.");

  return NULL;
}

// This is the extension thread.
static void *axis_extension_thread_main(void *self_) {
  axis_extension_thread_t *self = (axis_extension_thread_t *)self_;
  return axis_extension_thread_main_actual(self);
}

void axis_extension_thread_start(axis_extension_thread_t *self) {
  axis_ASSERT(self &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: because the extension thread has not started
                 // yet, we can _not_ check thread safety here.
                 axis_extension_thread_check_integrity(self, false),
             "Should not happen.");

  axis_thread_create(axis_string_get_raw_str(&self->extension_group->name),
                    axis_extension_thread_main, self);

  // The runloop of the extension_thread is created within the extension thread
  // itself, which introduces a time gap. If the engine (extension_context)
  // attempts to post a task to the runloop of extension_thread before the
  // runloop has been created, it would result in a segmentation fault since the
  // runloop would still be NULL. There are two approaches to handle this
  // situation:
  //
  // 1) Protect both the extension_thread and engine access to
  //    extension_thread::runloop with a mutex. But this is too heavy.
  // 2) The approach adopted here is to have the engine thread wait briefly
  //    until the runloop is successfully created by the extension_thread before
  //    proceeding. This eliminates the need to lock every time the runloop is
  //    accessed.
  axis_event_wait(self->runloop_is_ready_to_use, -1);
}

static void axis_extension_thread_on_triggering_close(void *self_,
                                                     axis_UNUSED void *arg) {
  axis_extension_thread_t *self = self_;
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  // The closing flow should be executed only once.
  if (self->is_close_triggered) {
    return;
  }

  self->is_close_triggered = true;

  switch (self->state) {
    case axis_EXTENSION_THREAD_STATE_INIT:
      // Enter the deinit flow of the extension group directly.
      axis_extension_group_on_deinit(self->extension_group);
      break;

    case axis_EXTENSION_THREAD_STATE_CREATING_EXTENSIONS:
      // We need to wait until `on_create_extensions_done()` is called, as that
      // is the point when all the created extensions can be retrieved to begin
      // the close process. Otherwise, memory leaks caused by those extensions
      // may occur.
      break;

    case axis_EXTENSION_THREAD_STATE_NORMAL:
      axis_extension_thread_stop_life_cycle_of_all_extensions(self);
      break;

    case axis_EXTENSION_THREAD_STATE_PREPARE_TO_CLOSE:
    case axis_EXTENSION_THREAD_STATE_CLOSED:
    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }
}

void axis_extension_thread_close(axis_extension_thread_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: this function is intended to be called in any threads.
  axis_ASSERT(axis_extension_thread_check_integrity(self, false),
             "Should not happen.");

  axis_LOGD("Try to close extension thread.");

  // Notify extension thread that it is about to close.
  axis_runloop_post_task_tail(
      self->runloop, axis_extension_thread_on_triggering_close, self, NULL);
}

bool axis_extension_thread_call_by_me(axis_extension_thread_t *self) {
  axis_ASSERT(self &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: this function is intended to be called in any
                 // threads.
                 axis_extension_thread_check_integrity(self, false),
             "Should not happen.");

  return axis_thread_equal(NULL, axis_sanitizer_thread_check_get_belonging_thread(
                                    &self->thread_check));
}

bool axis_extension_thread_not_call_by_me(axis_extension_thread_t *self) {
  axis_ASSERT(self &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: this function is intended to be called in any
                 // threads.
                 axis_extension_thread_check_integrity(self, false),
             "Should not happen.");

  return !axis_extension_thread_call_by_me(self);
}

axis_EXTENSION_THREAD_STATE
axis_extension_thread_get_state(axis_extension_thread_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  return self->state;
}

void axis_extension_thread_set_state(axis_extension_thread_t *self,
                                    axis_EXTENSION_THREAD_STATE state) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  self->state = state;
}

static void axis_extension_thread_add_extension(axis_extension_thread_t *self,
                                               axis_extension_t *extension) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  axis_ASSERT(extension && axis_extension_check_integrity(extension, true),
             "Should not happen.");

  extension->extension_thread = self;

  axis_UNUSED bool rc =
      axis_extension_store_add_extension(self->extension_store, extension);
  axis_ASSERT(rc, "Should not happen.");
}

static void axis_extension_thread_stop_life_cycle_of_all_extensions_task(
    void *self, axis_UNUSED void *arg) {
  axis_extension_thread_t *extension_thread = self;
  axis_ASSERT(extension_thread &&
                 axis_extension_thread_check_integrity(extension_thread, true),
             "Invalid argument.");

  axis_extension_thread_stop_life_cycle_of_all_extensions(extension_thread);
}

/**
 * Begin processing all lifecycle stages of the extensions contained within the
 * extension thread. This means starting to invoke each extension's series of
 * lifecycle methods, beginning with `on_configure`.
 */
static void axis_extension_thread_start_life_cycle_of_all_extensions_task(
    void *self_, axis_UNUSED void *arg) {
  axis_extension_thread_t *self = self_;
  axis_ASSERT(axis_extension_thread_check_integrity(self, true),
             "Should not happen.");

  if (self->is_close_triggered) {
    return;
  }

  axis_extension_thread_set_state(self, axis_EXTENSION_THREAD_STATE_NORMAL);

  // From here, it begins calling a series of lifecycle methods for the
  // extension, starting with `on_configure`.

  axis_list_foreach (&self->extensions, iter) {
    axis_extension_t *extension = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(extension && axis_extension_check_integrity(extension, true),
               "Should not happen.");

    axis_extension_load_metadata(extension);
  }
}

/**
 * After the initialization of all extension threads in the engine (representing
 * a graph) is completed (regardless of whether the result is success or
 * failure), the engine needs to respond to the original requester of the graph
 * creation (i.e., a `start_graph` command) with a result.
 */
static void axis_engine_on_all_extension_threads_are_ready(
    axis_engine_t *self, axis_extension_thread_t *extension_thread) {
  axis_ASSERT(self && axis_engine_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(
      extension_thread &&
          // axis_NOLINTNEXTLINE(thread-check)
          // thread-check: this function does not access this extension_thread,
          // we just check if the arg is an axis_extension_thread_t.
          axis_extension_thread_check_integrity(extension_thread, false),
      "Should not happen.");

  axis_extension_context_t *extension_context = self->extension_context;
  axis_ASSERT(extension_context &&
                 axis_extension_context_check_integrity(extension_context, true),
             "Should not happen.");

  extension_context->extension_threads_cnt_of_ready++;
  if (extension_context->extension_threads_cnt_of_ready ==
      axis_list_size(&extension_context->extension_threads)) {
    bool error_occurred = false;

    // Check if there were any errors during the creation and/or initialization
    // of any extension thread/group. If errors are found, shut down the
    // engine/graph and return the corresponding result to the original
    // requester.

    axis_list_foreach (&extension_context->extension_groups, iter) {
      axis_extension_group_t *extension_group = axis_ptr_listnode_get(iter.node);
      axis_ASSERT(extension_group && axis_extension_group_check_integrity(
                                        extension_group, false),
                 "Should not happen.");

      if (!axis_error_is_success(&extension_group->err_before_ready)) {
        error_occurred = true;
        break;
      }
    }

    axis_shared_ptr_t *state_requester_cmd =
        extension_context->state_requester_cmd;
    axis_ASSERT(state_requester_cmd, "Should not happen.");

    axis_shared_ptr_t *cmd_result = NULL;
    if (error_occurred) {
      cmd_result = axis_cmd_result_create_from_cmd(axis_STATUS_CODE_ERROR,
                                                  state_requester_cmd);
    } else {
      axis_LOGD("[%s] All extension threads are initted.",
               axis_engine_get_id(self, true));

      axis_string_t *graph_id = &self->graph_id;

      const char *body_str =
          axis_string_is_empty(graph_id) ? "" : axis_string_get_raw_str(graph_id);

      cmd_result = axis_cmd_result_create_from_cmd(axis_STATUS_CODE_OK,
                                                  state_requester_cmd);
      axis_msg_set_property(cmd_result, "detail",
                           axis_value_create_string(body_str), NULL);

      // Mark the engine that it could start to handle messages.
      self->is_ready_to_handle_msg = true;

      axis_LOGD("[%s] Engine is ready to handle messages.",
               axis_engine_get_id(self, true));
    }

    axis_env_return_result(self->axis_env, cmd_result, state_requester_cmd, NULL,
                          NULL, NULL);
    axis_shared_ptr_destroy(cmd_result);

    axis_shared_ptr_destroy(state_requester_cmd);
    extension_context->state_requester_cmd = NULL;

    if (error_occurred) {
      axis_app_t *app = self->app;
      axis_ASSERT(app && axis_app_check_integrity(app, false),
                 "Invalid argument.");

      // This graph/engine will not be functioning properly, so it will be shut
      // down directly.
      axis_shared_ptr_t *stop_graph_cmd = axis_cmd_stop_graph_create();
      axis_msg_clear_and_set_dest(stop_graph_cmd, axis_app_get_uri(app),
                                 axis_engine_get_id(self, false), NULL, NULL,
                                 NULL);

      axis_env_send_cmd(self->axis_env, stop_graph_cmd, NULL, NULL, NULL);

      axis_shared_ptr_destroy(stop_graph_cmd);
    } else {
      // Because the engine is just ready to handle messages, hence, we trigger
      // the engine to handle any _pending_/_cached_ external messages if any.
      axis_engine_handle_in_msgs_async(self);
    }
  }
}

static void
axis_engine_find_extension_info_for_all_extensions_of_extension_thread(
    void *self_, void *arg) {
  axis_engine_t *self = self_;
  axis_ASSERT(self && axis_engine_check_integrity(self, true),
             "Should not happen.");

  axis_extension_context_t *extension_context = self->extension_context;
  axis_ASSERT(extension_context &&
                 axis_extension_context_check_integrity(extension_context, true),
             "Should not happen.");

  axis_UNUSED axis_extension_thread_t *extension_thread = arg;
  axis_ASSERT(extension_thread &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: this function does not access this
                 // extension_thread, we just check if the arg is an
                 // axis_extension_thread_t.
                 axis_extension_thread_check_integrity(extension_thread, false),
             "Should not happen.");

  axis_list_foreach (&extension_thread->extensions, iter) {
    axis_extension_t *extension = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(axis_extension_check_integrity(extension, false),
               "Should not happen.");

    // Setup 'extension_context' field, this is the most important field when
    // extension is initiating.
    extension->extension_context = extension_context;

    // Find the extension_info of the specified 'extension'.
    extension->extension_info =
        axis_extension_context_get_extension_info_by_name(
            extension_context, axis_app_get_uri(extension_context->engine->app),
            axis_engine_get_id(extension_context->engine, true),
            axis_extension_group_get_name(extension_thread->extension_group,
                                         false),
            axis_extension_get_name(extension, false));
  }

  if (extension_thread->is_close_triggered) {
    axis_runloop_post_task_tail(
        axis_extension_thread_get_attached_runloop(extension_thread),
        axis_extension_thread_stop_life_cycle_of_all_extensions_task,
        extension_thread, NULL);
  } else {
    axis_engine_on_all_extension_threads_are_ready(self, extension_thread);

    axis_runloop_post_task_tail(
        axis_extension_thread_get_attached_runloop(extension_thread),
        axis_extension_thread_start_life_cycle_of_all_extensions_task,
        extension_thread, NULL);
  }
}

void axis_extension_thread_add_all_created_extensions(
    axis_extension_thread_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  axis_extension_context_t *extension_context = self->extension_context;
  axis_ASSERT(
      extension_context &&
          // axis_NOLINTNEXTLINE(thread-check)
          // thread-check: We are in the extension thread, and throughout the
          // entire lifecycle of the extension, the extension_context where
          // the extension resides remains unchanged. Even in the closing
          // flow, the extension_context is closed later than the extension
          // itself. Therefore, using a pointer to the extension_context
          // within the extension thread is thread-safe.
          axis_extension_context_check_integrity(extension_context, false),
      "Should not happen.");

  axis_list_foreach (&self->extensions, iter) {
    axis_extension_t *extension = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(axis_extension_check_integrity(extension, true),
               "Should not happen.");

    // Correct the belonging_thread of the contained path_table.
    axis_sanitizer_thread_check_set_belonging_thread_to_current_thread(
        &extension->path_table->thread_check);

    axis_extension_thread_add_extension(self, extension);
  }

  // Notify the engine to handle those newly created extensions.

  axis_engine_t *engine = extension_context->engine;
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: The runloop of the engine will not be changed during the
  // whole lifetime of the extension thread, so it's thread safe to access it
  // here.
  axis_ASSERT(engine && axis_engine_check_integrity(engine, false),
             "Should not happen.");

  axis_runloop_post_task_tail(
      axis_engine_get_attached_runloop(engine),
      axis_engine_find_extension_info_for_all_extensions_of_extension_thread,
      engine, self);
}
