//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <stdlib.h>
#include <sys/param.h>

#include "include_internal/ten_runtime/binding/go/internal/common.h"
#include "include_internal/ten_runtime/binding/go/msg/msg.h"
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env.h"
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env_internal.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "ten_runtime/binding/go/interface/ten/common.h"
#include "ten_runtime/binding/go/interface/ten/msg.h"
#include "ten_runtime/binding/go/interface/ten/ten_env.h"
#include "ten_runtime/common/errno.h"
#include "ten_runtime/ten_env_proxy/ten_env_proxy.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/lib/smart_ptr.h"
#include "ten_utils/macro/check.h"

typedef struct ten_env_notify_send_data_info_t {
  ten_go_msg_t *data_bridge;
  uintptr_t callback_handle;
} ten_env_notify_send_data_info_t;

static ten_env_notify_send_data_info_t *ten_env_notify_send_data_info_create(
    ten_go_msg_t *data_bridge, uintptr_t callback_handle) {
  TEN_ASSERT(data_bridge, "Invalid argument.");

  ten_env_notify_send_data_info_t *info =
      TEN_MALLOC(sizeof(ten_env_notify_send_data_info_t));
  TEN_ASSERT(info, "Failed to allocate memory.");

  info->data_bridge = data_bridge;
  info->callback_handle = callback_handle;

  return info;
}

static void ten_env_notify_send_data_info_destroy(
    ten_env_notify_send_data_info_t *info) {
  TEN_ASSERT(info, "Invalid argument.");

  TEN_FREE(info);
}

static void ten_env_proxy_notify_send_data(ten_env_t *ten_env,
                                           void *user_data) {
  TEN_ASSERT(user_data, "Invalid argument.");
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_env_notify_send_data_info_t *notify_info = user_data;

  ten_go_error_t cgo_error;
  ten_go_error_init_with_errno(&cgo_error, TEN_ERRNO_OK);

  ten_error_t err;
  ten_error_init(&err);

  bool res = ten_env_send_data(ten_env, notify_info->data_bridge->c_msg, NULL,
                               NULL, &err);
  if (res) {
    // `send_data` succeeded, transferring the ownership of the data message out
    // of the Go data message.
    ten_shared_ptr_destroy(notify_info->data_bridge->c_msg);
    notify_info->data_bridge->c_msg = NULL;
  } else {
    // Prepare error information to pass to Go.
    ten_go_error_from_error(&cgo_error, &err);
  }

  // Call back into Go to signal that the async operation in C is complete.
  tenGoCAsyncApiCallback(notify_info->callback_handle, cgo_error);

  ten_error_deinit(&err);

  ten_env_notify_send_data_info_destroy(notify_info);
}

ten_go_error_t ten_go_ten_env_send_data(uintptr_t bridge_addr,
                                        uintptr_t data_bridge_addr,
                                        uintptr_t callback_handle) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");

  ten_go_msg_t *data = ten_go_msg_reinterpret(data_bridge_addr);
  TEN_ASSERT(data && ten_go_msg_check_integrity(data), "Should not happen.");

  ten_go_error_t cgo_error;
  ten_go_error_init_with_errno(&cgo_error, TEN_ERRNO_OK);

  TEN_GO_TEN_ENV_IS_ALIVE_REGION_BEGIN(
      self, { ten_go_error_set_errno(&cgo_error, TEN_ERRNO_TEN_IS_CLOSED); });

  ten_error_t err;
  ten_error_init(&err);

  ten_env_notify_send_data_info_t *notify_info =
      ten_env_notify_send_data_info_create(data, callback_handle);

  if (!ten_env_proxy_notify(self->c_ten_env_proxy,
                            ten_env_proxy_notify_send_data, notify_info, false,
                            &err)) {
    // Failed to invoke ten_env_proxy_notify.
    ten_env_notify_send_data_info_destroy(notify_info);
    ten_go_error_from_error(&cgo_error, &err);
  }

  ten_error_deinit(&err);
  TEN_GO_TEN_ENV_IS_ALIVE_REGION_END(self);

ten_is_close:
  return cgo_error;
}
