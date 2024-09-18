//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/engine/on_xxx.h"

#include "include_internal/ten_runtime/addon/addon.h"
#include "include_internal/ten_runtime/engine/engine.h"
#include "include_internal/ten_runtime/engine/msg_interface/common.h"
#include "include_internal/ten_runtime/extension/extension.h"
#include "include_internal/ten_runtime/extension_context/extension_context.h"
#include "include_internal/ten_runtime/extension_context/ten_env/on_xxx.h"
#include "include_internal/ten_runtime/extension_group/extension_group.h"
#include "include_internal/ten_runtime/extension_thread/extension_thread.h"
#include "include_internal/ten_runtime/extension_thread/on_xxx.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "include_internal/ten_utils/log/log.h"
#include "ten_runtime/msg/cmd_result/cmd_result.h"
#include "ten_runtime/ten_env/ten_env.h"
#include "ten_utils/container/list.h"
#include "ten_utils/lib/smart_ptr.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"
#include "ten_utils/sanitizer/thread_check.h"

void ten_engine_on_all_extensions_added(void *self_, void *arg) {
  ten_engine_t *self = self_;
  TEN_ASSERT(self && ten_engine_check_integrity(self, true),
             "Should not happen.");

  TEN_UNUSED ten_extension_thread_t *extension_thread = arg;
  TEN_ASSERT(
      extension_thread &&
          // TEN_NOLINTNEXTLINE(thread-check)
          // thread-check: this function does not access this extension_thread,
          // we just check if the arg is an ten_extension_thread_t.
          ten_extension_thread_check_integrity(extension_thread, false),
      "Should not happen.");

  self->extension_context
      ->extension_threads_cnt_of_all_extensions_added_to_engine++;
  if (self->extension_context
          ->extension_threads_cnt_of_all_extensions_added_to_engine ==
      ten_list_size(&self->extension_context->extension_threads)) {
    TEN_LOGD("[%s] All extension threads has added all extensions to engine.",
             ten_engine_get_name(self));

    ten_list_foreach (&self->extension_context->extension_threads, iter) {
      ten_extension_thread_t *extension_thread =
          ten_ptr_listnode_get(iter.node);
      TEN_ASSERT(extension_thread && ten_extension_thread_check_integrity(
                                         extension_thread, false),
                 "Should not happen.");

      ten_runloop_post_task_tail(
          ten_extension_thread_get_attached_runloop(extension_thread),
          ten_extension_thread_on_all_extensions_in_all_extension_threads_added_to_engine,
          extension_thread, NULL);
    }
  }
}

void ten_engine_on_extension_thread_closed(void *self_, void *arg) {
  ten_engine_t *self = self_;
  TEN_ASSERT(self && ten_engine_check_integrity(self, true),
             "Should not happen.");

  ten_extension_thread_t *extension_thread = arg;
  TEN_ASSERT(
      extension_thread &&
          // TEN_NOLINTNEXTLINE(thread-check)
          // thread-check: this function does not access this extension_thread,
          // we just check if the arg is an ten_extension_thread_t.
          ten_extension_thread_check_integrity(extension_thread, false),
      "Should not happen.");

  TEN_LOGD("[%s] Waiting for extension thread (%p) be reclaimed.",
           ten_engine_get_name(self), extension_thread);
  TEN_UNUSED int rc =
      ten_thread_join(ten_sanitizer_thread_check_get_belonging_thread(
                          &extension_thread->thread_check),
                      -1);
  TEN_ASSERT(!rc, "Should not happen.");
  TEN_LOGD("[%s] Extension thread (%p) is reclaimed.",
           ten_engine_get_name(self), extension_thread);

  // Extension thread is disappear, so we migrate the extension_group and
  // extension_thread to the engine thread now.
  ten_sanitizer_thread_check_inherit_from(&extension_thread->thread_check,
                                          &self->thread_check);
  ten_sanitizer_thread_check_inherit_from(
      &extension_thread->extension_group->thread_check, &self->thread_check);
  ten_sanitizer_thread_check_inherit_from(
      &extension_thread->extension_group->ten_env->thread_check,
      &self->thread_check);

  self->extension_context->extension_threads_cnt_of_closed++;

  ten_extension_context_on_close(self->extension_context);
}

void ten_engine_on_extension_thread_initted(void *self_, void *arg) {
  ten_engine_t *self = self_;
  TEN_ASSERT(self && ten_engine_check_integrity(self, true),
             "Should not happen.");

  ten_extension_thread_t *extension_thread = arg;
  TEN_ASSERT(
      extension_thread &&
          // TEN_NOLINTNEXTLINE(thread-check)
          // thread-check: this function does not access this extension_thread,
          // we just check if the arg is an ten_extension_thread_t.
          ten_extension_thread_check_integrity(extension_thread, false),
      "Should not happen.");

  self->extension_context->extension_threads_cnt_of_initted++;
  if (self->extension_context->extension_threads_cnt_of_initted ==
      ten_list_size(&self->extension_context->extension_threads)) {
    TEN_LOGD("[%s] All extension threads are initted.",
             ten_engine_get_name(self));

    ten_list_foreach (&self->extension_context->extension_threads, iter) {
      ten_extension_thread_t *extension_thread =
          ten_ptr_listnode_get(iter.node);
      TEN_ASSERT(extension_thread && ten_extension_thread_check_integrity(
                                         extension_thread, false),
                 "Should not happen.");
    }

    // All the extension threads requested by this command have been completed,
    // return the result for this command.
    //
    // We cannot notify the engine that this thread is started completely
    // _before_ this line. Because the codes of previous states would modify
    // some data structures (i.e., to add the extension information to the
    // extension store, etc.), and when the main (I/O) thread (ex: the app
    // thread or the engine thread) receives messages from remote apps, the main
    // thread would 'search/read' those data structures to find the correct
    // extension to dispatch those received messages.
    //
    // After notifying the engine, the engine would send a 'OK' status back
    // to the previous graph stage, and finally notifying the client that
    // the whole graph is built-up successfully, so that the client will
    // start to send commands into the graph.
    //
    // So if we notify the engine too early (before this line), that would
    // cause a very seldom timing issue that when the extension thread is
    // still modifying those data structures, and the main (I/O) thread (ex: the
    // app thread or the engine thread) is started to receive messages, and the
    // main (I/O) thread will fail to find the correct destination extension
    // (Because the extension doesn't register itself to the extension store).

    ten_string_t *graph_name = &self->graph_name;

    const char *body_str = ten_string_is_empty(graph_name)
                               ? ""
                               : ten_string_get_raw_str(graph_name);

    ten_shared_ptr_t *state_requester_cmd =
        extension_thread->extension_context->state_requester_cmd;

    ten_shared_ptr_t *returned_cmd =
        ten_cmd_result_create_from_cmd(TEN_STATUS_CODE_OK, state_requester_cmd);
    ten_msg_set_property(returned_cmd, "detail",
                         ten_value_create_string(body_str), NULL);

    // We have sent the result for the original state_requester_cmd, so it is
    // useless now, destroy it.
    ten_shared_ptr_destroy(state_requester_cmd);
    extension_thread->extension_context->state_requester_cmd = NULL;

#if defined(_DEBUG)
    ten_msg_dump(
        returned_cmd, NULL,
        "Return extension-system-initted-result to previous stage: ^m");
#endif

    ten_engine_dispatch_msg(self, returned_cmd);

    ten_shared_ptr_destroy(returned_cmd);

    // Mark the engine that it could start to handle messages.
    self->is_ready_to_handle_msg = true;

    TEN_LOGD("[%s] Engine is ready to handle messages.",
             ten_engine_get_name(self));

    // Because the engine is just ready to handle messages, hence, we trigger
    // the engine to handle any external messages if any.
    ten_engine_handle_in_msgs_async(self);
  }
}

void ten_engine_on_addon_create_extension_group_done(void *self_, void *arg) {
  ten_engine_t *self = self_;
  TEN_ASSERT(self && ten_engine_check_integrity(self, true),
             "Should not happen.");

  ten_extension_context_t *extension_context = self->extension_context;
  TEN_ASSERT(extension_context, "Invalid argument.");
  TEN_ASSERT(ten_extension_context_check_integrity(extension_context, true),
             "Invalid use of extension_context %p.", extension_context);

  ten_extension_context_on_addon_create_extension_group_done_info_t *info = arg;
  TEN_ASSERT(info, "Should not happen.");

  ten_extension_group_t *extension_group = info->extension_group;
  TEN_ASSERT(extension_group &&
                 // TEN_NOLINTNEXTLINE(thread-check)
                 // thread-check: The extension thread has not been created yet,
                 // so it is thread safe
                 ten_extension_group_check_integrity(extension_group, false),
             "Should not happen.");

  ten_extension_context_on_addon_create_extension_group_done(
      extension_context->ten_env, extension_group, info->addon_context);

  ten_extension_context_on_addon_create_extension_group_done_info_destroy(info);
}

void ten_engine_on_addon_destroy_extension_group_done(void *self_, void *arg) {
  ten_engine_t *self = self_;
  TEN_ASSERT(self && ten_engine_check_integrity(self, true),
             "Should not happen.");

  ten_extension_context_t *extension_context = self->extension_context;
  TEN_ASSERT(extension_context, "Invalid argument.");
  TEN_ASSERT(ten_extension_context_check_integrity(extension_context, true),
             "Invalid use of extension_context %p.", extension_context);

  ten_addon_context_t *addon_context = arg;
  TEN_ASSERT(addon_context, "Should not happen.");

  // This happens on the engine thread, so it's thread safe.

  ten_extension_context_on_addon_destroy_extension_group_done(
      extension_context->ten_env, addon_context);
}
