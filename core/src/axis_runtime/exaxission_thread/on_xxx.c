//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/extension_thread/on_xxx.h"

#include <stdbool.h>

#include "include_internal/axis_runtime/addon/addon.h"
#include "include_internal/axis_runtime/addon/addon_host.h"
#include "include_internal/axis_runtime/addon/extension/extension.h"
#include "include_internal/axis_runtime/common/loc.h"
#include "include_internal/axis_runtime/engine/on_xxx.h"
#include "include_internal/axis_runtime/extension/close.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension/metadata.h"
#include "include_internal/axis_runtime/extension/msg_handling.h"
#include "include_internal/axis_runtime/extension/path_timer.h"
#include "include_internal/axis_runtime/extension_context/extension_context.h"
#include "include_internal/axis_runtime/extension_group/base_dir.h"
#include "include_internal/axis_runtime/extension_group/extension_group.h"
#include "include_internal/axis_runtime/extension_group/metadata.h"
#include "include_internal/axis_runtime/extension_group/on_xxx.h"
#include "include_internal/axis_runtime/extension_store/extension_store.h"
#include "include_internal/axis_runtime/extension_thread/extension_thread.h"
#include "include_internal/axis_runtime/extension_thread/msg_interface/common.h"
#include "include_internal/axis_runtime/metadata/metadata.h"
#include "include_internal/axis_runtime/metadata/metadata_info.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/path/path_table.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "axis_runtime/extension/extension.h"
#include "axis_utils/container/list.h"
#include "axis_utils/container/list_node_ptr.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"
#include "axis_utils/sanitizer/thread_check.h"

void axis_extension_inherit_thread_ownership(
    axis_extension_t *self, axis_extension_thread_t *extension_thread) {
  axis_ASSERT(self &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: The correct threading ownership will be setup
                 // soon, so we can _not_ check thread safety here.
                 axis_extension_check_integrity(self, false),
             "Should not happen.");

  axis_ASSERT(extension_thread, "Invalid argument.");
  axis_ASSERT(axis_extension_thread_check_integrity(extension_thread, true),
             "Invalid use of extension_thread %p.", extension_thread);

  // Move the ownership of the extension relevant resources to the current
  // thread.
  axis_sanitizer_thread_check_inherit_from(&self->thread_check,
                                          &extension_thread->thread_check);
  axis_sanitizer_thread_check_inherit_from(&self->axis_env->thread_check,
                                          &extension_thread->thread_check);
}

void axis_extension_thread_on_extension_group_on_init_done(
    void *self_, axis_UNUSED void *arg) {
  axis_extension_thread_t *self = (axis_extension_thread_t *)self_;
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  // The extension system is about to be shut down, so do not proceed with
  // initialization anymore.
  if (self->is_close_triggered) {
    return;
  }

  axis_extension_group_t *extension_group = self->extension_group;
  axis_ASSERT(extension_group &&
                 axis_extension_group_check_integrity(extension_group, true),
             "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  bool rc = axis_handle_manifest_info_when_on_configure_done(
      &extension_group->manifest_info,
      axis_extension_group_get_base_dir(extension_group),
      &extension_group->manifest, &err);
  if (!rc) {
    axis_LOGW("Failed to load extension group manifest data, FATAL ERROR.");
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    exit(EXIT_FAILURE);
  }

  rc = axis_handle_property_info_when_on_configure_done(
      &extension_group->property_info,
      axis_extension_group_get_base_dir(extension_group),
      &extension_group->property, &err);
  if (!rc) {
    axis_LOGW("Failed to load extension group property data, FATAL ERROR.");
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    exit(EXIT_FAILURE);
  }

  axis_error_deinit(&err);

  axis_extension_group_create_extensions(self->extension_group);
}

void axis_extension_thread_stop_life_cycle_of_all_extensions(
    axis_extension_thread_t *self) {
  axis_ASSERT(self && axis_extension_thread_check_integrity(self, true),
             "Invalid argument.");

  axis_extension_thread_set_state(self,
                                 axis_EXTENSION_THREAD_STATE_PREPARE_TO_CLOSE);

  if (axis_list_is_empty(&self->extensions)) {
    // This extension group does not contain any extensions, so it can directly
    // proceed to the deinitialization phase of the extension group.
    axis_extension_group_on_deinit(self->extension_group);
  } else {
    // Loop for all the containing extensions, and call their on_stop().
    axis_list_foreach (&self->extensions, iter) {
      axis_extension_t *extension = axis_ptr_listnode_get(iter.node);
      axis_ASSERT(axis_extension_check_integrity(extension, true),
                 "Should not happen.");

      axis_extension_on_stop(extension);
    }
  }
}

void axis_extension_thread_on_extension_group_on_deinit_done(
    void *self_, axis_UNUSED void *arg) {
  axis_extension_thread_t *self = (axis_extension_thread_t *)self_;
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  axis_extension_group_t *extension_group = self->extension_group;
  axis_ASSERT(extension_group &&
                 axis_extension_group_check_integrity(extension_group, true),
             "Should not happen.");

  // Notify the 'ten' object of this extension group that we are closing.
  axis_env_t *extension_group_axis_env = extension_group->axis_env;
  axis_ASSERT(extension_group_axis_env &&
                 axis_env_check_integrity(extension_group_axis_env, true),
             "Should not happen.");
  axis_env_close(extension_group_axis_env);

  axis_runloop_stop(self->runloop);
}

void axis_extension_thread_on_all_extensions_deleted(void *self_,
                                                    axis_UNUSED void *arg) {
  axis_extension_thread_t *self = (axis_extension_thread_t *)self_;
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  axis_list_clear(&self->extensions);

  axis_extension_group_t *extension_group = self->extension_group;
  axis_ASSERT(extension_group &&
                 axis_extension_group_check_integrity(extension_group, true),
             "Should not happen.");

  axis_extension_group_on_deinit(extension_group);
}

void axis_extension_thread_on_addon_create_extension_done(void *self_,
                                                         void *arg) {
  axis_extension_thread_t *self = (axis_extension_thread_t *)self_;
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  axis_extension_group_t *extension_group = self->extension_group;
  axis_ASSERT(extension_group &&
                 axis_extension_group_check_integrity(extension_group, true),
             "Should not happen.");

  axis_extension_thread_on_addon_create_extension_done_ctx_t *ctx = arg;
  axis_ASSERT(arg, "Should not happen.");

  axis_extension_t *extension = ctx->extension;
  axis_extension_inherit_thread_ownership(extension, self);
  axis_ASSERT(extension && axis_extension_check_integrity(extension, true),
             "Should not happen.");

  axis_extension_group_on_addon_create_extension_done(
      extension_group->axis_env, extension, ctx->addon_context);

  axis_extension_thread_on_addon_create_extension_done_ctx_destroy(ctx);
}

void axis_extension_thread_on_addon_destroy_extension_done(void *self_,
                                                          void *arg) {
  axis_extension_thread_t *self = (axis_extension_thread_t *)self_;
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  axis_extension_group_t *extension_group = self->extension_group;
  axis_ASSERT(extension_group &&
                 axis_extension_group_check_integrity(extension_group, true),
             "Should not happen.");

  axis_addon_context_t *addon_context = arg;
  axis_ASSERT(addon_context, "Should not happen.");

  axis_env_t *extension_group_ten = extension_group->axis_env;
  axis_ASSERT(
      extension_group_ten && axis_env_check_integrity(extension_group_ten, true),
      "Should not happen.");

  // This happens on the extension thread, so it's thread safe.

  axis_extension_group_on_addon_destroy_extension_done(extension_group_ten,
                                                      addon_context);
}

void axis_extension_thread_create_extension_instance(void *self_, void *arg) {
  axis_extension_thread_t *self = (axis_extension_thread_t *)self_;
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  axis_addon_on_create_extension_instance_ctx_t *addon_instance_info = arg;
  axis_ASSERT(addon_instance_info, "Should not happen.");

  axis_addon_create_instance_async(
      self->extension_group->axis_env, addon_instance_info->addon_type,
      axis_string_get_raw_str(&addon_instance_info->addon_name),
      axis_string_get_raw_str(&addon_instance_info->instance_name),
      addon_instance_info->cb, addon_instance_info->cb_data);

  axis_addon_on_create_extension_instance_ctx_destroy(addon_instance_info);
}

void axis_extension_thread_destroy_addon_instance(void *self_, void *arg) {
  axis_extension_thread_t *self = (axis_extension_thread_t *)self_;
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_thread_check_integrity(self, true),
             "Invalid use of extension_thread %p.", self);

  axis_addon_host_on_destroy_instance_ctx_t *destroy_instance_info = arg;
  axis_ASSERT(destroy_instance_info, "Should not happen.");

  axis_addon_host_destroy_instance_async(
      destroy_instance_info->addon_host, self->extension_group->axis_env,
      destroy_instance_info->instance, destroy_instance_info->cb,
      destroy_instance_info->cb_data);

  axis_addon_host_on_destroy_instance_ctx_destroy(destroy_instance_info);
}

axis_extension_thread_on_addon_create_extension_done_ctx_t *
axis_extension_thread_on_addon_create_extension_done_ctx_create(void) {
  axis_extension_thread_on_addon_create_extension_done_ctx_t *self = axis_MALLOC(
      sizeof(axis_extension_thread_on_addon_create_extension_done_ctx_t));

  self->addon_context = NULL;
  self->extension = NULL;

  return self;
}

void axis_extension_thread_on_addon_create_extension_done_ctx_destroy(
    axis_extension_thread_on_addon_create_extension_done_ctx_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_FREE(self);
}
