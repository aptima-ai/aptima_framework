//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "include_internal/ten_runtime/extension_thread/on_xxx.h"

#include <stdbool.h>

#include "include_internal/ten_runtime/addon/addon.h"
#include "include_internal/ten_runtime/common/loc.h"
#include "include_internal/ten_runtime/engine/engine.h"
#include "include_internal/ten_runtime/engine/on_xxx.h"
#include "include_internal/ten_runtime/extension/close.h"
#include "include_internal/ten_runtime/extension/extension.h"
#include "include_internal/ten_runtime/extension/extension_cb_default.h"
#include "include_internal/ten_runtime/extension/metadata.h"
#include "include_internal/ten_runtime/extension/msg_handling.h"
#include "include_internal/ten_runtime/extension/on_xxx.h"
#include "include_internal/ten_runtime/extension/path_timer.h"
#include "include_internal/ten_runtime/extension_context/extension_context.h"
#include "include_internal/ten_runtime/extension_context/internal/del_extension.h"
#include "include_internal/ten_runtime/extension_context/internal/extension_group_is_inited.h"
#include "include_internal/ten_runtime/extension_context/internal/extension_group_is_stopped.h"
#include "include_internal/ten_runtime/extension_context/internal/extension_thread_is_closing.h"
#include "include_internal/ten_runtime/extension_group/extension_group.h"
#include "include_internal/ten_runtime/extension_group/metadata.h"
#include "include_internal/ten_runtime/extension_group/on_xxx.h"
#include "include_internal/ten_runtime/extension_store/extension_store.h"
#include "include_internal/ten_runtime/extension_thread/extension_thread.h"
#include "include_internal/ten_runtime/extension_thread/msg_interface/common.h"
#include "include_internal/ten_runtime/metadata/metadata.h"
#include "include_internal/ten_runtime/metadata/metadata_info.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "include_internal/ten_runtime/path/path.h"
#include "include_internal/ten_runtime/path/path_table.h"
#include "include_internal/ten_runtime/schema_store/store.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "ten_runtime/extension/extension.h"
#include "ten_utils/container/list.h"
#include "ten_utils/container/list_node_ptr.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/log/log.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"
#include "ten_utils/sanitizer/thread_check.h"
#include "ten_utils/value/value.h"

void ten_extension_inherit_thread_ownership(
    ten_extension_t *self, ten_extension_thread_t *extension_thread) {
  TEN_ASSERT(self &&
                 // TEN_NOLINTNEXTLINE(thread-check)
                 // thread-check: The correct threading ownership will be setup
                 // soon, so we can _not_ check thread safety here.
                 ten_extension_check_integrity(self, false),
             "Should not happen.");

  TEN_ASSERT(extension_thread, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(extension_thread, true),
             "Invalid use of extension_thread %p.", extension_thread);

  // Move the ownership of the extension relevant resources to the current
  // thread.
  ten_sanitizer_thread_check_inherit_from(&self->thread_check,
                                          &extension_thread->thread_check);
  ten_sanitizer_thread_check_inherit_from(&self->ten_env->thread_check,
                                          &extension_thread->thread_check);
}

void ten_extension_thread_on_extension_added_to_engine(void *self_, void *arg) {
  ten_extension_thread_t *self = self_;
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  ten_extension_t *extension = arg;
  TEN_ASSERT(extension, "Invalid argument.");

  ten_extension_inherit_thread_ownership(extension, self);
  TEN_ASSERT(ten_extension_check_integrity(extension, true),
             "Invalid use of extension %p.", extension);

  self->extensions_cnt_of_added_to_engine++;
  if (self->extensions_cnt_of_added_to_engine ==
      ten_list_size(&self->extensions)) {
    TEN_LOGD(
        "[%s] All extensions of extension group has been added to engine, "
        "notify engine about this.",
        ten_string_get_raw_str(
            &extension->extension_thread->extension_group->name));

    ten_engine_t *engine = self->extension_context->engine;
    // TEN_NOLINTNEXTLINE(thread-check)
    // thread-check: The runloop of the engine will not be changed during the
    // whole lifetime of the extension thread, so it's thread safe to access it
    // here.
    TEN_ASSERT(engine && ten_engine_check_integrity(engine, false),
               "Should not happen.");

    // All Extensions are added to the engine, notify the engine this fact.
    ten_runloop_post_task_tail(ten_engine_get_attached_runloop(engine),
                               ten_engine_on_all_extensions_added, engine,
                               extension->extension_thread);
  }
}

void ten_extension_thread_on_extension_deleted_from_engine(void *self_,
                                                           void *arg) {
  ten_extension_thread_t *self = self_;
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  ten_extension_t *extension = arg;
  TEN_ASSERT(extension, "Invalid argument.");

  ten_extension_inherit_thread_ownership(extension, self);
  TEN_ASSERT(ten_extension_check_integrity(extension, true),
             "Invalid use of extension %p.", extension);

  TEN_LOGD("[%s] Deleted from extension thread (%s).",
           ten_extension_get_name(extension),
           ten_string_get_raw_str(&self->extension_group->name));

  // Delete the extension from the extension store of the extension thread, so
  // that no more messages could be routed to this extension in the future.
  ten_extension_store_del_extension(self->extension_store, extension, true);

  self->extensions_cnt_of_deleted_from_engine++;
  if (self->extensions_cnt_of_deleted_from_engine ==
      ten_list_size(&self->extensions)) {
    ten_extension_group_destroy_extensions(self->extension_group,
                                           self->extensions);
  }
}

void ten_extension_thread_on_extension_group_on_init_done(
    void *self_, TEN_UNUSED void *arg) {
  ten_extension_thread_t *self = (ten_extension_thread_t *)self_;
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  ten_extension_group_t *extension_group = self->extension_group;
  TEN_ASSERT(extension_group &&
                 ten_extension_group_check_integrity(extension_group, true),
             "Should not happen.");

  ten_error_t err;
  ten_error_init(&err);

  bool rc = ten_handle_manifest_info_when_on_init_done(
      &extension_group->manifest_info,
      ten_string_get_raw_str(ten_extension_group_get_base_dir(extension_group)),
      &extension_group->manifest, &err);
  if (!rc) {
    TEN_LOGW("Failed to load extension group manifest data, FATAL ERROR.");
    exit(EXIT_FAILURE);
  }

  rc = ten_handle_property_info_when_on_init_done(
      &extension_group->property_info,
      ten_string_get_raw_str(ten_extension_group_get_base_dir(extension_group)),
      &extension_group->property, &err);
  if (!rc) {
    TEN_LOGW("Failed to load extension group property data, FATAL ERROR.");
    exit(EXIT_FAILURE);
  }

  ten_error_deinit(&err);

  ten_extension_group_create_extensions(self->extension_group);
}

static void ten_extension_adjust_and_validate_property_on_init(
    ten_extension_t *self) {
  TEN_ASSERT(self && ten_extension_check_integrity(self, true),
             "Should not happen.");

  ten_error_t err;
  ten_error_init(&err);

  bool success = ten_schema_store_adjust_properties(&self->schema_store,
                                                    &self->property, &err);
  if (!success) {
    TEN_LOGW("[%s] Failed to adjust property type: %s.",
             ten_extension_get_name(self), ten_error_errmsg(&err));
    goto done;
  }

  success = ten_schema_store_validate_properties(&self->schema_store,
                                                 &self->property, &err);
  if (!success) {
    TEN_LOGW("[%s] Invalid property: %s.", ten_extension_get_name(self),
             ten_error_errmsg(&err));
    goto done;
  }

done:
  ten_error_deinit(&err);
  if (!success) {
    TEN_ASSERT(0, "Invalid property.");
  }
}

static bool ten_extension_parse_interface_schema(ten_extension_t *self,
                                                 ten_value_t *api_definition,
                                                 ten_error_t *err) {
  TEN_ASSERT(self && ten_extension_check_integrity(self, true),
             "Invalid argument.");
  TEN_ASSERT(api_definition && ten_value_check_integrity(api_definition),
             "Invalid argument.");

  bool result = ten_schema_store_set_interface_schema_definition(
      &self->schema_store, api_definition,
      ten_string_get_raw_str(&self->base_dir), err);
  if (!result) {
    TEN_LOGW("[%s] Failed to set interface schema definition: %s.",
             ten_extension_get_name(self), ten_error_errmsg(err));
  }

  return result;
}

void ten_extension_thread_on_extension_on_init_done(void *self_, void *arg) {
  ten_extension_thread_t *self = (ten_extension_thread_t *)self_;
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  ten_error_t err;
  ten_error_init(&err);

  ten_extension_on_init_done_t *on_init_done = arg;
  TEN_ASSERT(on_init_done, "Should not happen.");

  ten_extension_t *extension = on_init_done->extension;
  TEN_ASSERT(extension && ten_extension_check_integrity(extension, true),
             "Should not happen.");

  if (ten_extension_thread_get_state(self) >=
      TEN_EXTENSION_THREAD_STATE_PREPARE_TO_CLOSE) {
    goto done;
  }

  bool rc = ten_handle_manifest_info_when_on_init_done(
      &extension->manifest_info,
      ten_string_get_raw_str(ten_extension_get_base_dir(extension)),
      &extension->manifest, &err);
  if (!rc) {
    TEN_LOGW("Failed to load extension manifest data, FATAL ERROR.");
    exit(EXIT_FAILURE);
  }

  rc = ten_handle_property_info_when_on_init_done(
      &extension->property_info,
      ten_string_get_raw_str(ten_extension_get_base_dir(extension)),
      &extension->property, &err);
  if (!rc) {
    TEN_LOGW("Failed to load extension property data, FATAL ERROR.");
    exit(EXIT_FAILURE);
  }

  rc = ten_extension_resolve_properties_in_graph(extension, &err);
  TEN_ASSERT(rc, "Failed to resolve properties in graph.");

  ten_extension_merge_properties_from_graph(extension);

  rc = ten_extension_handle_ten_namespace_properties(
      extension, extension->extension_context);
  TEN_ASSERT(rc, "[%s] Failed to handle '_ten' properties.",
             ten_string_get_raw_str(&extension->name));

  ten_value_t *api_definition = ten_metadata_init_schema_store(
      &extension->manifest, &extension->schema_store);
  if (api_definition) {
    bool success =
        ten_extension_parse_interface_schema(extension, api_definition, &err);
    TEN_ASSERT(success, "Failed to parse interface schema.");
  }

  ten_extension_adjust_and_validate_property_on_init(extension);

  // Create timers for automatically cleaning expired IN_PATHs and OUT_PATHs.
  ten_timer_t *in_path_timer =
      ten_extension_create_timer_for_in_path(extension);
  ten_list_push_ptr_back(&extension->path_timers, in_path_timer, NULL);
  ten_timer_enable(in_path_timer);

  ten_timer_t *out_path_timer =
      ten_extension_create_timer_for_out_path(extension);
  ten_list_push_ptr_back(&extension->path_timers, out_path_timer, NULL);
  ten_timer_enable(out_path_timer);

  ten_extension_set_state(extension, TEN_EXTENSION_STATE_INITED);

  // The interface info has been resolved, and extensions might send msg out
  // during `on_start()`, so it's the best time to merge the interface info to
  // the extension_info.
  rc =
      ten_extension_determine_and_merge_all_interface_dest_extension(extension);
  TEN_ASSERT(rc, "Should not happen.");

  self->extensions_cnt_of_on_init_done++;

  if (self->extensions_cnt_of_on_init_done ==
      ten_list_size(&self->extensions)) {
    // All extensions in this extension group/thread have been inited.

    // Because the extension's on_init() may initialize some states of the
    // extension, we must wait until all extensions have completed their
    // 'on_init()' before they can start processing 'on_cmd()'.
    //
    // When the state of the extension thread is switched to
    // TEN_EXTENSION_THREAD_STATE_NORMAL, the messages will be pushed into the
    // extensions contained in the extension thread. Therefore, we can only
    // change the state of the extension thread to
    // TEN_EXTENSION_THREAD_STATE_NORMAL at this time.
    ten_extension_thread_set_state(self, TEN_EXTENSION_THREAD_STATE_NORMAL);

    ten_extension_context_t *extension_context = self->extension_context;
    TEN_ASSERT(extension_context, "Invalid argument.");
    // TEN_NOLINTNEXTLINE(thread-check)
    // thread-check: This function will be called in the extension thread,
    // however, the extension_context would not be changed after the extension
    // system is starting, so it's safe to access the extension_context
    // information in the extension thead.
    //
    // However, for the strict thread safety, it's possible to modify the
    // logic here to use asynchronous operations (i.e., add a task to the
    // extension_context, and add a task to the extension_thread when the
    // result is found) here.
    TEN_ASSERT(ten_extension_context_check_integrity(extension_context, false),
               "Invalid use of extension_context %p.", extension_context);

    ten_engine_t *engine = extension_context->engine;
    TEN_ASSERT(engine, "Invalid argument.");
    // TEN_NOLINTNEXTLINE(thread-check)
    // thread-check: The runloop of the engine will not be changed during the
    // whole lifetime of the extension thread, so it's thread safe to access
    // it here.
    TEN_ASSERT(ten_engine_check_integrity(engine, false),
               "Invalid use of engine %p.", engine);

    ten_runloop_post_task_tail(
        ten_engine_get_attached_runloop(engine),
        ten_extension_context_on_all_extensions_in_extension_group_are_inited,
        extension_context, self->extension_group);
  }

done:
  ten_error_deinit(&err);

  ten_extension_on_init_done_destroy(on_init_done);
}

void ten_extension_thread_call_all_extensions_on_start(void *self_,
                                                       TEN_UNUSED void *arg) {
  ten_extension_thread_t *self = (ten_extension_thread_t *)self_;
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  if (ten_extension_thread_get_state(self) >=
      TEN_EXTENSION_THREAD_STATE_PREPARE_TO_CLOSE) {
    // Already in the closing flow.
    return;
  }

  // Call on_start() of each containing extensions.
  ten_list_foreach (&self->extensions, iter) {
    ten_extension_t *extension = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(extension && ten_extension_check_integrity(extension, true),
               "Should not happen.");

    ten_extension_on_start(extension);
  }

  size_t pending_msgs_size = ten_list_size(&self->pending_msgs);
  if (pending_msgs_size) {
    // Flush the previously got messages, which are received before on_start(),
    // into the extension thread.
    TEN_LOGD("Flushing %zu pending msgs received before on_start().",
             pending_msgs_size);

    ten_list_foreach (&self->pending_msgs, iter) {
      ten_shared_ptr_t *msg = ten_smart_ptr_listnode_get(iter.node);
      TEN_ASSERT(msg, "Should not happen.");

      ten_extension_thread_handle_msg_async(self, msg);
    }
    ten_list_clear(&self->pending_msgs);
  }
}

void ten_extension_thread_on_extension_on_start_done(void *self_, void *arg) {
  ten_extension_thread_t *self = (ten_extension_thread_t *)self_;
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  ten_extension_on_start_stop_deinit_done_t *on_start_done = arg;
  TEN_ASSERT(on_start_done, "Should not happen.");

  if (ten_extension_thread_get_state(self) >=
      TEN_EXTENSION_THREAD_STATE_PREPARE_TO_CLOSE) {
    goto done;
  }

  ten_extension_t *extension = on_start_done->extension;
  TEN_ASSERT(extension && ten_extension_check_integrity(extension, true),
             "Should not happen.");

  self->extensions_cnt_of_on_start_done++;

  if (self->extensions_cnt_of_on_start_done ==
      ten_list_size(&self->extensions)) {
    ten_extension_thread_set_state(self,
                                   TEN_EXTENSION_THREAD_STATE_ALL_STARTED);
  }

  ten_extension_set_state(extension, TEN_EXTENSION_STATE_STARTED);

  size_t pending_msgs_size = ten_list_size(&extension->pending_msgs);
  if (pending_msgs_size) {
    // Flush the previously got messages, which are received before
    // on_start_done(), into the extension thread.
    TEN_LOGD("Flushing %zu pending msgs received before on_start_done().",
             pending_msgs_size);

    ten_list_foreach (&extension->pending_msgs, iter) {
      ten_shared_ptr_t *msg = ten_smart_ptr_listnode_get(iter.node);
      TEN_ASSERT(msg, "Should not happen.");

      ten_extension_handle_in_msg(extension, msg);
    }
    ten_list_clear(&extension->pending_msgs);
  }

done:
  ten_extension_on_start_stop_deinit_done_destroy(on_start_done);
}

static void ten_extension_thread_process_remaining_paths(
    ten_extension_t *extension) {
  TEN_ASSERT(extension && ten_extension_check_integrity(extension, true),
             "Should not happen.");

  ten_path_table_t *path_table = extension->path_table;
  TEN_ASSERT(path_table, "Should not happen.");

  ten_list_t *in_paths = &path_table->in_paths;
  TEN_ASSERT(in_paths && ten_list_check_integrity(in_paths),
             "Should not happen.");

  // Clear the _IN_ paths of the extension.
  ten_list_clear(in_paths);

  ten_list_t *out_paths = &path_table->out_paths;
  TEN_ASSERT(out_paths && ten_list_check_integrity(out_paths),
             "Should not happen.");

  size_t out_paths_cnt = ten_list_size(out_paths);
  if (out_paths_cnt) {
    // Call ten_extension_handle_in_msg to consume cmd results, so that the
    // _OUT_paths can be removed.
    TEN_LOGD("[%s] Flushing %zu remaining out paths.",
             ten_extension_get_name(extension), out_paths_cnt);

    ten_list_foreach (out_paths, iter) {
      ten_path_t *path = (ten_path_t *)ten_ptr_listnode_get(iter.node);
      TEN_ASSERT(path && ten_path_check_integrity(path, true),
                 "Should not happen.");

      ten_extension_terminate_out_path_prematurely(extension, path,
                                                   "The extension is stopped.");
    }
  }
}

void ten_extension_thread_on_extension_on_stop_done(void *self_, void *arg) {
  ten_extension_thread_t *self = (ten_extension_thread_t *)self_;
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  ten_extension_on_start_stop_deinit_done_t *on_stop_done = arg;
  TEN_ASSERT(on_stop_done, "Should not happen.");

  ten_extension_t *stopped_extension = on_stop_done->extension;
  TEN_ASSERT(stopped_extension &&
                 ten_extension_check_integrity(stopped_extension, true),
             "Should not happen.");
  TEN_ASSERT(stopped_extension->extension_thread == self, "Should not happen.");

  self->extensions_cnt_of_on_stop_done++;

  if (self->extensions_cnt_of_on_stop_done ==
      ten_list_size(&self->extensions)) {
    // All extensions in this extension group/thread have been stopped.

    ten_extension_context_t *extension_context = self->extension_context;
    TEN_ASSERT(extension_context, "Invalid argument.");
    // TEN_NOLINTNEXTLINE(thread-check)
    // thread-check: This function will be called in the extension thread,
    // however, the extension_context would not be changed after the extension
    // system is starting, so it's safe to access the extension_context
    // information in the extension thead.
    //
    // However, for the strict thread safety, it's possible to modify the logic
    // here to use asynchronous operations (i.e., add a task to the
    // extension_context, and add a task to the extension_thread when the result
    // is found) here.
    TEN_ASSERT(ten_extension_context_check_integrity(extension_context, false),
               "Invalid use of extension_context %p.", extension_context);

    ten_engine_t *engine = extension_context->engine;
    TEN_ASSERT(engine, "Invalid argument.");
    // TEN_NOLINTNEXTLINE(thread-check)
    // thread-check: The runloop of the engine will not be changed during the
    // whole lifetime of the extension thread, so it's thread safe to access it
    // here.
    TEN_ASSERT(ten_engine_check_integrity(engine, false),
               "Invalid use of engine %p.", engine);

    ten_runloop_post_task_tail(
        ten_engine_get_attached_runloop(engine),
        ten_extension_context_on_all_extensions_in_extension_group_are_stopped,
        extension_context, self->extension_group);
  }

  ten_extension_on_start_stop_deinit_done_destroy(on_stop_done);
}

void ten_extension_thread_pre_close(void *self_, TEN_UNUSED void *arg) {
  ten_extension_thread_t *self = (ten_extension_thread_t *)self_;
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  ten_list_foreach (&self->extensions, iter) {
    ten_extension_t *extension = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(extension && ten_extension_check_integrity(extension, true),
               "Should not happen.");

    ten_extension_do_pre_close_action(extension);
  }
}

void ten_extension_thread_on_extension_set_closing_flag(void *self_,
                                                        void *arg) {
  ten_extension_thread_t *self = (ten_extension_thread_t *)self_;
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  ten_extension_t *extension = (ten_extension_t *)arg;
  TEN_ASSERT(extension && ten_extension_check_integrity(extension, true),
             "Should not happen.");

  self->extensions_cnt_of_set_closing_flag++;

  if (self->extensions_cnt_of_set_closing_flag ==
      ten_list_size(&self->extensions)) {
    // Important: All the registered result handlers have to be called.
    //
    // Ex: If there are still some _IN_ or _OUT_ paths remaining in the path
    // table of extensions, in order to prevent memory leaks such as the
    // result handler in C++ binding, we need to create the corresponding
    // cmd results and send them into the original source extension.
    ten_list_foreach (&self->extensions, iter) {
      ten_extension_t *extension = ten_ptr_listnode_get(iter.node);
      TEN_ASSERT(extension && ten_extension_check_integrity(extension, true),
                 "Should not happen.");

      ten_extension_thread_process_remaining_paths(extension);
    }

    ten_extension_thread_set_state(self, TEN_EXTENSION_THREAD_STATE_CLOSING);

    // Even after this point in time, if other extension threads send messages
    // to this extension, because the state of this extension is already
    // CLOSING, the extension thread will not forward the messages to the
    // extensions it belongs to. Therefore, for those extensions, they can
    // safely begin the deinit and final destroy actions.

    ten_runloop_post_task_tail(
        ten_engine_get_attached_runloop(self->extension_context->engine),
        ten_extension_context_on_extension_thread_closing_flag_is_set,
        self->extension_context, self->extension_group);
  }
}

void ten_extension_thread_call_all_extensions_on_deinit(void *self_,
                                                        TEN_UNUSED void *arg) {
  ten_extension_thread_t *self = (ten_extension_thread_t *)self_;
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  TEN_ASSERT(ten_extension_thread_get_state(self) ==
                 TEN_EXTENSION_THREAD_STATE_CLOSING,
             "Extension thread is not closing: %d",
             ten_extension_thread_get_state(self));

  // Call on_deinit() of each containing extensions.
  ten_list_foreach (&self->extensions, iter) {
    ten_extension_t *extension = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(extension && ten_extension_check_integrity(extension, true),
               "Should not happen.");

    ten_extension_on_deinit(extension);
  }
}

void ten_extension_thread_on_extension_on_deinit_done(void *self_, void *arg) {
  ten_extension_thread_t *self = (ten_extension_thread_t *)self_;
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  ten_extension_on_start_stop_deinit_done_t *on_deinit_done = arg;
  TEN_ASSERT(on_deinit_done, "Should not happen.");

  ten_extension_t *deinit_extension = on_deinit_done->extension;
  TEN_ASSERT(
      deinit_extension && ten_extension_check_integrity(deinit_extension, true),
      "Should not happen.");
  TEN_ASSERT(deinit_extension->extension_thread == self, "Should not happen.");

  // Notify the 'ten' object of this extension that we are closing.
  TEN_ASSERT(deinit_extension->ten_env &&
                 ten_env_check_integrity(deinit_extension->ten_env, true),
             "Should not happen.");
  ten_env_close(deinit_extension->ten_env);

  ten_extension_context_t *extension_context = self->extension_context;
  TEN_ASSERT(extension_context, "Invalid argument.");
  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: This function will be called in the extension thread,
  // however, the extension_context would not be changed after the extension
  // system is starting, so it's safe to access the extension_context
  // information in the extension thead.
  //
  // However, for the strict thread safety, it's possible to modify the logic
  // here to use asynchronous operations (i.e., add a task to the
  // extension_context, and add a task to the extension_thread when the result
  // is found) here.
  TEN_ASSERT(ten_extension_context_check_integrity(extension_context, false),
             "Invalid use of extension_context %p.", extension_context);

  ten_engine_t *engine = extension_context->engine;
  TEN_ASSERT(engine, "Invalid argument.");
  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: The runloop of the engine will not be changed during the
  // whole lifetime of the extension thread, so it's thread safe to access it
  // here.
  TEN_ASSERT(ten_engine_check_integrity(engine, false),
             "Invalid use of engine %p.", engine);

  ten_runloop_post_task_tail(ten_engine_get_attached_runloop(engine),
                             ten_extension_context_delete_extension,
                             extension_context, deinit_extension);

  ten_extension_on_start_stop_deinit_done_destroy(on_deinit_done);
}

void ten_extension_thread_on_all_extensions_in_all_extension_threads_added_to_engine(
    void *self_, TEN_UNUSED void *arg) {
  ten_extension_thread_t *self = self_;
  TEN_ASSERT(ten_extension_thread_check_integrity(self, true),
             "Should not happen.");

  // The extension has just been created, the `on_init()` of the extension has
  // not been called yet. This function needs to be called before `on_init()` of
  // extensions, as the `extension::extension_info` field is used during the
  // `on_init()` stage, refer to `ten_extension_merge_properties_from_graph()`.
  // However, we can not parse `interface` info here, as the `interface_in` and
  // `interface_out` are defined in the manifest of extensions, which means that
  // the `interface` info is not available until `Extension::on_init_done()`.
  ten_extension_thread_determine_all_extension_dest_from_graph(self);

  // Notify the engine that the extension thread is inited.
  ten_engine_t *engine = self->extension_context->engine;
  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: The runloop of the engine will not be changed during the
  // whole lifetime of the extension thread, so it's thread safe to access it
  // here.
  TEN_ASSERT(engine && ten_engine_check_integrity(engine, false),
             "Should not happen.");

  // TEN_NOLINTNEXTLINE(thread-check)
  ten_runloop_post_task_tail(ten_engine_get_attached_runloop(engine),
                             ten_engine_on_extension_thread_inited, engine,
                             self);

  if (ten_extension_thread_get_state(self) >=
      TEN_EXTENSION_THREAD_STATE_PREPARE_TO_CLOSE) {
    return;
  }

  ten_list_foreach (&self->extensions, iter) {
    ten_extension_t *extension = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(extension && ten_extension_check_integrity(extension, true),
               "Should not happen.");

    ten_extension_load_metadata(extension);
  }
}

void ten_extension_thread_on_extension_group_on_deinit_done(
    void *self_, TEN_UNUSED void *arg) {
  ten_extension_thread_t *self = (ten_extension_thread_t *)self_;
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  ten_extension_group_t *extension_group = self->extension_group;
  TEN_ASSERT(extension_group &&
                 ten_extension_group_check_integrity(extension_group, true),
             "Should not happen.");

  // Notify the 'ten' object of this extension group that we are closing.
  ten_env_t *extension_group_ten_env = extension_group->ten_env;
  TEN_ASSERT(extension_group_ten_env &&
                 ten_env_check_integrity(extension_group_ten_env, true),
             "Should not happen.");
  ten_env_close(extension_group_ten_env);

  ten_runloop_stop(self->runloop);
}

void ten_extension_thread_on_all_extensions_deleted(void *self_,
                                                    TEN_UNUSED void *arg) {
  ten_extension_thread_t *self = (ten_extension_thread_t *)self_;
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  ten_list_clear(&self->extensions);

  ten_extension_group_t *extension_group = self->extension_group;
  TEN_ASSERT(extension_group &&
                 ten_extension_group_check_integrity(extension_group, true),
             "Should not happen.");

  ten_extension_group_on_deinit(extension_group);
}

void ten_extension_thread_on_all_extensions_created(void *self_, void *arg) {
  ten_extension_thread_t *self = (ten_extension_thread_t *)self_;
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  ten_list_t *extensions = (ten_list_t *)arg;
  ten_list_swap(&self->extensions, extensions);
  TEN_FREE(extensions);

  ten_list_foreach (&self->extensions, iter) {
    ten_extension_t *extension = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(extension, "Invalid argument.");

    ten_extension_inherit_thread_ownership(extension, self);
    TEN_ASSERT(ten_extension_check_integrity(extension, true),
               "Invalid use of extension %p.", extension);
  }

  ten_extension_thread_start_to_add_all_created_extension_to_engine(self);
}

void ten_extension_thread_on_addon_create_extension_done(void *self_,
                                                         void *arg) {
  ten_extension_thread_t *self = (ten_extension_thread_t *)self_;
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  ten_extension_group_t *extension_group = self->extension_group;
  TEN_ASSERT(extension_group &&
                 ten_extension_group_check_integrity(extension_group, true),
             "Should not happen.");

  ten_extension_thread_on_addon_create_extension_done_info_t *info = arg;
  TEN_ASSERT(arg, "Should not happen.");

  ten_extension_t *extension = info->extension;
  ten_extension_inherit_thread_ownership(extension, self);
  TEN_ASSERT(extension && ten_extension_check_integrity(extension, true),
             "Should not happen.");

  ten_extension_group_on_addon_create_extension_done(
      extension_group->ten_env, extension, info->addon_context);

  ten_extension_thread_on_addon_create_extension_done_info_destroy(info);
}

void ten_extension_thread_on_addon_destroy_extension_done(void *self_,
                                                          void *arg) {
  ten_extension_thread_t *self = (ten_extension_thread_t *)self_;
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  ten_extension_group_t *extension_group = self->extension_group;
  TEN_ASSERT(extension_group &&
                 ten_extension_group_check_integrity(extension_group, true),
             "Should not happen.");

  ten_addon_context_t *addon_context = arg;
  TEN_ASSERT(addon_context, "Should not happen.");

  ten_env_t *extension_group_ten = extension_group->ten_env;
  TEN_ASSERT(
      extension_group_ten && ten_env_check_integrity(extension_group_ten, true),
      "Should not happen.");

  // This happens on the extension thread, so it's thread safe.

  ten_extension_group_on_addon_destroy_extension_done(extension_group_ten,
                                                      addon_context);
}

void ten_extension_thread_create_addon_instance(void *self_, void *arg) {
  ten_extension_thread_t *self = (ten_extension_thread_t *)self_;
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  ten_addon_on_create_instance_info_t *addon_instance_info = arg;
  TEN_ASSERT(addon_instance_info, "Should not happen.");

  ten_addon_create_instance_async(
      self->extension_group->ten_env,
      ten_string_get_raw_str(&addon_instance_info->addon_name),
      ten_string_get_raw_str(&addon_instance_info->instance_name),
      addon_instance_info->addon_type, addon_instance_info->cb,
      addon_instance_info->cb_data);

  ten_addon_on_create_instance_info_destroy(addon_instance_info);
}

void ten_extension_thread_destroy_addon_instance(void *self_, void *arg) {
  ten_extension_thread_t *self = (ten_extension_thread_t *)self_;
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  ten_addon_on_destroy_instance_info_t *destroy_instance_info = arg;
  TEN_ASSERT(destroy_instance_info, "Should not happen.");

  ten_addon_host_destroy_instance_async(
      destroy_instance_info->addon_host, self->extension_group->ten_env,
      destroy_instance_info->instance, destroy_instance_info->cb,
      destroy_instance_info->cb_data);

  ten_addon_on_destroy_instance_info_destroy(destroy_instance_info);
}

ten_extension_thread_on_addon_create_extension_done_info_t *
ten_extension_thread_on_addon_create_extension_done_info_create(void) {
  ten_extension_thread_on_addon_create_extension_done_info_t *self = TEN_MALLOC(
      sizeof(ten_extension_thread_on_addon_create_extension_done_info_t));

  self->addon_context = NULL;
  self->extension = NULL;

  return self;
}

void ten_extension_thread_on_addon_create_extension_done_info_destroy(
    ten_extension_thread_on_addon_create_extension_done_info_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_FREE(self);
}
