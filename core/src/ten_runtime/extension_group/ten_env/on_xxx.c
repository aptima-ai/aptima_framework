//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/extension_group/on_xxx.h"

#include "include_internal/ten_runtime/addon/addon.h"
#include "include_internal/ten_runtime/common/loc.h"
#include "include_internal/ten_runtime/extension/extension.h"
#include "include_internal/ten_runtime/extension_group/extension_group.h"
#include "include_internal/ten_runtime/extension_thread/extension_thread.h"
#include "include_internal/ten_runtime/extension_thread/msg_interface/common.h"
#include "include_internal/ten_runtime/extension_thread/on_xxx.h"
#include "include_internal/ten_runtime/metadata/metadata_info.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "ten_runtime/app/app.h"
#include "ten_runtime/ten_env/ten_env.h"
#include "ten_utils/container/list.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"

void ten_extension_group_on_init(ten_env_t *ten_env) {
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");
  TEN_ASSERT(
      ten_env_get_attach_to(ten_env) == TEN_ENV_ATTACH_TO_EXTENSION_GROUP,
      "Invalid argument.");

  ten_extension_group_t *self = ten_env_get_attached_extension_group(ten_env);
  TEN_ASSERT(self && ten_extension_group_check_integrity(self, true),
             "Should not happen.");

  self->manifest_info =
      ten_metadata_info_create(TEN_METADATA_ATTACH_TO_MANIFEST, self->ten_env);
  self->property_info =
      ten_metadata_info_create(TEN_METADATA_ATTACH_TO_PROPERTY, self->ten_env);

  if (self->on_init) {
    self->on_init(self, self->ten_env);
  } else {
    ten_extension_group_on_init_done(self->ten_env);
  }
}

void ten_extension_group_on_deinit(ten_extension_group_t *self) {
  TEN_ASSERT(self && ten_extension_group_check_integrity(self, true),
             "Should not happen.");
  TEN_ASSERT(self->ten_env && ten_env_check_integrity(self->ten_env, true),
             "Should not happen.");

  self->state = TEN_EXTENSION_GROUP_STATE_DEINITING;

  if (self->on_deinit) {
    self->on_deinit(self, self->ten_env);
  } else {
    ten_extension_group_on_deinit_done(self->ten_env);
  }
}

void ten_extension_group_on_init_done(ten_env_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_env_check_integrity(self, true), "Invalid use of ten_env %p.",
             self);

  ten_extension_group_t *extension_group =
      ten_env_get_attached_extension_group(self);
  TEN_ASSERT(extension_group &&
                 ten_extension_group_check_integrity(extension_group, true),
             "Should not happen.");

  TEN_LOGD("[%s] on_init() done.",
           ten_extension_group_get_name(extension_group, true));

  ten_extension_thread_t *extension_thread = extension_group->extension_thread;
  TEN_ASSERT(extension_thread &&
                 ten_extension_thread_check_integrity(extension_thread, true),
             "Should not happen.");

  ten_runloop_post_task_tail(
      ten_extension_group_get_attached_runloop(extension_group),
      ten_extension_thread_on_extension_group_on_init_done, extension_thread,
      NULL);
}

void ten_extension_group_on_deinit_done(ten_env_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_env_check_integrity(self, true), "Invalid use of ten_env %p.",
             self);

  ten_extension_group_t *extension_group =
      ten_env_get_attached_extension_group(self);
  TEN_ASSERT(extension_group &&
                 ten_extension_group_check_integrity(extension_group, true),
             "Should not happen.");
  TEN_ASSERT(extension_group->state >= TEN_EXTENSION_GROUP_STATE_DEINITING,
             "Should not happen.");

  if (!ten_list_is_empty(&self->ten_proxy_list)) {
    // There is still the presence of ten_env_proxy, so the closing process
    // cannot continue.
    TEN_LOGI(
        "[%s] Failed to on_deinit_done() because of existed ten_env_proxy.",
        ten_extension_group_get_name(extension_group, true));
    return;
  }

  if (extension_group->state == TEN_EXTENSION_GROUP_STATE_DEINITTED) {
    return;
  }
  extension_group->state = TEN_EXTENSION_GROUP_STATE_DEINITTED;

  TEN_LOGD("[%s] on_deinit() done.",
           ten_extension_group_get_name(extension_group, true));

  ten_extension_thread_t *extension_thread = extension_group->extension_thread;
  TEN_ASSERT(extension_thread &&
                 ten_extension_thread_check_integrity(extension_thread, true),
             "Should not happen.");

  // All extensions belong to this extension thread (group) are deleted, notify
  // this to the extension thread.
  ten_runloop_post_task_tail(
      ten_extension_group_get_attached_runloop(extension_group),
      ten_extension_thread_on_extension_group_on_deinit_done, extension_thread,
      NULL);
}

void ten_extension_group_on_create_extensions_done(ten_extension_group_t *self,
                                                   ten_list_t *extensions) {
  TEN_ASSERT(self && ten_extension_group_check_integrity(self, true),
             "Should not happen.");
  TEN_ASSERT(self->extension_thread, "Should not happen.");

  TEN_LOGD("[%s] create_extensions() done.",
           ten_string_get_raw_str(&self->name));

  ten_extension_thread_t *extension_thread = self->extension_thread;
  TEN_ASSERT(extension_thread &&
                 ten_extension_thread_check_integrity(extension_thread, true),
             "Should not happen.");

  ten_list_swap(&extension_thread->extensions, extensions);

  ten_list_foreach (&extension_thread->extensions, iter) {
    ten_extension_t *extension = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(extension, "Invalid argument.");

    ten_extension_inherit_thread_ownership(extension, extension_thread);
    TEN_ASSERT(ten_extension_check_integrity(extension, true),
               "Invalid use of extension %p.", extension);
  }

  ten_extension_thread_add_all_created_extensions(extension_thread);
}

void ten_extension_group_on_destroy_extensions_done(
    ten_extension_group_t *self) {
  TEN_ASSERT(self && ten_extension_group_check_integrity(self, true),
             "Should not happen.");
  TEN_ASSERT(self->extension_thread, "Should not happen.");

  TEN_LOGD("[%s] destroy_extensions() done.",
           ten_string_get_raw_str(&self->name));

  ten_extension_thread_t *extension_thread = self->extension_thread;
  TEN_ASSERT(extension_thread &&
                 ten_extension_thread_check_integrity(extension_thread, true),
             "Should not happen.");

  ten_runloop_post_task_tail(ten_extension_group_get_attached_runloop(self),
                             ten_extension_thread_on_all_extensions_deleted,
                             extension_thread, NULL);
}

void ten_extension_group_on_addon_create_extension_done(
    ten_env_t *self, void *instance, ten_addon_context_t *addon_context) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_env_check_integrity(self, true), "Invalid use of ten_env %p.",
             self);

  TEN_ASSERT(self->attach_to == TEN_ENV_ATTACH_TO_EXTENSION_GROUP,
             "Should not happen.");

  TEN_UNUSED ten_extension_group_t *extension_group =
      ten_env_get_attached_extension_group(self);
  TEN_ASSERT(extension_group &&
                 ten_extension_group_check_integrity(extension_group, true),
             "Should not happen.");

  ten_extension_t *extension = instance;
  TEN_ASSERT(extension && ten_extension_check_integrity(extension, true),
             "Should not happen.");

  ten_env_t *extension_ten_env = extension->ten_env;
  TEN_ASSERT(
      extension_ten_env && ten_env_check_integrity(extension_ten_env, true),
      "Should not happen.");

  // This happens on the extension thread, so it's thread safe.

  if (addon_context->create_instance_done_cb) {
    addon_context->create_instance_done_cb(
        self, instance, addon_context->create_instance_done_cb_data);
  }

  if (addon_context) {
    ten_addon_context_destroy(addon_context);
  }
}

void ten_extension_group_on_addon_destroy_extension_done(
    ten_env_t *self, ten_addon_context_t *addon_context) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_env_check_integrity(self, true), "Invalid use of ten_env %p.",
             self);

  TEN_ASSERT(self->attach_to == TEN_ENV_ATTACH_TO_EXTENSION_GROUP,
             "Should not happen.");

  TEN_UNUSED ten_extension_group_t *extension_group =
      ten_env_get_attached_extension_group(self);
  TEN_ASSERT(extension_group &&
                 ten_extension_group_check_integrity(extension_group, true),
             "Should not happen.");

  if (addon_context->destroy_instance_done_cb) {
    addon_context->destroy_instance_done_cb(
        self, addon_context->destroy_instance_done_cb_data);
  }

  ten_addon_context_destroy(addon_context);
}

const char *ten_extension_group_get_name(ten_extension_group_t *self,
                                         bool check_thread) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_group_check_integrity(self, check_thread),
             "Invalid use of extension group %p.", self);

  return ten_string_get_raw_str(&self->name);
}
