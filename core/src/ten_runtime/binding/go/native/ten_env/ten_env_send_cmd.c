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
#include "ten_runtime/binding/go/interface/ten/common.h"
#include "ten_runtime/binding/go/interface/ten/msg.h"
#include "ten_runtime/binding/go/interface/ten/ten_env.h"
#include "ten_runtime/ten_env/internal/send.h"
#include "ten_runtime/ten_env_proxy/ten_env_proxy.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"

typedef struct ten_env_notify_send_cmd_info_t {
  ten_shared_ptr_t *c_cmd;
  ten_go_handle_t handler_id;
  bool is_ex;
} ten_env_notify_send_cmd_info_t;

static ten_env_notify_send_cmd_info_t *ten_env_notify_send_cmd_info_create(
    ten_shared_ptr_t *c_cmd, ten_go_handle_t handler_id, bool is_ex) {
  TEN_ASSERT(c_cmd, "Invalid argument.");

  ten_env_notify_send_cmd_info_t *info =
      TEN_MALLOC(sizeof(ten_env_notify_send_cmd_info_t));
  TEN_ASSERT(info, "Failed to allocate memory.");

  info->c_cmd = c_cmd;
  info->handler_id = handler_id;
  info->is_ex = is_ex;

  return info;
}

static void ten_env_notify_send_cmd_info_destroy(
    ten_env_notify_send_cmd_info_t *info) {
  TEN_ASSERT(info, "Invalid argument.");

  if (info->c_cmd) {
    ten_shared_ptr_destroy(info->c_cmd);
    info->c_cmd = NULL;
  }

  info->handler_id = 0;

  TEN_FREE(info);
}

static void ten_env_proxy_notify_send_cmd(ten_env_t *ten_env, void *user_data) {
  TEN_ASSERT(user_data, "Invalid argument.");
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_env_notify_send_cmd_info_t *notify_info = user_data;

  ten_error_t err;
  ten_error_init(&err);

  ten_env_send_cmd_func_t send_cmd_func = NULL;
  if (notify_info->is_ex) {
    send_cmd_func = ten_env_send_cmd_ex;
  } else {
    send_cmd_func = ten_env_send_cmd;
  }

  bool res = false;
  if (notify_info->handler_id == TEN_GO_NO_RESPONSE_HANDLER) {
    res = send_cmd_func(ten_env, notify_info->c_cmd, NULL, NULL, &err);
  } else {
    ten_go_callback_info_t *info =
        ten_go_callback_info_create(notify_info->handler_id);
    res = send_cmd_func(ten_env, notify_info->c_cmd, proxy_send_xxx_callback,
                        info, &err);

    if (!res) {
      ten_go_callback_info_destroy(info);
    }
  }

  if (!res) {
    if (notify_info->handler_id != TEN_GO_NO_RESPONSE_HANDLER) {
      ten_go_ten_env_t *ten_env_bridge = ten_go_ten_env_wrap(ten_env);

      TEN_ASSERT(err.err_no != TEN_ERRNO_OK, "Should not happen.");
      ten_go_error_t cgo_error;
      ten_go_error_from_error(&cgo_error, &err);

      tenGoOnCmdResult(ten_env_bridge->bridge.go_instance, 0,
                       notify_info->handler_id, cgo_error);
    }
  }

  ten_error_deinit(&err);

  ten_env_notify_send_cmd_info_destroy(notify_info);
}

ten_go_error_t ten_go_ten_env_send_cmd(uintptr_t bridge_addr,
                                       uintptr_t cmd_bridge_addr,
                                       ten_go_handle_t handler_id, bool is_ex) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");

  ten_go_msg_t *cmd = ten_go_msg_reinterpret(cmd_bridge_addr);
  TEN_ASSERT(cmd && ten_go_msg_check_integrity(cmd), "Should not happen.");
  TEN_ASSERT(ten_go_msg_c_msg(cmd), "Should not happen.");

  ten_go_error_t cgo_error;
  ten_go_error_init_with_errno(&cgo_error, TEN_ERRNO_OK);

  TEN_GO_TEN_ENV_IS_ALIVE_REGION_BEGIN(self, {
    ten_go_error_init_with_errno(&cgo_error, TEN_ERRNO_TEN_IS_CLOSED);
  });

  ten_error_t err;
  ten_error_init(&err);

  ten_env_notify_send_cmd_info_t *notify_info =
      ten_env_notify_send_cmd_info_create(
          ten_go_msg_move_c_msg(cmd),
          handler_id <= 0 ? TEN_GO_NO_RESPONSE_HANDLER : handler_id, is_ex);

  if (!ten_env_proxy_notify(self->c_ten_env_proxy,
                            ten_env_proxy_notify_send_cmd, notify_info, false,
                            &err)) {
    ten_env_notify_send_cmd_info_destroy(notify_info);
    ten_go_error_from_error(&cgo_error, &err);
  }

  TEN_GO_TEN_ENV_IS_ALIVE_REGION_END(self);
  ten_error_deinit(&err);

ten_is_close:
  return cgo_error;
}
