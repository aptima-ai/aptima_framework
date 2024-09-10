//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env.h"
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env_internal.h"
#include "ten_runtime/binding/go/interface/ten/ten.h"
#include "ten_runtime/ten_env_proxy/ten_env_proxy.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/macro/check.h"

typedef struct ten_env_notify_is_cmd_connected_info_t {
  bool result;
  ten_string_t name;
  ten_event_t *completed;
} ten_env_notify_is_cmd_connected_info_t;

static ten_env_notify_is_cmd_connected_info_t *
ten_env_notify_is_cmd_connected_info_create(const char *name) {
  ten_env_notify_is_cmd_connected_info_t *info =
      TEN_MALLOC(sizeof(ten_env_notify_is_cmd_connected_info_t));
  TEN_ASSERT(info, "Failed to allocate memory.");

  info->result = true;
  ten_string_init_formatted(&info->name, "%s", name);
  info->completed = ten_event_create(0, 1);

  return info;
}

static void ten_env_notify_is_cmd_connected_info_destroy(
    ten_env_notify_is_cmd_connected_info_t *info) {
  TEN_ASSERT(info, "Invalid argument.");

  ten_string_deinit(&info->name);
  ten_event_destroy(info->completed);

  TEN_FREE(info);
}

static void ten_notify_is_cmd_connected(ten_env_t *ten_env, void *user_data) {
  TEN_ASSERT(user_data, "Invalid argument.");
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_env_notify_is_cmd_connected_info_t *info = user_data;
  TEN_ASSERT(info, "Should not happen.");

  ten_error_t err;
  ten_error_init(&err);

  info->result = ten_env_is_cmd_connected(
      ten_env, ten_string_get_raw_str(&info->name), &err);
  TEN_ASSERT(info->result, "Should not happen.");

  ten_event_set(info->completed);

  ten_error_deinit(&err);
}

bool ten_go_ten_env_is_cmd_connected(uintptr_t bridge_addr, const char *name) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");

  bool result = true;

  TEN_GO_TEN_IS_ALIVE_REGION_BEGIN(self, result = false;);

  ten_error_t err;
  ten_error_init(&err);
  ten_env_notify_is_cmd_connected_info_t *info =
      ten_env_notify_is_cmd_connected_info_create(name);

  if (!ten_env_proxy_notify(self->c_ten_proxy, ten_notify_is_cmd_connected,
                            info, false, &err)) {
    TEN_LOGD("TEN/GO failed to is_cmd_connected.");
    result = false;
    goto done;
  }

  ten_event_wait(info->completed, -1);

done:
  ten_error_deinit(&err);
  ten_env_notify_is_cmd_connected_info_destroy(info);
  TEN_GO_TEN_IS_ALIVE_REGION_END(self);
ten_is_close:
  return result;
}
