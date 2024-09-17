//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "ten_runtime/addon/extension/extension.h"

#include "include_internal/ten_runtime/addon/addon.h"
#include "include_internal/ten_runtime/addon/common/store.h"
#include "include_internal/ten_runtime/extension/extension.h"
#include "include_internal/ten_runtime/extension_group/extension_group.h"
#include "include_internal/ten_runtime/extension_thread/extension_thread.h"
#include "include_internal/ten_runtime/extension_thread/on_xxx.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "ten_runtime/addon/addon.h"
#include "ten_runtime/ten_env/ten_env.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"

static ten_addon_store_t g_extension_store = {
    false,
    NULL,
    TEN_LIST_INIT_VAL,
};

ten_addon_store_t *ten_extension_get_store(void) { return &g_extension_store; }

ten_extension_t *ten_addon_create_extension(ten_env_t *ten_env,
                                            const char *addon_name,
                                            const char *instance_name,
                                            TEN_UNUSED ten_error_t *err) {
  TEN_ASSERT(addon_name && instance_name, "Should not happen.");
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");
  TEN_ASSERT(ten_env->attach_to == TEN_ENV_ATTACH_TO_EXTENSION_GROUP,
             "Should not happen.");

  return ten_addon_create_instance(ten_env, addon_name, instance_name,
                                   TEN_ADDON_TYPE_EXTENSION);
}

bool ten_addon_create_extension_async_for_mock(
    ten_env_t *ten_env, const char *addon_name, const char *instance_name,
    ten_env_addon_on_create_instance_async_cb_t cb, void *cb_data,
    TEN_UNUSED ten_error_t *err) {
  TEN_ASSERT(addon_name && instance_name, "Should not happen.");
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  // Use this trick to remember that this mock_ten is for use by the
  // extension.
  ten_env_set_attach_to(ten_env, TEN_ENV_ATTACH_TO_EXTENSION, err);

  return ten_addon_create_instance_async(ten_env, addon_name, instance_name,
                                         TEN_ADDON_TYPE_EXTENSION, cb, cb_data);
}

bool ten_addon_create_extension_async(
    ten_env_t *ten_env, const char *addon_name, const char *instance_name,
    ten_env_addon_on_create_instance_async_cb_t cb, void *cb_data,
    TEN_UNUSED ten_error_t *err) {
  TEN_ASSERT(addon_name && instance_name, "Should not happen.");

  TEN_ASSERT(ten_env, "Invalid argument.");
  TEN_ASSERT(ten_env_check_integrity(ten_env, true),
             "Invalid use of ten_env %p.", ten_env);

  TEN_ASSERT(ten_env->attach_to == TEN_ENV_ATTACH_TO_EXTENSION_GROUP,
             "Should not happen.");

  // Because there might be more than one extension threads to create extensions
  // from the corresponding extension addons simultaneously. So we can _not_
  // save the pointers of 'cb' and 'user_data' into 'ten_env', they will
  // override each other. Instead we need to pass those pointers through
  // parameters below.

  ten_extension_group_t *extension_group =
      ten_env_get_attached_extension_group(ten_env);
  TEN_ASSERT(extension_group &&
                 ten_extension_group_check_integrity(extension_group, true),
             "Should not happen.");

  // Check whether current thread is extension thread. If not, we should switch
  // to extension thread.
  if (ten_extension_thread_call_by_me(extension_group->extension_thread)) {
    return ten_addon_create_instance_async(ten_env, addon_name, instance_name,
                                           TEN_ADDON_TYPE_EXTENSION, cb,
                                           cb_data);
  } else {
    ten_addon_on_create_instance_info_t *addon_instance_info =
        ten_addon_on_create_instance_info_create(
            addon_name, instance_name, TEN_ADDON_TYPE_EXTENSION, cb, cb_data);

    ten_runloop_post_task_tail(
        ten_extension_group_get_attached_runloop(extension_group),
        ten_extension_thread_create_addon_instance,
        extension_group->extension_thread, addon_instance_info);
    return true;
  }
}

bool ten_addon_destroy_extension(ten_env_t *ten_env, ten_extension_t *extension,
                                 TEN_UNUSED ten_error_t *err) {
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");
  TEN_ASSERT(extension && ten_extension_check_integrity(extension, true),
             "Should not happen.");
  TEN_ASSERT(ten_env->attach_to == TEN_ENV_ATTACH_TO_EXTENSION_GROUP,
             "Should not happen.");

  ten_addon_host_t *addon_host = extension->addon_host;
  TEN_ASSERT(addon_host,
             "Should not happen, otherwise, memory leakage will occur.");

  return ten_addon_host_destroy_instance(addon_host, ten_env, extension);
}

bool ten_addon_destroy_extension_async_for_mock(
    ten_env_t *ten_env, ten_extension_t *extension,
    ten_env_addon_on_destroy_instance_async_cb_t cb, void *cb_data,
    TEN_UNUSED ten_error_t *err) {
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");
  TEN_ASSERT(extension && ten_extension_check_integrity(extension, true),
             "Should not happen.");

  ten_addon_host_t *addon_host = extension->addon_host;
  TEN_ASSERT(addon_host,
             "Should not happen, otherwise, memory leakage will occur.");

  return ten_addon_host_destroy_instance_async(addon_host, ten_env, extension,
                                               cb, cb_data);
}

bool ten_addon_destroy_extension_async(
    ten_env_t *ten_env, ten_extension_t *extension,
    ten_env_addon_on_destroy_instance_async_cb_t cb, void *cb_data,
    TEN_UNUSED ten_error_t *err) {
  TEN_ASSERT(ten_env, "Invalid argument.");
  TEN_ASSERT(ten_env_check_integrity(ten_env, true),
             "Invalid use of ten_env %p.", ten_env);

  TEN_ASSERT(cb, "Should not happen.");

  TEN_ASSERT(extension && ten_extension_check_integrity(extension, true),
             "Should not happen.");

  TEN_ASSERT(ten_env->attach_to == TEN_ENV_ATTACH_TO_EXTENSION_GROUP,
             "Should not happen.");

  ten_addon_host_t *addon_host = extension->addon_host;
  TEN_ASSERT(addon_host,
             "Should not happen, otherwise, memory leakage will occur.");

  ten_extension_group_t *extension_group =
      ten_env_get_attached_extension_group(ten_env);
  TEN_ASSERT(extension_group &&
                 ten_extension_group_check_integrity(extension_group, true),
             "Should not happen.");

  if (ten_extension_thread_call_by_me(extension_group->extension_thread)) {
    return ten_addon_host_destroy_instance_async(addon_host, ten_env, extension,
                                                 cb, cb_data);
  } else {
    ten_addon_on_destroy_instance_info_t *destroy_instance_info =
        ten_addon_host_on_destroy_instance_info_create(addon_host, extension,
                                                       cb, cb_data);

    ten_runloop_post_task_tail(
        ten_extension_group_get_attached_runloop(extension_group),
        ten_extension_thread_destroy_addon_instance,
        extension_group->extension_thread, destroy_instance_info);
    return true;
  }
}

ten_addon_host_t *ten_addon_register_extension(const char *name,
                                               ten_addon_t *addon) {
  if (!name || strlen(name) == 0) {
    TEN_LOGE("The addon name is required.");
    exit(EXIT_FAILURE);
  }

  ten_addon_store_init(ten_extension_get_store());

  ten_addon_host_t *addon_host =
      ten_addon_host_create(TEN_ADDON_TYPE_EXTENSION);
  TEN_ASSERT(addon_host, "Should not happen.");

  ten_addon_register(ten_extension_get_store(), addon_host, name, addon);
  TEN_LOGI("Registered addon '%s' as extension",
           ten_string_get_raw_str(&addon_host->name));

  return addon_host;
}

void ten_addon_unregister_extension(const char *name,
                                    TEN_UNUSED ten_addon_t *addon) {
  TEN_ASSERT(name, "Should not happen.");

  TEN_LOGV("Unregistered addon of extension '%s'.", name);

  ten_addon_unregister(ten_extension_get_store(), name, addon);
}
