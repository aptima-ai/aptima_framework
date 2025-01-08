//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <stdlib.h>

#include "include_internal/axis_runtime/binding/go/internal/common.h"
#include "include_internal/axis_runtime/binding/go/msg/msg.h"
#include "include_internal/axis_runtime/binding/go/axis_env/axis_env.h"
#include "include_internal/axis_runtime/binding/go/axis_env/axis_env_internal.h"
#include "axis_runtime/binding/go/interface/ten/common.h"
#include "axis_runtime/binding/go/interface/ten/msg.h"
#include "axis_runtime/binding/go/interface/ten/axis_env.h"
#include "axis_runtime/axis_env/internal/send.h"
#include "axis_runtime/axis_env_proxy/axis_env_proxy.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/macro/check.h"

typedef struct axis_env_notify_send_cmd_ctx_t {
  axis_shared_ptr_t *c_cmd;
  axis_go_handle_t handler_id;
  bool is_ex;
} axis_env_notify_send_cmd_ctx_t;

static axis_env_notify_send_cmd_ctx_t *axis_env_notify_send_cmd_ctx_create(
    axis_shared_ptr_t *c_cmd, axis_go_handle_t handler_id, bool is_ex) {
  axis_ASSERT(c_cmd, "Invalid argument.");

  axis_env_notify_send_cmd_ctx_t *ctx =
      axis_MALLOC(sizeof(axis_env_notify_send_cmd_ctx_t));
  axis_ASSERT(ctx, "Failed to allocate memory.");

  ctx->c_cmd = c_cmd;
  ctx->handler_id = handler_id;
  ctx->is_ex = is_ex;

  return ctx;
}

static void axis_env_notify_send_cmd_ctx_destroy(
    axis_env_notify_send_cmd_ctx_t *ctx) {
  axis_ASSERT(ctx, "Invalid argument.");

  if (ctx->c_cmd) {
    axis_shared_ptr_destroy(ctx->c_cmd);
    ctx->c_cmd = NULL;
  }

  ctx->handler_id = 0;

  axis_FREE(ctx);
}

static void axis_env_proxy_notify_send_cmd(axis_env_t *axis_env, void *user_data) {
  axis_ASSERT(user_data, "Invalid argument.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_env_notify_send_cmd_ctx_t *notify_info = user_data;
  axis_ASSERT(notify_info, "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  axis_env_send_cmd_func_t send_cmd_func = NULL;
  if (notify_info->is_ex) {
    send_cmd_func = axis_env_send_cmd_ex;
  } else {
    send_cmd_func = axis_env_send_cmd;
  }

  bool res = false;
  if (notify_info->handler_id == axis_GO_NO_RESPONSE_HANDLER) {
    res = send_cmd_func(axis_env, notify_info->c_cmd, NULL, NULL, &err);
  } else {
    axis_go_callback_ctx_t *ctx =
        axis_go_callback_ctx_create(notify_info->handler_id);
    res = send_cmd_func(axis_env, notify_info->c_cmd, proxy_send_xxx_callback,
                        ctx, &err);

    if (!res) {
      axis_go_callback_ctx_destroy(ctx);
    }
  }

  if (!res) {
    if (notify_info->handler_id != axis_GO_NO_RESPONSE_HANDLER) {
      axis_go_axis_env_t *axis_env_bridge = axis_go_axis_env_wrap(axis_env);

      axis_ASSERT(err.err_no != axis_ERRNO_OK, "Should not happen.");
      axis_go_error_t cgo_error;
      axis_go_error_from_error(&cgo_error, &err);

      tenGoOnCmdResult(axis_env_bridge->bridge.go_instance, 0,
                       notify_info->handler_id, cgo_error);
    }
  }

  axis_error_deinit(&err);

  axis_env_notify_send_cmd_ctx_destroy(notify_info);
}

axis_go_error_t axis_go_axis_env_send_cmd(uintptr_t bridge_addr,
                                       uintptr_t cmd_bridge_addr,
                                       axis_go_handle_t handler_id, bool is_ex) {
  axis_go_axis_env_t *self = axis_go_axis_env_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");

  axis_go_msg_t *cmd = axis_go_msg_reinterpret(cmd_bridge_addr);
  axis_ASSERT(cmd && axis_go_msg_check_integrity(cmd), "Should not happen.");
  axis_ASSERT(axis_go_msg_c_msg(cmd), "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_GO_axis_ENV_IS_ALIVE_REGION_BEGIN(self, {
    axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_axis_IS_CLOSED);
  });

  axis_error_t err;
  axis_error_init(&err);

  axis_env_notify_send_cmd_ctx_t *notify_info =
      axis_env_notify_send_cmd_ctx_create(
          axis_go_msg_move_c_msg(cmd),
          handler_id <= 0 ? axis_GO_NO_RESPONSE_HANDLER : handler_id, is_ex);

  if (!axis_env_proxy_notify(self->c_axis_env_proxy,
                            axis_env_proxy_notify_send_cmd, notify_info, false,
                            &err)) {
    axis_env_notify_send_cmd_ctx_destroy(notify_info);
    axis_go_error_from_error(&cgo_error, &err);
  }

  axis_GO_axis_ENV_IS_ALIVE_REGION_END(self);
  axis_error_deinit(&err);

axis_is_close:
  return cgo_error;
}
