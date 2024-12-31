//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/extension_thread/on_xxx.h"

#include <stdbool.h>

#include "include_internal/ten_runtime/addon/addon.h"
#include "include_internal/ten_runtime/common/loc.h"
#include "include_internal/ten_runtime/engine/on_xxx.h"
#include "include_internal/ten_runtime/extension/close.h"
#include "include_internal/ten_runtime/extension/extension.h"
#include "include_internal/ten_runtime/extension/metadata.h"
#include "include_internal/ten_runtime/extension/msg_handling.h"
#include "include_internal/ten_runtime/extension/path_timer.h"
#include "include_internal/ten_runtime/extension_context/extension_context.h"
#include "include_internal/ten_runtime/extension_group/base_dir.h"
#include "include_internal/ten_runtime/extension_group/extension_group.h"
#include "include_internal/ten_runtime/extension_group/metadata.h"
#include "include_internal/ten_runtime/extension_group/on_xxx.h"
#include "include_internal/ten_runtime/extension_store/extension_store.h"
#include "include_internal/ten_runtime/extension_thread/extension_thread.h"
#include "include_internal/ten_runtime/extension_thread/msg_interface/common.h"
#include "include_internal/ten_runtime/metadata/metadata.h"
#include "include_internal/ten_runtime/metadata/metadata_info.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "include_internal/ten_runtime/path/path_table.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "ten_runtime/extension/extension.h"
#include "ten_utils/container/list.h"
#include "ten_utils/container/list_node_ptr.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"
#include "ten_utils/sanitizer/thread_check.h"

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
      ten_extension_group_get_base_dir(extension_group),
      &extension_group->manifest, &err);
  if (!rc) {
    TEN_LOGW("Failed to load extension group manifest data, FATAL ERROR.");
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    exit(EXIT_FAILURE);
  }

  rc = ten_handle_property_info_when_on_configure_done(
      &extension_group->property_info,
      ten_extension_group_get_base_dir(extension_group),
      &extension_group->property, &err);
  if (!rc) {
    TEN_LOGW("Failed to load extension group property data, FATAL ERROR.");
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    exit(EXIT_FAILURE);
  }

  ten_error_deinit(&err);

  ten_extension_group_create_extensions(self->extension_group);
}

void ten_extension_thread_start_life_cycle_of_all_extensions_task(
    void *self_, TEN_UNUSED void *arg) {
  ten_extension_thread_t *self = self_;
  TEN_ASSERT(ten_extension_thread_check_integrity(self, true),
             "Should not happen.");

  if (self->is_close_triggered) {
    return;
  }

  ten_extension_thread_set_state(self, TEN_EXTENSION_THREAD_STATE_NORMAL);

  ten_list_foreach (&self->extensions, iter) {
    ten_extension_t *extension = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(extension && ten_extension_check_integrity(extension, true),
               "Should not happen.");

    ten_extension_load_metadata(extension);
  }
}

void ten_extension_thread_stop_life_cycle_of_all_extensions(
    ten_extension_thread_t *self) {
  TEN_ASSERT(self && ten_extension_thread_check_integrity(self, true),
             "Invalid argument.");

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

void ten_extension_thread_stop_life_cycle_of_all_extensions_task(
    void *self, TEN_UNUSED void *arg) {
  ten_extension_thread_t *extension_thread = self;
  TEN_ASSERT(extension_thread &&
                 ten_extension_thread_check_integrity(extension_thread, true),
             "Invalid argument.");

  ten_extension_thread_stop_life_cycle_of_all_extensions(extension_thread);
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

void ten_extension_thread_create_extension_instance(void *self_, void *arg) {
  ten_extension_thread_t *self = (ten_extension_thread_t *)self_;
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  ten_addon_on_create_extension_instance_info_t *addon_instance_info = arg;
  TEN_ASSERT(addon_instance_info, "Should not happen.");

  ten_addon_create_instance_async(
      self->extension_group->ten_env, addon_instance_info->addon_type,
      ten_string_get_raw_str(&addon_instance_info->addon_name),
      ten_string_get_raw_str(&addon_instance_info->instance_name),
      addon_instance_info->cb, addon_instance_info->cb_data);

  ten_addon_on_create_extension_instance_info_destroy(addon_instance_info);
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
