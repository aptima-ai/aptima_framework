//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <stdlib.h>

#include "include_internal/ten_runtime/binding/go/internal/common.h"
#include "include_internal/ten_runtime/binding/go/msg/msg.h"
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env.h"
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env_internal.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "ten_runtime/binding/go/interface/ten/common.h"
#include "ten_runtime/binding/go/interface/ten/msg.h"
#include "ten_runtime/binding/go/interface/ten/ten_env.h"
#include "ten_runtime/common/errno.h"
#include "ten_runtime/ten_env/internal/return.h"
#include "ten_runtime/ten_env_proxy/ten_env_proxy.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/log/log.h"
#include "ten_utils/macro/check.h"

typedef struct ten_env_notify_return_result_info_t {
  ten_shared_ptr_t *c_cmd;
  ten_shared_ptr_t *c_target_cmd;
  ten_go_handle_t handler_id;
} ten_env_notify_return_result_info_t;

static ten_env_notify_return_result_info_t *
ten_env_notify_return_result_info_create(ten_shared_ptr_t *c_cmd,
                                         ten_shared_ptr_t *c_target_cmd,
                                         ten_go_handle_t handler_id) {
  TEN_ASSERT(c_cmd, "Invalid argument.");

  ten_env_notify_return_result_info_t *info =
      TEN_MALLOC(sizeof(ten_env_notify_return_result_info_t));
  TEN_ASSERT(info, "Failed to allocate memory.");

  info->c_cmd = c_cmd;
  info->c_target_cmd = c_target_cmd;
  info->handler_id = handler_id;

  return info;
}

static void ten_env_notify_return_result_info_destroy(
    ten_env_notify_return_result_info_t *info) {
  TEN_ASSERT(info, "Invalid argument.");

  if (info->c_cmd) {
    ten_shared_ptr_destroy(info->c_cmd);
    info->c_cmd = NULL;
  }

  if (info->c_target_cmd) {
    ten_shared_ptr_destroy(info->c_target_cmd);
    info->c_target_cmd = NULL;
  }

  info->handler_id = 0;

  TEN_FREE(info);
}

static void proxy_handle_return_error(ten_env_t *ten_env, void *user_data,
                                      ten_error_t *err) {
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_go_callback_info_t *callback_info = user_data;
  TEN_ASSERT(callback_info, "Should not happen.");

  ten_go_error_t cgo_error;
  ten_go_error_init_with_errno(&cgo_error, TEN_ERRNO_OK);

  if (err) {
    ten_go_error_from_error(&cgo_error, err);
  }

  TEN_ASSERT(callback_info->callback_id != TEN_GO_NO_RESPONSE_HANDLER,
             "Should not happen.");

  ten_go_ten_env_t *ten_env_bridge = ten_go_ten_env_wrap(ten_env);

  tenGoOnError(ten_env_bridge->bridge.go_instance, callback_info->callback_id,
               cgo_error);

  ten_go_callback_info_destroy(callback_info);
}

static void ten_env_proxy_notify_return_result(ten_env_t *ten_env,
                                               void *user_data) {
  TEN_ASSERT(user_data, "Invalid argument.");
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_env_notify_return_result_info_t *info = user_data;
  TEN_ASSERT(info, "Should not happen.");

  ten_go_error_t cgo_error;
  ten_go_error_init_with_errno(&cgo_error, TEN_ERRNO_OK);

  ten_error_t err;
  ten_error_init(&err);

  bool rc = false;
  if (info->handler_id == TEN_GO_NO_RESPONSE_HANDLER) {
    if (info->c_target_cmd) {
      rc = ten_env_return_result(ten_env, info->c_cmd, info->c_target_cmd, NULL,
                                 NULL, &err);
      TEN_ASSERT(rc, "Should not happen.");
    } else {
      rc = ten_env_return_result_directly(ten_env, info->c_cmd, NULL, NULL,
                                          &err);
      TEN_ASSERT(rc, "Should not happen.");
    }

    if (!rc) {
      // The error cannot be handled by the developer, all we can do is to log
      // the error.
      TEN_LOGE(
          "Failed to return result to Go, but no error handler is provided. "
          "errno: %d, errmsg: %s",
          ten_error_errno(&err), ten_error_errmsg(&err));
    }
  } else {
    ten_go_callback_info_t *callback_info =
        ten_go_callback_info_create(info->handler_id);
    if (info->c_target_cmd) {
      rc =
          ten_env_return_result(ten_env, info->c_cmd, info->c_target_cmd,
                                proxy_handle_return_error, callback_info, &err);
    } else {
      rc = ten_env_return_result_directly(
          ten_env, info->c_cmd, proxy_handle_return_error, callback_info, &err);
    }

    if (!rc) {
      // Prepare error information to pass to Go.
      ten_go_error_from_error(&cgo_error, &err);

      ten_go_ten_env_t *ten_env_bridge = ten_go_ten_env_wrap(ten_env);

      tenGoOnError(ten_env_bridge->bridge.go_instance, info->handler_id,
                   cgo_error);

      ten_go_callback_info_destroy(callback_info);
    }
  }

  ten_error_deinit(&err);

  ten_env_notify_return_result_info_destroy(info);
}

ten_go_error_t ten_go_ten_env_return_result(uintptr_t bridge_addr,
                                            uintptr_t cmd_result_bridge_addr,
                                            uintptr_t cmd_bridge_addr,
                                            ten_go_handle_t handler_id) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");

  ten_go_msg_t *cmd = ten_go_msg_reinterpret(cmd_bridge_addr);
  TEN_ASSERT(cmd && ten_go_msg_check_integrity(cmd), "Should not happen.");
  TEN_ASSERT(ten_go_msg_c_msg(cmd), "Should not happen.");

  ten_go_msg_t *cmd_result = ten_go_msg_reinterpret(cmd_result_bridge_addr);
  TEN_ASSERT(cmd_result && ten_go_msg_check_integrity(cmd_result),
             "Should not happen.");

  ten_go_error_t cgo_error;
  ten_go_error_init_with_errno(&cgo_error, TEN_ERRNO_OK);
  TEN_GO_TEN_ENV_IS_ALIVE_REGION_BEGIN(
      self, { ten_go_error_set_errno(&cgo_error, TEN_ERRNO_TEN_IS_CLOSED); });

  ten_error_t err;
  ten_error_init(&err);

  ten_env_notify_return_result_info_t *return_result_info =
      ten_env_notify_return_result_info_create(
          ten_go_msg_move_c_msg(cmd_result), ten_go_msg_move_c_msg(cmd),
          handler_id <= 0 ? TEN_GO_NO_RESPONSE_HANDLER : handler_id);

  if (!ten_env_proxy_notify(self->c_ten_env_proxy,
                            ten_env_proxy_notify_return_result,
                            return_result_info, false, &err)) {
    ten_env_notify_return_result_info_destroy(return_result_info);
    ten_go_error_from_error(&cgo_error, &err);
  }

  ten_error_deinit(&err);
  TEN_GO_TEN_ENV_IS_ALIVE_REGION_END(self);

ten_is_close:
  return cgo_error;
}

ten_go_error_t ten_go_ten_env_return_result_directly(
    uintptr_t bridge_addr, uintptr_t cmd_result_bridge_addr,
    ten_go_handle_t handler_id) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");

  ten_go_msg_t *cmd_result = ten_go_msg_reinterpret(cmd_result_bridge_addr);
  TEN_ASSERT(cmd_result && ten_go_msg_check_integrity(cmd_result),
             "Should not happen.");

  ten_go_error_t cgo_error;
  ten_go_error_init_with_errno(&cgo_error, TEN_ERRNO_OK);

  TEN_GO_TEN_ENV_IS_ALIVE_REGION_BEGIN(
      self, { ten_go_error_set_errno(&cgo_error, TEN_ERRNO_TEN_IS_CLOSED); });
  ten_error_t err;
  ten_error_init(&err);

  ten_env_notify_return_result_info_t *return_result_info =
      ten_env_notify_return_result_info_create(
          ten_go_msg_move_c_msg(cmd_result), NULL,
          handler_id <= 0 ? TEN_GO_NO_RESPONSE_HANDLER : handler_id);

  if (!ten_env_proxy_notify(self->c_ten_env_proxy,
                            ten_env_proxy_notify_return_result,
                            return_result_info, false, &err)) {
    ten_env_notify_return_result_info_destroy(return_result_info);
    ten_go_error_from_error(&cgo_error, &err);
  }

  ten_error_deinit(&err);
  TEN_GO_TEN_ENV_IS_ALIVE_REGION_END(self);

ten_is_close:
  return cgo_error;
}
