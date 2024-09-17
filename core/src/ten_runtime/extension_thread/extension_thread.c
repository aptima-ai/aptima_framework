//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "include_internal/ten_runtime/extension_thread/extension_thread.h"

#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>

#include "include_internal/ten_runtime/app/app.h"
#include "include_internal/ten_runtime/common/loc.h"
#include "include_internal/ten_runtime/engine/engine.h"
#include "include_internal/ten_runtime/engine/internal/thread.h"
#include "include_internal/ten_runtime/engine/on_xxx.h"
#include "include_internal/ten_runtime/extension/extension.h"
#include "include_internal/ten_runtime/extension_context/extension_context.h"
#include "include_internal/ten_runtime/extension_context/internal/add_extension.h"
#include "include_internal/ten_runtime/extension_group/extension_group.h"
#include "include_internal/ten_runtime/extension_store/extension_store.h"
#include "include_internal/ten_runtime/extension_thread/msg_interface/common.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "ten_utils/macro/check.h"
#include "include_internal/ten_utils/sanitizer/thread_check.h"
#include "ten_runtime/extension/extension.h"
#include "ten_runtime/ten_env/ten_env.h"
#include "ten_utils/container/list.h"
#include "ten_utils/io/runloop.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/mutex.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/lib/thread.h"
#include "ten_utils/macro/mark.h"
#include "ten_utils/sanitizer/thread_check.h"

bool ten_extension_thread_check_integrity_if_in_lock_mode(
    ten_extension_thread_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  if (self->in_lock_mode) {
    return true;
  }

  return false;
}

bool ten_extension_thread_check_integrity(ten_extension_thread_t *self,
                                          bool check_thread) {
  TEN_ASSERT(self, "Should not happen.");

  if (ten_signature_get(&self->signature) !=
      (ten_signature_t)TEN_EXTENSION_THREAD_SIGNATURE) {
    TEN_ASSERT(0,
               "Failed to pass extension_thread signature checking: %" PRId64,
               self->signature);
    return false;
  }

  if (check_thread) {
    if (ten_extension_thread_check_integrity_if_in_lock_mode(self)) {
      return true;
    }

    if (!ten_sanitizer_thread_check_do_check(&self->thread_check)) {
      TEN_ASSERT(0, "Failed to pass extension_thread thread safety checking.");
      return false;
    }
  }

  return true;
}

ten_extension_thread_t *ten_extension_thread_create(void) {
  ten_extension_thread_t *self =
      (ten_extension_thread_t *)TEN_MALLOC(sizeof(ten_extension_thread_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  ten_signature_set(&self->signature,
                    (ten_signature_t)TEN_EXTENSION_THREAD_SIGNATURE);

  self->state = TEN_EXTENSION_THREAD_STATE_INIT;

  self->extension_context = NULL;
  self->extension_group = NULL;
  self->extension_store = ten_extension_store_create(
      offsetof(ten_extension_t, hh_in_extension_thread_extension_store));

  ten_list_init(&self->extensions);
  self->extensions_cnt_of_added_to_engine = 0;
  self->extensions_cnt_of_deleted_from_engine = 0;
  self->extensions_cnt_of_on_init_done = 0;
  self->extensions_cnt_of_on_start_done = 0;
  self->extensions_cnt_of_on_stop_done = 0;
  self->extensions_cnt_of_set_closing_flag = 0;

  ten_list_init(&self->pending_msgs);

  self->in_lock_mode = false;
  self->lock_mode_lock = ten_mutex_create();

  ten_sanitizer_thread_check_init(&self->thread_check);
  self->runloop = NULL;

  return self;
}

void ten_extension_thread_attach_to_group(
    ten_extension_thread_t *self, ten_extension_group_t *extension_group) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(self, false),
             "Invalid use of extension_thread %p.", self);
  TEN_ASSERT(extension_group, "Should not happen.");

  self->extension_group = extension_group;
}

void ten_extension_thread_attach_to_context_and_group(
    ten_extension_thread_t *self, ten_extension_context_t *extension_context,
    ten_extension_group_t *extension_group) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(self, false),
             "Invalid use of extension_thread %p.", self);
  TEN_ASSERT(extension_context && extension_group, "Should not happen.");

  self->extension_context = extension_context;
  ten_extension_thread_attach_to_group(self, extension_group);
}

void ten_extension_thread_destroy(ten_extension_thread_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(self, false),
             "Invalid use of extension_thread %p.", self);

  // All the Extensions should have been destroyed.
  TEN_ASSERT(ten_list_is_empty(&self->extensions), "Should not happen.");

  ten_signature_set(&self->signature, 0);

  ten_list_clear(&self->pending_msgs);

  if (self->runloop) {
    ten_runloop_destroy(self->runloop);
    self->runloop = NULL;
  }

  ten_sanitizer_thread_check_deinit(&self->thread_check);
  ten_extension_store_destroy(self->extension_store);

  ten_mutex_destroy(self->lock_mode_lock);
  self->lock_mode_lock = NULL;

  TEN_FREE(self);
}

void ten_extension_thread_remove_from_extension_context(
    ten_extension_thread_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);
  TEN_ASSERT(ten_engine_check_integrity(self->extension_context->engine, true),
             "Should not happen.");

  self->extension_group->extension_thread = NULL;

  ten_extension_thread_destroy(self);
}

// Notify extension context (engine) that we (extension thread) are closed, so
// that engine can join this thread to prevent memory leak.
static void ten_extension_thread_notify_engine_we_are_closed(
    ten_extension_thread_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  ten_engine_t *engine = self->extension_context->engine;
  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: In the closing flow, the closing of the engine is always
  // after the closing of the extension thread, so its thread safe to access the
  // runloop of the engine here.
  TEN_ASSERT(engine && ten_engine_check_integrity(engine, false),
             "Should not happen.");

  ten_runloop_t *engine_loop = ten_engine_get_attached_runloop(engine);
  TEN_ASSERT(engine_loop, "Should not happen.");

  ten_extension_thread_set_state(self, TEN_EXTENSION_THREAD_STATE_CLOSED);

  ten_runloop_post_task_tail(engine_loop, ten_engine_on_extension_thread_closed,
                             engine, self);
}

static void ten_extension_thread_start_runloop(ten_extension_thread_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  ten_runloop_post_task_tail(
      self->runloop, ten_extension_thread_handle_start_msg_task, self, NULL);
}

ten_runloop_t *ten_extension_thread_get_attached_runloop(
    ten_extension_thread_t *self) {
  TEN_ASSERT(self &&
                 // TEN_NOLINTNEXTLINE(thread-check)
                 // thread-check: This function is intended to be called in
                 // threads other than the extension thread itself.
                 ten_extension_thread_check_integrity(self, false),
             "Should not happen.");

  return self->runloop;
}

static void ten_extension_thread_inherit_thread_ownership(
    ten_extension_thread_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: The correct threading ownership will be setup
  // soon, so we can _not_ check thread safety here.
  TEN_ASSERT(ten_extension_thread_check_integrity(self, false),
             "Invalid use extension thread %p.", self);

  // Move the ownership of the extension thread relevant resources to the
  // belonging extension thread.
  ten_sanitizer_thread_check_init_with_current_thread(&self->thread_check);
  ten_sanitizer_thread_check_inherit_from(&self->extension_store->thread_check,
                                          &self->thread_check);

  ten_extension_group_t *extension_group = self->extension_group;
  TEN_ASSERT(extension_group, "Invalid argument.");
  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: The correct threading ownership will be setup
  // soon, so we can _not_ check thread safety here.
  TEN_ASSERT(ten_extension_group_check_integrity(extension_group, false),
             "Invalid use extension group %p.", extension_group);

  ten_sanitizer_thread_check_inherit_from(&extension_group->thread_check,
                                          &self->thread_check);
  ten_sanitizer_thread_check_inherit_from(
      &extension_group->ten_env->thread_check, &self->thread_check);
}

void *ten_extension_thread_main_actual(ten_extension_thread_t *self) {
  TEN_LOGD("Extension thread is started");

  TEN_ASSERT(self &&
                 // TEN_NOLINTNEXTLINE(thread-check)
                 // thread-check: The correct threading ownership will be setup
                 // soon, so we can _not_ check thread safety here.
                 ten_extension_thread_check_integrity(self, false),
             "Should not happen.");

  ten_extension_thread_inherit_thread_ownership(self);

  // The runloop should be created in its own thread.
  self->runloop = ten_runloop_create(NULL);
  TEN_ASSERT(self->runloop, "Should not happen.");

  // Run the extension thread event loop
  ten_extension_thread_start_runloop(self);
  ten_runloop_run(self->runloop);

  ten_extension_thread_notify_engine_we_are_closed(self);

  TEN_LOGD("Extension thread is stopped.");

  return NULL;
}

// This is the extension thread.
static void *ten_extension_thread_main(void *self_) {
  ten_extension_thread_t *self = (ten_extension_thread_t *)self_;
  return ten_extension_thread_main_actual(self);
}

void ten_extension_thread_start(ten_extension_thread_t *self) {
  TEN_ASSERT(self &&
                 // TEN_NOLINTNEXTLINE(thread-check)
                 // thread-check: because the extension thread has not started
                 // yet, we can _not_ check thread safety here.
                 ten_extension_thread_check_integrity(self, false),
             "Should not happen.");

  ten_thread_create("extension thread", ten_extension_thread_main, self);
}

static void ten_extension_thread_on_triggering_close(void *self_,
                                                     TEN_UNUSED void *arg) {
  ten_extension_thread_t *self = self_;
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  // The closing flow should be executed only once.
  if (ten_extension_thread_get_state(self) >=
      TEN_EXTENSION_THREAD_STATE_PREPARE_TO_CLOSE) {
    return;
  }

  ten_extension_thread_set_state(self,
                                 TEN_EXTENSION_THREAD_STATE_PREPARE_TO_CLOSE);

  // Loop for all the containing extensions, and call their on_stop().
  ten_list_foreach (&self->extensions, iter) {
    ten_extension_t *extension = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(ten_extension_check_integrity(extension, true),
               "Should not happen.");

    ten_extension_on_stop(extension);
  }
}

void ten_extension_thread_close(ten_extension_thread_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: this function is intended to be called in any threads.
  TEN_ASSERT(ten_extension_thread_check_integrity(self, false),
             "Should not happen.");

  TEN_LOGD("Try to close extension thread.");

  // Notify extension thread that it is about to close.
  ten_runloop_post_task_tail(
      self->runloop, ten_extension_thread_on_triggering_close, self, NULL);
}

bool ten_extension_thread_call_by_me(ten_extension_thread_t *self) {
  TEN_ASSERT(self &&
                 // TEN_NOLINTNEXTLINE(thread-check)
                 // thread-check: this function is intended to be called in any
                 // threads.
                 ten_extension_thread_check_integrity(self, false),
             "Should not happen.");

  return ten_thread_equal(NULL, ten_sanitizer_thread_check_get_belonging_thread(
                                    &self->thread_check));
}

bool ten_extension_thread_not_call_by_me(ten_extension_thread_t *self) {
  TEN_ASSERT(self &&
                 // TEN_NOLINTNEXTLINE(thread-check)
                 // thread-check: this function is intended to be called in any
                 // threads.
                 ten_extension_thread_check_integrity(self, false),
             "Should not happen.");

  return !ten_extension_thread_call_by_me(self);
}

TEN_EXTENSION_THREAD_STATE
ten_extension_thread_get_state(ten_extension_thread_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  return self->state;
}

void ten_extension_thread_set_state(ten_extension_thread_t *self,
                                    TEN_EXTENSION_THREAD_STATE state) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  self->state = state;
}

void ten_extension_thread_determine_all_extension_dest_from_graph(
    ten_extension_thread_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  TEN_LOGD("Determine destinations of all extensions from graph.");

  ten_list_foreach (&self->extensions, iter) {
    ten_extension_t *extension = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(ten_extension_check_integrity(extension, true),
               "Should not happen.");

    // Find the extension_info of the specified 'extension'.
    extension->extension_info =
        ten_extension_context_get_extension_info_by_name(
            self->extension_context,
            ten_string_get_raw_str(
                ten_app_get_uri(self->extension_context->engine->app)),
            ten_string_get_raw_str(
                &self->extension_context->engine->graph_name),
            ten_string_get_raw_str(
                &extension->extension_thread->extension_group->name),
            ten_string_get_raw_str(&extension->name));

    // Check if this extension is involved in the graph, and if yes, find
    // out all the destination extension.
    if (extension->extension_info) {
      ten_extension_determine_all_dest_extension(extension,
                                                 self->extension_context);
    }
  }
}

static void ten_extension_thread_add_extension(ten_extension_thread_t *self,
                                               ten_extension_t *extension) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  TEN_ASSERT(extension && ten_extension_check_integrity(extension, true),
             "Should not happen.");

  extension->extension_thread = self;

  TEN_UNUSED bool rc =
      ten_extension_store_add_extension(self->extension_store, extension, true);
  TEN_ASSERT(rc, "Should not happen.");
}

void ten_extension_thread_start_to_add_all_created_extension_to_engine(
    ten_extension_thread_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  ten_list_foreach (&self->extensions, iter) {
    ten_extension_t *extension = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(ten_extension_check_integrity(extension, true),
               "Should not happen.");

    // Correct the belonging_thread of the contained path_table.
    ten_sanitizer_thread_check_set_belonging_thread_to_current_thread(
        &extension->path_table->thread_check);

    // TODO(Wei): According to the provided extension_info_from_graph, we
    // should be able to fill the correct runtime graph relevant information
    // into the 'runtime_msg_dest_info'.
    ten_all_msg_type_dest_runtime_info_init(&extension->msg_dest_runtime_info);

    ten_extension_context_t *extension_context = self->extension_context;
    TEN_ASSERT(
        extension_context &&
            // TEN_NOLINTNEXTLINE(thread-check)
            // thread-check: We are in the extension thread, and throughout the
            // entire lifecycle of the extension, the extension_context where
            // the extension resides remains unchanged. Even in the closing
            // flow, the extension_context is closed later than the extension
            // itself. Therefore, using a pointer to the extension_context
            // within the extension thread is thread-safe.
            ten_extension_context_check_integrity(extension_context, false),
        "Should not happen.");

    // Setup 'extension_context' field, this is the most important field when
    // extension is initiating.
    extension->extension_context = extension_context;

    ten_extension_thread_add_extension(self, extension);
    ten_extension_link_its_ten_to_extension_context(extension,
                                                    extension_context);

    ten_engine_t *engine = extension_context->engine;
    // TEN_NOLINTNEXTLINE(thread-check)
    // thread-check: The runloop of the engine will not be changed during the
    // whole lifetime of the extension thread, so it's thread safe to access it
    // here.
    TEN_ASSERT(engine && ten_engine_check_integrity(engine, false),
               "Should not happen.");

    ten_runloop_post_task_tail(ten_engine_get_attached_runloop(engine),
                               ten_extension_context_add_extension,
                               extension_context, extension);
  }
}
