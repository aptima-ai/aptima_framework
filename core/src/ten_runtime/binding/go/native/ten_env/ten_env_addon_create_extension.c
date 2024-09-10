//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include <stdlib.h>

#include "include_internal/ten_runtime/addon/addon.h"
#include "include_internal/ten_runtime/binding/go/addon/addon.h"
#include "include_internal/ten_runtime/binding/go/extension/extension.h"
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env.h"
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env_internal.h"
#include "include_internal/ten_runtime/extension/extension.h"
#include "include_internal/ten_runtime/extension_group/extension_group.h"
#include "ten_runtime/addon/extension/extension.h"
#include "ten_runtime/binding/common.h"
#include "ten_runtime/binding/go/interface/ten/common.h"
#include "ten_runtime/binding/go/interface/ten/ten.h"
#include "ten_runtime/ten.h"
#include "ten_runtime/ten_env/ten_env.h"
#include "ten_runtime/ten_env_proxy/ten_env_proxy.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/macro/check.h"

typedef struct addon_create_extension_done_call_info_t {
  ten_extension_t *extension;
  ten_env_t *extension_group_ten;
} addon_create_extension_done_call_info_t;

typedef struct ten_env_notify_addon_create_extension_info_t {
  ten_string_t addon_name;
  ten_string_t instance_name;
  ten_go_callback_info_t *callback_info;
} ten_env_notify_addon_create_extension_info_t;

static ten_env_notify_addon_create_extension_info_t *
ten_env_notify_addon_create_extension_info_create(
    const char *cmd_id, const char *cmd_name,
    ten_go_callback_info_t *callback_info) {
  TEN_ASSERT(cmd_id && cmd_name, "Invalid argument.");

  ten_env_notify_addon_create_extension_info_t *info =
      TEN_MALLOC(sizeof(ten_env_notify_addon_create_extension_info_t));
  TEN_ASSERT(info, "Failed to allocate memory.");

  ten_string_init_formatted(&info->addon_name, "%s", cmd_id);
  ten_string_init_formatted(&info->instance_name, "%s", cmd_name);
  info->callback_info = callback_info;

  return info;
}

static void ten_env_notify_addon_create_extension_info_destroy(
    ten_env_notify_addon_create_extension_info_t *info) {
  TEN_ASSERT(info, "Invalid argument.");

  ten_string_deinit(&info->addon_name);
  ten_string_deinit(&info->instance_name);
  info->callback_info = NULL;

  TEN_FREE(info);
}

static void proxy_addon_create_extension_done(ten_env_t *ten_env,
                                              void *instance, void *cb_data) {
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");
  TEN_ASSERT(ten_env->attach_to == TEN_ENV_ATTACH_TO_EXTENSION_GROUP,
             "Should not happen.");

  TEN_UNUSED ten_extension_group_t *extension_group =
      ten_env_get_attached_target(ten_env);
  TEN_ASSERT(extension_group &&
                 ten_extension_group_check_integrity(extension_group, true),
             "Should not happen.");

  ten_extension_t *extension = instance;
  TEN_ASSERT(extension && ten_extension_check_integrity(extension, true),
             "Should not happen.");

  ten_addon_host_t *addon_host = ten_extension_get_addon(extension);
  TEN_ASSERT(addon_host, "Should not happen.");

  ten_go_addon_t *addon_bridge =
      addon_host->addon->binding_handle.me_in_target_lang;
  TEN_ASSERT(addon_bridge && ten_go_addon_check_integrity(addon_bridge),
             "Should not happen.");

  ten_go_callback_info_t *callback_info = cb_data;
  TEN_ASSERT(callback_info, "Should not happen.");

  ten_go_ten_env_t *ten_bridge = ten_go_ten_env_wrap(ten_env);
  ten_go_extension_t *extension_bridge =
      ten_binding_handle_get_me_in_target_lang(
          (ten_binding_handle_t *)extension);

  tenGoOnAddonCreateExtensionDone(
      ten_bridge->bridge.go_instance, ten_go_addon_go_handle(addon_bridge),
      ten_go_extension_go_handle(extension_bridge), callback_info->callback_id);

  ten_go_callback_info_destroy(callback_info);
}

static void ten_env_notify_addon_create_extension(ten_env_t *ten_env,
                                                  void *user_data) {
  TEN_ASSERT(user_data, "Invalid argument.");
  TEN_ASSERT(
      ten_env &&
          ten_env_check_integrity(
              ten_env,
              ten_env->attach_to != TEN_ENV_ATTACH_TO_ADDON ? true : false),
      "Should not happen.");

  ten_env_notify_addon_create_extension_info_t *info = user_data;

  ten_error_t err;
  ten_error_init(&err);

  bool rc = ten_addon_create_extension_async(
      ten_env, ten_string_get_raw_str(&info->addon_name),
      ten_string_get_raw_str(&info->instance_name),
      proxy_addon_create_extension_done, info->callback_info, &err);
  TEN_ASSERT(rc, "Should not happen.");

  ten_error_deinit(&err);

  ten_env_notify_addon_create_extension_info_destroy(info);
}

bool ten_go_ten_env_addon_create_extension(uintptr_t bridge_addr,
                                           const char *addon_name,
                                           const char *instance_name,
                                           ten_go_handle_t callback) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");
  TEN_ASSERT(addon_name && instance_name, "Should not happen.");

  bool result = true;

  TEN_GO_TEN_IS_ALIVE_REGION_BEGIN(self, result = false;);

  ten_error_t err;
  ten_error_init(&err);

  ten_go_callback_info_t *callback_info = ten_go_callback_info_create(callback);
  ten_env_notify_addon_create_extension_info_t *addon_extension_create_info =
      ten_env_notify_addon_create_extension_info_create(
          addon_name, instance_name, callback_info);

  if (!ten_env_proxy_notify(self->c_ten_proxy,
                            ten_env_notify_addon_create_extension,
                            addon_extension_create_info, false, &err)) {
    TEN_LOGD("TEN/GO failed to addon_extension_create.");

    ten_env_notify_addon_create_extension_info_destroy(
        addon_extension_create_info);
    result = false;
  }

  ten_error_deinit(&err);
  TEN_GO_TEN_IS_ALIVE_REGION_END(self);
ten_is_close:
  return result;
}
