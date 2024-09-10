//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include <stdlib.h>

#include "include_internal/ten_runtime/binding/go/ten_env/ten_env.h"
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env_internal.h"
#include "include_internal/ten_runtime/binding/go/value/value.h"
#include "ten_runtime/binding/go/interface/ten/ten.h"
#include "ten_runtime/binding/go/interface/ten/value.h"
#include "ten_runtime/ten_env_proxy/ten_env_proxy.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/macro/check.h"

typedef struct ten_env_notify_get_property_info_t {
  ten_string_t path;
  ten_value_t *c_value;
  ten_go_callback_info_t *info;
} ten_env_notify_get_property_info_t;

static ten_env_notify_get_property_info_t *
ten_env_notify_get_property_info_create(const char *path,
                                        ten_go_callback_info_t *info) {
  ten_env_notify_get_property_info_t *get_property_info =
      TEN_MALLOC(sizeof(ten_env_notify_get_property_info_t));
  TEN_ASSERT(info, "Failed to allocate memory.");

  ten_string_init_formatted(&get_property_info->path, "%s", path);
  get_property_info->c_value = NULL;
  get_property_info->info = info;

  return get_property_info;
}

static void ten_env_notify_get_property_info_destroy(
    ten_env_notify_get_property_info_t *info) {
  TEN_ASSERT(info, "Invalid argument.");

  ten_string_deinit(&info->path);
  info->c_value = NULL;
  ten_go_callback_info_destroy(info->info);

  TEN_FREE(info);
}

static void proxy_get_property_callback(ten_env_t *ten_env, ten_value_t *res,
                                        void *cb_data,
                                        TEN_UNUSED ten_error_t *err) {
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_env_notify_get_property_info_t *get_property_info = cb_data;
  TEN_ASSERT(get_property_info, "Should not happen.");

  ten_go_handle_t handler_id = get_property_info->info->callback_id;
  ten_go_ten_env_t *ten_bridge = ten_go_ten_env_wrap(ten_env);
  ten_go_handle_t value = 0;
  if (res != NULL) {
    value = ten_go_wrap_value(res, true);
  }

  tenGoGetPropertyCallback(ten_bridge->bridge.go_instance, handler_id, value);

  ten_env_notify_get_property_info_destroy(get_property_info);
}

static void ten_notify_get_property_async(ten_env_t *ten_env, void *user_data) {
  TEN_ASSERT(user_data, "Invalid argument.");
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_env_notify_get_property_info_t *get_property_info = user_data;
  TEN_ASSERT(get_property_info, "Should not happen.");

  ten_error_t err;
  ten_error_init(&err);

  bool rc = ten_env_peek_property_async(
      ten_env, ten_string_get_raw_str(&get_property_info->path),
      proxy_get_property_callback, get_property_info, NULL);
  TEN_ASSERT(rc, "Should not happen.");

  ten_error_deinit(&err);
}

bool ten_go_ten_env_get_property_async(uintptr_t bridge_addr, const char *path,
                                       ten_go_handle_t callback_id) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");
  TEN_ASSERT(path, "Should not happen.");

  bool result = true;
  ten_env_notify_get_property_info_t *get_property_info = NULL;

  TEN_GO_TEN_IS_ALIVE_REGION_BEGIN(self, result = false;);

  ten_error_t err;
  ten_error_init(&err);

  get_property_info = ten_env_notify_get_property_info_create(
      path, ten_go_callback_info_create(callback_id));

  if (!ten_env_proxy_notify(self->c_ten_proxy, ten_notify_get_property_async,
                            get_property_info, false, &err)) {
    TEN_LOGD("TEN/GO failed to get_property.");

    ten_env_notify_get_property_info_destroy(get_property_info);
    result = false;
  }

  ten_error_deinit(&err);
  TEN_GO_TEN_IS_ALIVE_REGION_END(self);
ten_is_close:
  return result;
}
