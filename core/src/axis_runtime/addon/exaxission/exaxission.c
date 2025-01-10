//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_runtime/addon/extension/extension.h"

#include "include_internal/axis_runtime/addon/addon.h"
#include "include_internal/axis_runtime/addon/addon_host.h"
#include "include_internal/axis_runtime/addon/common/store.h"
#include "include_internal/axis_runtime/addon/extension/extension.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension_group/extension_group.h"
#include "include_internal/axis_runtime/extension_thread/extension_thread.h"
#include "include_internal/axis_runtime/extension_thread/on_xxx.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "axis_runtime/addon/addon.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"
#include "axis_utils/macro/memory.h"

static axis_addon_store_t g_extension_store = {
    false,
    NULL,
    axis_LIST_INIT_VAL,
};

axis_addon_store_t *axis_extension_get_global_store(void) {
  axis_addon_store_init(&g_extension_store);
  return &g_extension_store;
}

static axis_addon_on_create_extension_instance_ctx_t *
axis_addon_on_create_extension_instance_ctx_create(
    axis_ADDON_TYPE addon_type, const char *addon_name,
    const char *instance_name, axis_env_addon_create_instance_done_cb_t cb,
    void *cb_data) {
  axis_ASSERT(addon_name && instance_name, "Should not happen.");

  axis_addon_on_create_extension_instance_ctx_t *self =
      (axis_addon_on_create_extension_instance_ctx_t *)axis_MALLOC(
          sizeof(axis_addon_on_create_extension_instance_ctx_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_string_init_formatted(&self->addon_name, "%s", addon_name);
  axis_string_init_formatted(&self->instance_name, "%s", instance_name);
  self->addon_type = addon_type;
  self->cb = cb;
  self->cb_data = cb_data;

  return self;
}

void axis_addon_on_create_extension_instance_ctx_destroy(
    axis_addon_on_create_extension_instance_ctx_t *self) {
  axis_ASSERT(self, "Should not happen.");

  axis_string_deinit(&self->addon_name);
  axis_string_deinit(&self->instance_name);

  axis_FREE(self);
}

axis_addon_create_extension_done_ctx_t *
axis_addon_create_extension_done_ctx_create(
    const char *extension_name,
    axis_extension_group_create_extensions_done_ctx_t *ctx) {
  axis_ASSERT(extension_name && ctx, "Invalid argument.");

  axis_addon_create_extension_done_ctx_t *self =
      axis_MALLOC(sizeof(axis_addon_create_extension_done_ctx_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_string_init_from_c_str(&self->extension_name, extension_name,
                             strlen(extension_name));
  self->create_extensions_done_ctx = ctx;

  return self;
}

void axis_addon_create_extension_done_ctx_destroy(
    axis_addon_create_extension_done_ctx_t *self) {
  axis_ASSERT(self, "Should not happen.");

  axis_string_deinit(&self->extension_name);

  axis_FREE(self);
}

bool axis_addon_create_extension(axis_env_t *axis_env, const char *addon_name,
                                const char *instance_name,
                                axis_env_addon_create_instance_done_cb_t cb,
                                void *cb_data, axis_UNUSED axis_error_t *err) {
  axis_ASSERT(addon_name && instance_name, "Should not happen.");

  axis_ASSERT(axis_env, "Invalid argument.");
  axis_ASSERT(axis_env_check_integrity(axis_env, true),
             "Invalid use of axis_env %p.", axis_env);

  axis_ASSERT(axis_env->attach_to == axis_ENV_ATTACH_TO_EXTENSION_GROUP,
             "Should not happen.");

  // Because there might be more than one extension threads to create extensions
  // from the corresponding extension addons simultaneously. So we can _not_
  // save the pointers of 'cb' and 'user_data' into 'axis_env', they will
  // override each other. Instead we need to pass those pointers through
  // parameters below.

  axis_extension_group_t *extension_group =
      axis_env_get_attached_extension_group(axis_env);
  axis_ASSERT(extension_group &&
                 axis_extension_group_check_integrity(extension_group, true),
             "Should not happen.");

  // Check whether current thread is extension thread. If not, we should switch
  // to extension thread.
  if (axis_extension_thread_call_by_me(extension_group->extension_thread)) {
    return axis_addon_create_instance_async(axis_env, axis_ADDON_TYPE_EXTENSION,
                                           addon_name, instance_name, cb,
                                           cb_data);
  } else {
    axis_addon_on_create_extension_instance_ctx_t *ctx =
        axis_addon_on_create_extension_instance_ctx_create(
            axis_ADDON_TYPE_EXTENSION, addon_name, instance_name, cb, cb_data);

    axis_runloop_post_task_tail(
        axis_extension_group_get_attached_runloop(extension_group),
        axis_extension_thread_create_extension_instance,
        extension_group->extension_thread, ctx);
    return true;
  }
}

bool axis_addon_destroy_extension(axis_env_t *axis_env, axis_extension_t *extension,
                                 axis_env_addon_destroy_instance_done_cb_t cb,
                                 void *cb_data, axis_UNUSED axis_error_t *err) {
  axis_ASSERT(axis_env, "Invalid argument.");
  axis_ASSERT(axis_env_check_integrity(axis_env, true),
             "Invalid use of axis_env %p.", axis_env);

  axis_ASSERT(cb, "Should not happen.");

  axis_ASSERT(extension && axis_extension_check_integrity(extension, true),
             "Should not happen.");

  axis_ASSERT(axis_env->attach_to == axis_ENV_ATTACH_TO_EXTENSION_GROUP,
             "Should not happen.");

  axis_addon_host_t *addon_host = extension->addon_host;
  axis_ASSERT(addon_host,
             "Should not happen, otherwise, memory leakage will occur.");

  axis_extension_group_t *extension_group =
      axis_env_get_attached_extension_group(axis_env);
  axis_ASSERT(extension_group &&
                 axis_extension_group_check_integrity(extension_group, true),
             "Should not happen.");

  if (axis_extension_thread_call_by_me(extension_group->extension_thread)) {
    return axis_addon_host_destroy_instance_async(addon_host, axis_env, extension,
                                                 cb, cb_data);
  } else {
    axis_addon_host_on_destroy_instance_ctx_t *destroy_instance_info =
        axis_addon_host_on_destroy_instance_ctx_create(addon_host, extension, cb,
                                                      cb_data);

    axis_runloop_post_task_tail(
        axis_extension_group_get_attached_runloop(extension_group),
        axis_extension_thread_destroy_addon_instance,
        extension_group->extension_thread, destroy_instance_info);
    return true;
  }
}

axis_addon_host_t *axis_addon_register_extension(const char *name,
                                               const char *base_dir,
                                               axis_addon_t *addon,
                                               void *register_ctx) {
  return axis_addon_register(axis_ADDON_TYPE_EXTENSION, name, base_dir, addon,
                            register_ctx);
}

axis_addon_t *axis_addon_unregister_extension(const char *name) {
  axis_ASSERT(name, "Should not happen.");

  return axis_addon_unregister(axis_extension_get_global_store(), name);
}

void axis_addon_unregister_all_extension(void) {
  axis_addon_store_del_all(axis_extension_get_global_store());
}
