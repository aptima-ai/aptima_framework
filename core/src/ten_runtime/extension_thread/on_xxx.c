//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/extension_thread/on_xxx.h"

#include <stdbool.h>

#include "include_internal/ten_runtime/addon/addon.h"
#include "include_internal/ten_runtime/common/loc.h"
#include "include_internal/ten_runtime/engine/engine.h"
#include "include_internal/ten_runtime/engine/on_xxx.h"
#include "include_internal/ten_runtime/extension/close.h"
#include "include_internal/ten_runtime/extension/extension.h"
#include "include_internal/ten_runtime/extension/metadata.h"
#include "include_internal/ten_runtime/extension/msg_handling.h"
#include "include_internal/ten_runtime/extension/on_xxx.h"
#include "include_internal/ten_runtime/extension/path_timer.h"
#include "include_internal/ten_runtime/extension_context/extension_context.h"
#include "include_internal/ten_runtime/extension_context/internal/del_extension.h"
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
#include "include_internal/ten_runtime/msg/cmd_base/cmd_base.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "include_internal/ten_runtime/path/path.h"
#include "include_internal/ten_runtime/path/path_table.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "ten_runtime/extension/extension.h"
#include "ten_runtime/msg/cmd_result/cmd_result.h"
#include "ten_utils/container/list.h"
#include "ten_utils/container/list_node_ptr.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/lib/smart_ptr.h"
#include "ten_utils/lib/string.h"
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

  // The extension system is about to be shut down, so do not proceed with
  // initialization anymore.
  if (self->is_close_triggered) {
    return;
  }

  ten_extension_group_t *extension_group = self->extension_group;
  TEN_ASSERT(extension_group &&
                 ten_extension_group_check_integrity(extension_group, true),
             "Should not happen.");

  ten_error_t err;
  ten_error_init(&err);

  bool rc = ten_handle_manifest_info_when_on_configure_done(
      &extension_group->manifest_info,
      ten_string_get_raw_str(ten_extension_group_get_base_dir(extension_group)),
      &extension_group->manifest, &err);
  if (!rc) {
    TEN_LOGW("Failed to load extension group manifest data, FATAL ERROR.");
    exit(EXIT_FAILURE);
  }

  rc = ten_handle_property_info_when_on_configure_done(
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

    ten_list_t cmd_result_list = TEN_LIST_INIT_VAL;
    ten_list_foreach (out_paths, iter) {
      ten_path_t *path = (ten_path_t *)ten_ptr_listnode_get(iter.node);
      TEN_ASSERT(path && ten_path_check_integrity(path, true),
                 "Should not happen.");

      ten_shared_ptr_t *cmd_result =
          ten_cmd_result_create(TEN_STATUS_CODE_ERROR);
      TEN_ASSERT(cmd_result && ten_cmd_base_check_integrity(cmd_result),
                 "Should not happen.");

      ten_msg_set_property(
          cmd_result, "detail",
          ten_value_create_string(ten_string_get_raw_str(&path->cmd_id)), NULL);
      ten_cmd_base_set_cmd_id(cmd_result,
                              ten_string_get_raw_str(&path->cmd_id));
      ten_list_push_smart_ptr_back(&cmd_result_list, cmd_result);
      ten_shared_ptr_destroy(cmd_result);
    }

    ten_list_foreach (&cmd_result_list, iter) {
      ten_shared_ptr_t *cmd_result = ten_smart_ptr_listnode_get(iter.node);
      TEN_ASSERT(cmd_result && ten_cmd_base_check_integrity(cmd_result),
                 "Should not happen.");

      ten_extension_handle_in_msg(extension, cmd_result);
    }

    ten_list_clear(&cmd_result_list);
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

  // Notify the engine that the extension thread is initted.
  ten_engine_t *engine = self->extension_context->engine;
  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: The runloop of the engine will not be changed during the
  // whole lifetime of the extension thread, so it's thread safe to access it
  // here.
  TEN_ASSERT(engine && ten_engine_check_integrity(engine, false),
             "Should not happen.");

  // TEN_NOLINTNEXTLINE(thread-check)
  ten_runloop_post_task_tail(ten_engine_get_attached_runloop(engine),
                             ten_engine_on_extension_thread_initted, engine,
                             self);

  if (self->is_close_triggered) {
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
