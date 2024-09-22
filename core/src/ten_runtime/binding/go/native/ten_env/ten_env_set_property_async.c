//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <stdlib.h>

#include "include_internal/ten_runtime/binding/go/ten_env/ten_env.h"
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env_internal.h"
#include "include_internal/ten_runtime/binding/go/value/value.h"
#include "ten_runtime/binding/go/interface/ten/ten_env.h"
#include "ten_runtime/binding/go/interface/ten/value.h"
#include "ten_runtime/ten_env_proxy/ten_env_proxy.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"

typedef struct ten_env_notify_set_property_info_t {
  bool result;
  ten_string_t path;
  ten_value_t *c_value;
  ten_go_callback_info_t *info;
} ten_env_notify_set_property_info_t;

static ten_env_notify_set_property_info_t *
ten_env_notify_set_property_info_create(const char *name, ten_value_t *c_value,
                                        ten_go_callback_info_t *info) {
  ten_env_notify_set_property_info_t *set_property_info =
      TEN_MALLOC(sizeof(ten_env_notify_set_property_info_t));
  TEN_ASSERT(info, "Failed to allocate memory.");

  set_property_info->result = true;
  ten_string_init_formatted(&set_property_info->path, "%s", name);
  set_property_info->c_value = ten_value_clone(c_value);
  set_property_info->info = info;

  return set_property_info;
}

static void ten_env_notify_set_property_info_destroy(
    ten_env_notify_set_property_info_t *info, bool delete_value) {
  TEN_ASSERT(info, "Invalid argument.");

  ten_string_deinit(&info->path);
  if (delete_value && info->c_value) {
    ten_value_destroy(info->c_value);
    info->c_value = NULL;
  }
  ten_go_callback_info_destroy(info->info);

  TEN_FREE(info);
}

static void proxy_set_property_callback(ten_env_t *ten_env, bool res,
                                        void *cb_data,
                                        TEN_UNUSED ten_error_t *err) {
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_env_notify_set_property_info_t *set_property_info = cb_data;
  TEN_ASSERT(set_property_info, "Should not happen.");

  ten_go_handle_t handler_id = set_property_info->info->callback_id;
  ten_go_ten_env_t *ten_bridge = ten_go_ten_env_wrap(ten_env);

  tenGoSetPropertyCallback(ten_bridge->bridge.go_instance, handler_id, res);

  ten_env_notify_set_property_info_destroy(set_property_info, false);
}

static void ten_env_proxy_notify_set_property(ten_env_t *ten_env,
                                              void *user_data) {
  TEN_ASSERT(user_data, "Invalid argument.");
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_env_notify_set_property_info_t *set_property_info = user_data;
  TEN_ASSERT(set_property_info, "Should not happen.");

  ten_error_t err;
  ten_error_init(&err);

  bool rc = ten_env_set_property_async(
      ten_env, ten_string_get_raw_str(&set_property_info->path),
      set_property_info->c_value, proxy_set_property_callback,
      set_property_info, &err);
  TEN_ASSERT(rc, "Should not happen.");

  ten_error_deinit(&err);
}

bool ten_go_ten_env_set_property_async(uintptr_t bridge_addr, const char *name,
                                       ten_go_value_t *value,
                                       ten_go_handle_t callback_id) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");
  TEN_ASSERT(value && ten_go_value_check_integrity(value),
             "Should not happen.");

  bool result = true;
  TEN_GO_TEN_IS_ALIVE_REGION_BEGIN(self, result = false;);

  ten_error_t err;
  ten_error_init(&err);

  ten_env_notify_set_property_info_t *set_property_info =
      ten_env_notify_set_property_info_create(
          name, ten_go_value_c_value(value),
          ten_go_callback_info_create(callback_id));

  if (!ten_env_proxy_notify(self->c_ten_env_proxy,
                            ten_env_proxy_notify_set_property,
                            set_property_info, false, &err)) {
    TEN_LOGD("TEN/GO failed to set_property: %s", name);
    ten_env_notify_set_property_info_destroy(set_property_info, true);
    result = false;
  }

  ten_error_deinit(&err);
  TEN_GO_TEN_IS_ALIVE_REGION_END(self);
ten_is_close:
  return result;
}
