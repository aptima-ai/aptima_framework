//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/addon/extension_group/extension_group.h"

#include "include_internal/axis_runtime/addon/addon.h"
#include "include_internal/axis_runtime/addon/addon_host.h"
#include "include_internal/axis_runtime/addon/common/store.h"
#include "include_internal/axis_runtime/engine/engine.h"
#include "include_internal/axis_runtime/extension_group/extension_group.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "axis_utils/macro/check.h"

static axis_addon_store_t g_extension_group_store = {
    false,
    NULL,
    axis_LIST_INIT_VAL,
};

axis_addon_store_t *axis_extension_group_get_global_store(void) {
  axis_addon_store_init(&g_extension_group_store);
  return &g_extension_group_store;
}

axis_addon_host_t *axis_addon_register_extension_group(const char *name,
                                                     const char *base_dir,
                                                     axis_addon_t *addon,
                                                     void *register_ctx) {
  return axis_addon_register(axis_ADDON_TYPE_EXTENSION_GROUP, name, base_dir,
                            addon, register_ctx);
}

axis_addon_t *axis_addon_unregister_extension_group(const char *name) {
  axis_ASSERT(name, "Should not happen.");

  return axis_addon_unregister(axis_extension_group_get_global_store(), name);
}

bool axis_addon_create_extension_group(
    axis_env_t *axis_env, const char *addon_name, const char *instance_name,
    axis_env_addon_create_instance_done_cb_t cb, void *user_data) {
  axis_ASSERT(addon_name && instance_name, "Should not happen.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");
  axis_ASSERT(axis_env->attach_to == axis_ENV_ATTACH_TO_ENGINE,
             "Should not happen.");

  // Because there might be more than one engine (threads) to create extension
  // groups from the corresponding extension group addons simultaneously. So we
  // can _not_ save the pointers of 'cb' and 'user_data' into 'axis_env', they
  // will override each other. Instead we need to pass those pointers through
  // parameters below.

  axis_engine_t *engine = axis_env_get_attached_engine(axis_env);
  axis_ASSERT(engine && axis_engine_check_integrity(engine, true),
             "Should not happen.");

  return axis_addon_create_instance_async(
      axis_env, axis_ADDON_TYPE_EXTENSION_GROUP, addon_name, instance_name, cb,
      user_data);
}

bool axis_addon_destroy_extension_group(
    axis_env_t *axis_env, axis_extension_group_t *extension_group,
    axis_env_addon_destroy_instance_done_cb_t cb, void *cb_data) {
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true) && cb,
             "Should not happen.");
  axis_ASSERT(extension_group &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: The extension thread has been stopped.
                 axis_extension_group_check_integrity(extension_group, false),
             "Should not happen.");
  axis_ASSERT(axis_env->attach_to == axis_ENV_ATTACH_TO_ENGINE,
             "Should not happen.");

  axis_engine_t *engine = axis_env_get_attached_engine(axis_env);
  axis_ASSERT(engine && axis_engine_check_integrity(engine, true),
             "Should not happen.");

  axis_addon_host_t *addon_host = extension_group->addon_host;
  axis_ASSERT(addon_host,
             "Should not happen, otherwise, memory leakage will occur.");

  return axis_addon_host_destroy_instance_async(addon_host, axis_env,
                                               extension_group, cb, cb_data);
}

void axis_addon_unregister_all_extension_group(void) {
  axis_addon_store_del_all(axis_extension_group_get_global_store());
}
