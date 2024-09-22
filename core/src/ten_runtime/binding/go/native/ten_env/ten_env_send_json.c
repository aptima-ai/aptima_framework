//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <stdlib.h>

#include "include_internal/ten_runtime/binding/go/internal/common.h"
#include "include_internal/ten_runtime/binding/go/internal/json.h"
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env.h"
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env_internal.h"
#include "ten_runtime/binding/go/interface/ten/common.h"
#include "ten_runtime/binding/go/interface/ten/ten_env.h"
#include "ten_runtime/common/errno.h"
#include "ten_runtime/ten_env_proxy/ten_env_proxy.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"

typedef struct ten_env_notify_send_json_info_t {
  ten_json_t *c_json;
  ten_go_handle_t handler_id;
} ten_env_notify_send_json_info_t;

static ten_env_notify_send_json_info_t *ten_env_notify_send_json_info_create(
    ten_json_t *c_json, ten_go_handle_t handler_id) {
  TEN_ASSERT(c_json, "Invalid argument.");

  ten_env_notify_send_json_info_t *info =
      TEN_MALLOC(sizeof(ten_env_notify_send_json_info_t));
  TEN_ASSERT(info, "Failed to allocate memory.");

  info->c_json = ten_json_incref(c_json);
  info->handler_id = handler_id;

  return info;
}

static void ten_env_notify_send_json_info_destroy(
    ten_env_notify_send_json_info_t *info) {
  TEN_ASSERT(info, "Invalid argument.");

  if (info->c_json) {
    ten_json_destroy(info->c_json);
    info->c_json = NULL;
  }

  info->handler_id = 0;

  TEN_FREE(info);
}

static void ten_env_proxy_notify_send_json(ten_env_t *ten_env,
                                           void *user_data) {
  TEN_ASSERT(user_data, "Invalid argument.");
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_env_notify_send_json_info_t *notify_info = user_data;

  ten_error_t err;
  ten_error_init(&err);

  TEN_UNUSED bool res = false;
  if (notify_info->handler_id == TEN_GO_NO_RESPONSE_HANDLER) {
    res = ten_env_send_json(ten_env, notify_info->c_json, NULL, NULL, NULL);
  } else {
    ten_go_callback_info_t *info =
        ten_go_callback_info_create(notify_info->handler_id);
    res = ten_env_send_json(ten_env, notify_info->c_json,
                            proxy_send_xxx_callback, info, NULL);
  }

  ten_error_deinit(&err);

  ten_env_notify_send_json_info_destroy(notify_info);
}

ten_go_status_t ten_go_ten_env_send_json(uintptr_t bridge_addr,
                                         const void *json_bytes,
                                         int json_bytes_len,
                                         ten_go_handle_t handler_id) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");
  TEN_ASSERT(json_bytes && json_bytes_len > 0, "Should not happen.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  TEN_GO_TEN_IS_ALIVE_REGION_BEGIN(
      self, { ten_go_status_set_errno(&status, TEN_ERRNO_TEN_IS_CLOSED); });

  ten_json_t *json = ten_go_json_loads(json_bytes, json_bytes_len, &status);
  if (json == NULL) {
    return status;
  }

  ten_env_notify_send_json_info_t *notify_info =
      ten_env_notify_send_json_info_create(
          json, handler_id <= 0 ? TEN_GO_NO_RESPONSE_HANDLER : handler_id);
  ten_json_destroy(json);

  ten_error_t err;
  ten_error_init(&err);
  if (!ten_env_proxy_notify(self->c_ten_env_proxy,
                            ten_env_proxy_notify_send_json, notify_info, false,
                            &err)) {
    ten_env_notify_send_json_info_destroy(notify_info);
    ten_go_status_from_error(&status, &err);
  }

  ten_error_deinit(&err);
  TEN_GO_TEN_IS_ALIVE_REGION_END(self);

ten_is_close:
  return status;
}
