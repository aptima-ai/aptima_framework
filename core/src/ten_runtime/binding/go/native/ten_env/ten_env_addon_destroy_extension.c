//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <stdlib.h>

#include "include_internal/ten_runtime/binding/go/extension/extension.h"
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env.h"
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env_internal.h"
#include "include_internal/ten_runtime/extension/extension.h"
#include "include_internal/ten_runtime/extension_group/extension_group.h"
#include "ten_runtime/addon/extension/extension.h"
#include "ten_runtime/binding/go/interface/ten/common.h"
#include "ten_runtime/binding/go/interface/ten/ten_env.h"
#include "ten_runtime/ten.h"
#include "ten_runtime/ten_env/ten_env.h"
#include "ten_runtime/ten_env_proxy/ten_env_proxy.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"

typedef struct ten_env_notify_addon_destroy_extension_info_t {
  ten_extension_t *c_extension;
  ten_go_callback_info_t *callback_info;
} ten_env_notify_addon_destroy_extension_info_t;

static ten_env_notify_addon_destroy_extension_info_t *
ten_env_notify_addon_destroy_extension_info_create(
    ten_extension_t *c_extension, ten_go_callback_info_t *callback_info) {
  TEN_ASSERT(c_extension, "Invalid argument.");

  ten_env_notify_addon_destroy_extension_info_t *info =
      TEN_MALLOC(sizeof(ten_env_notify_addon_destroy_extension_info_t));
  TEN_ASSERT(info, "Failed to allocate memory.");

  info->c_extension = c_extension;
  info->callback_info = callback_info;

  return info;
}

static void ten_env_notify_addon_destroy_extension_info_destroy(
    ten_env_notify_addon_destroy_extension_info_t *info) {
  TEN_ASSERT(info, "Invalid argument.");

  info->c_extension = NULL;
  info->callback_info = NULL;

  TEN_FREE(info);
}

static void proxy_addon_destroy_extension_done(ten_env_t *ten_env,
                                               void *cb_data) {
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");
  TEN_ASSERT(ten_env->attach_to == TEN_ENV_ATTACH_TO_EXTENSION_GROUP,
             "Should not happen.");

  TEN_UNUSED ten_extension_group_t *extension_group =
      ten_env_get_attached_target(ten_env);
  TEN_ASSERT(extension_group &&
                 ten_extension_group_check_integrity(extension_group, true),
             "Should not happen.");

  ten_env_notify_addon_destroy_extension_info_t *info = cb_data;

  ten_go_callback_info_t *callback_info = info->callback_info;
  TEN_ASSERT(callback_info, "Should not happen.");

  ten_go_ten_env_t *ten_env_bridge = ten_go_ten_env_wrap(ten_env);

  tenGoOnAddonDestroyExtensionDone(ten_env_bridge->bridge.go_instance,
                                   callback_info->callback_id);

  ten_env_notify_addon_destroy_extension_info_destroy(info);
}

static void ten_env_proxy_notify_addon_destroy_extension(ten_env_t *ten_env,
                                                         void *user_data) {
  TEN_ASSERT(user_data, "Invalid argument.");
  TEN_ASSERT(
      ten_env &&
          ten_env_check_integrity(
              ten_env,
              ten_env->attach_to != TEN_ENV_ATTACH_TO_ADDON ? true : false),
      "Should not happen.");

  ten_env_notify_addon_destroy_extension_info_t *info = user_data;

  ten_error_t err;
  ten_error_init(&err);

  bool rc = ten_addon_destroy_extension(ten_env, info->c_extension,
                                        proxy_addon_destroy_extension_done,
                                        info, &err);
  TEN_ASSERT(rc, "Should not happen.");

  ten_error_deinit(&err);
}

void ten_go_ten_env_addon_destroy_extension(uintptr_t bridge_addr,
                                            uintptr_t extension_bridge_addr,
                                            ten_go_handle_t callback) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");

  ten_go_extension_t *extension_bridge =
      ten_go_extension_reinterpret(extension_bridge_addr);
  TEN_ASSERT(
      extension_bridge && ten_go_extension_check_integrity(extension_bridge),
      "Should not happen.");

  TEN_GO_TEN_ENV_IS_ALIVE_REGION_BEGIN(self, {});

  ten_error_t err;
  ten_error_init(&err);

  ten_go_callback_info_t *callback_info = ten_go_callback_info_create(callback);
  ten_env_notify_addon_destroy_extension_info_t *addon_extension_destroy_info =
      ten_env_notify_addon_destroy_extension_info_create(
          ten_go_extension_c_extension(extension_bridge), callback_info);

  if (!ten_env_proxy_notify(self->c_ten_env_proxy,
                            ten_env_proxy_notify_addon_destroy_extension,
                            addon_extension_destroy_info, false, &err)) {
    TEN_LOGD("TEN/GO failed to addon_extension_destroy.");

    ten_env_notify_addon_destroy_extension_info_destroy(
        addon_extension_destroy_info);
  }

  ten_error_deinit(&err);
  TEN_GO_TEN_ENV_IS_ALIVE_REGION_END(self);
ten_is_close:
  return;
}
