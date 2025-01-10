//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/extension_group/on_xxx.h"

#include "include_internal/axis_runtime/addon/addon.h"
#include "include_internal/axis_runtime/common/loc.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension_group/extension_group.h"
#include "include_internal/axis_runtime/extension_thread/extension_thread.h"
#include "include_internal/axis_runtime/extension_thread/msg_interface/common.h"
#include "include_internal/axis_runtime/extension_thread/on_xxx.h"
#include "include_internal/axis_runtime/metadata/metadata_info.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "axis_runtime/app/app.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_utils/container/list.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"

void axis_extension_group_on_init(axis_env_t *axis_env) {
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");
  axis_ASSERT(
      axis_env_get_attach_to(axis_env) == axis_ENV_ATTACH_TO_EXTENSION_GROUP,
      "Invalid argument.");

  axis_extension_group_t *self = axis_env_get_attached_extension_group(axis_env);
  axis_ASSERT(self && axis_extension_group_check_integrity(self, true),
             "Should not happen.");

  self->manifest_info =
      axis_metadata_info_create(axis_METADATA_ATTACH_TO_MANIFEST, self->axis_env);
  self->property_info =
      axis_metadata_info_create(axis_METADATA_ATTACH_TO_PROPERTY, self->axis_env);

  if (self->on_init) {
    self->on_init(self, self->axis_env);
  } else {
    axis_extension_group_on_init_done(self->axis_env);
  }
}

void axis_extension_group_on_deinit(axis_extension_group_t *self) {
  axis_ASSERT(self && axis_extension_group_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(self->axis_env && axis_env_check_integrity(self->axis_env, true),
             "Should not happen.");

  self->state = axis_EXTENSION_GROUP_STATE_DEINITING;

  if (self->on_deinit) {
    self->on_deinit(self, self->axis_env);
  } else {
    axis_extension_group_on_deinit_done(self->axis_env);
  }
}

void axis_extension_group_on_init_done(axis_env_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_env_check_integrity(self, true), "Invalid use of axis_env %p.",
             self);

  axis_extension_group_t *extension_group =
      axis_env_get_attached_extension_group(self);
  axis_ASSERT(extension_group &&
                 axis_extension_group_check_integrity(extension_group, true),
             "Should not happen.");

  axis_LOGD("[%s] on_init() done.",
           axis_extension_group_get_name(extension_group, true));

  axis_extension_thread_t *extension_thread = extension_group->extension_thread;
  axis_ASSERT(extension_thread &&
                 axis_extension_thread_check_integrity(extension_thread, true),
             "Should not happen.");

  axis_runloop_post_task_tail(
      axis_extension_group_get_attached_runloop(extension_group),
      axis_extension_thread_on_extension_group_on_init_done, extension_thread,
      NULL);
}

void axis_extension_group_on_deinit_done(axis_env_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_env_check_integrity(self, true), "Invalid use of axis_env %p.",
             self);

  axis_extension_group_t *extension_group =
      axis_env_get_attached_extension_group(self);
  axis_ASSERT(extension_group &&
                 axis_extension_group_check_integrity(extension_group, true),
             "Should not happen.");
  axis_ASSERT(extension_group->state >= axis_EXTENSION_GROUP_STATE_DEINITING,
             "Should not happen.");

  if (!axis_list_is_empty(&self->axis_proxy_list)) {
    // There is still the presence of axis_env_proxy, so the closing process
    // cannot continue.
    axis_LOGI(
        "[%s] Failed to on_deinit_done() because of existed axis_env_proxy.",
        axis_extension_group_get_name(extension_group, true));
    return;
  }

  if (extension_group->state == axis_EXTENSION_GROUP_STATE_DEINITTED) {
    return;
  }
  extension_group->state = axis_EXTENSION_GROUP_STATE_DEINITTED;

  axis_LOGD("[%s] on_deinit() done.",
           axis_extension_group_get_name(extension_group, true));

  axis_extension_thread_t *extension_thread = extension_group->extension_thread;
  axis_ASSERT(extension_thread &&
                 axis_extension_thread_check_integrity(extension_thread, true),
             "Should not happen.");

  // All extensions belong to this extension thread (group) are deleted, notify
  // this to the extension thread.
  axis_runloop_post_task_tail(
      axis_extension_group_get_attached_runloop(extension_group),
      axis_extension_thread_on_extension_group_on_deinit_done, extension_thread,
      NULL);
}

void axis_extension_group_on_create_extensions_done(axis_extension_group_t *self,
                                                   axis_list_t *extensions) {
  axis_ASSERT(self && axis_extension_group_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(self->extension_thread, "Should not happen.");

  axis_LOGD("[%s] create_extensions() done.",
           axis_string_get_raw_str(&self->name));

  axis_extension_thread_t *extension_thread = self->extension_thread;
  axis_ASSERT(extension_thread &&
                 axis_extension_thread_check_integrity(extension_thread, true),
             "Should not happen.");

  // Remove the extensions that were not successfully created from the list of
  // created extensions to determine the actual extensions for this extension
  // group/thread. Later, when this extension group/thread needs to shut down,
  // only these actual extensions need to be handled, ensuring correctness.
  axis_list_iterator_t iter = axis_list_begin(extensions);
  while (!axis_list_iterator_is_end(iter)) {
    axis_extension_t *extension = (axis_extension_t *)axis_ptr_listnode_get(
        axis_list_iterator_to_listnode(iter));

    axis_listnode_t *current_node = iter.node;
    iter = axis_list_iterator_next(iter);

    if (extension == axis_EXTENSION_UNSUCCESSFULLY_CREATED) {
      axis_list_remove_node(extensions, current_node);
    }
  }

  axis_list_swap(&extension_thread->extensions, extensions);

  axis_list_foreach (&extension_thread->extensions, iter) {
    axis_extension_t *extension = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(extension, "Invalid argument.");

    axis_extension_inherit_thread_ownership(extension, extension_thread);
    axis_ASSERT(axis_extension_check_integrity(extension, true),
               "Invalid use of extension %p.", extension);
  }

  axis_extension_thread_add_all_created_extensions(extension_thread);
}

void axis_extension_group_on_destroy_extensions_done(
    axis_extension_group_t *self) {
  axis_ASSERT(self && axis_extension_group_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(self->extension_thread, "Should not happen.");

  axis_LOGD("[%s] destroy_extensions() done.",
           axis_string_get_raw_str(&self->name));

  axis_extension_thread_t *extension_thread = self->extension_thread;
  axis_ASSERT(extension_thread &&
                 axis_extension_thread_check_integrity(extension_thread, true),
             "Should not happen.");

  axis_runloop_post_task_tail(axis_extension_group_get_attached_runloop(self),
                             axis_extension_thread_on_all_extensions_deleted,
                             extension_thread, NULL);
}

void axis_extension_group_on_addon_create_extension_done(
    axis_env_t *self, void *instance, axis_addon_context_t *addon_context) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_env_check_integrity(self, true), "Invalid use of axis_env %p.",
             self);

  axis_ASSERT(self->attach_to == axis_ENV_ATTACH_TO_EXTENSION_GROUP,
             "Should not happen.");

  axis_UNUSED axis_extension_group_t *extension_group =
      axis_env_get_attached_extension_group(self);
  axis_ASSERT(extension_group &&
                 axis_extension_group_check_integrity(extension_group, true),
             "Should not happen.");

  axis_extension_t *extension = instance;
  axis_ASSERT(extension && axis_extension_check_integrity(extension, true),
             "Should not happen.");

  axis_env_t *extension_axis_env = extension->axis_env;
  axis_ASSERT(
      extension_axis_env && axis_env_check_integrity(extension_axis_env, true),
      "Should not happen.");

  // This happens on the extension thread, so it's thread safe.

  if (addon_context->create_instance_done_cb) {
    addon_context->create_instance_done_cb(
        self, instance, addon_context->create_instance_done_cb_data);
  }

  if (addon_context) {
    axis_addon_context_destroy(addon_context);
  }
}

void axis_extension_group_on_addon_destroy_extension_done(
    axis_env_t *self, axis_addon_context_t *addon_context) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_env_check_integrity(self, true), "Invalid use of axis_env %p.",
             self);

  axis_ASSERT(self->attach_to == axis_ENV_ATTACH_TO_EXTENSION_GROUP,
             "Should not happen.");

  axis_UNUSED axis_extension_group_t *extension_group =
      axis_env_get_attached_extension_group(self);
  axis_ASSERT(extension_group &&
                 axis_extension_group_check_integrity(extension_group, true),
             "Should not happen.");

  if (addon_context->destroy_instance_done_cb) {
    addon_context->destroy_instance_done_cb(
        self, addon_context->destroy_instance_done_cb_data);
  }

  axis_addon_context_destroy(addon_context);
}

const char *axis_extension_group_get_name(axis_extension_group_t *self,
                                         bool check_thread) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_group_check_integrity(self, check_thread),
             "Invalid use of extension group %p.", self);

  return axis_string_get_raw_str(&self->name);
}
