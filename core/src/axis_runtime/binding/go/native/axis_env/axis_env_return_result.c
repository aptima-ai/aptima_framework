//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <stdlib.h>

#include "include_internal/axis_runtime/binding/go/internal/common.h"
#include "include_internal/axis_runtime/binding/go/msg/msg.h"
#include "include_internal/axis_runtime/binding/go/axis_env/axis_env.h"
#include "include_internal/axis_runtime/binding/go/axis_env/axis_env_internal.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "axis_runtime/binding/go/interface/aptima/common.h"
#include "axis_runtime/binding/go/interface/aptima/msg.h"
#include "axis_runtime/binding/go/interface/aptima/axis_env.h"
#include "axis_runtime/common/errno.h"
#include "axis_runtime/axis_env/internal/return.h"
#include "axis_runtime/axis_env_proxy/axis_env_proxy.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/macro/check.h"

typedef struct axis_env_notify_return_result_ctx_t {
  axis_shared_ptr_t *c_cmd;
  axis_shared_ptr_t *c_target_cmd;
  axis_go_handle_t handler_id;
} axis_env_notify_return_result_ctx_t;

static axis_env_notify_return_result_ctx_t *
axis_env_notify_return_result_ctx_create(axis_shared_ptr_t *c_cmd,
                                        axis_shared_ptr_t *c_target_cmd,
                                        axis_go_handle_t handler_id) {
  axis_ASSERT(c_cmd, "Invalid argument.");

  axis_env_notify_return_result_ctx_t *ctx =
      axis_MALLOC(sizeof(axis_env_notify_return_result_ctx_t));
  axis_ASSERT(ctx, "Failed to allocate memory.");

  ctx->c_cmd = c_cmd;
  ctx->c_target_cmd = c_target_cmd;
  ctx->handler_id = handler_id;

  return ctx;
}

static void axis_env_notify_return_result_ctx_destroy(
    axis_env_notify_return_result_ctx_t *ctx) {
  axis_ASSERT(ctx, "Invalid argument.");

  if (ctx->c_cmd) {
    axis_shared_ptr_destroy(ctx->c_cmd);
    ctx->c_cmd = NULL;
  }

  if (ctx->c_target_cmd) {
    axis_shared_ptr_destroy(ctx->c_target_cmd);
    ctx->c_target_cmd = NULL;
  }

  ctx->handler_id = 0;

  axis_FREE(ctx);
}

static void proxy_handle_return_error(axis_env_t *axis_env, void *user_data,
                                      axis_error_t *err) {
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_go_callback_ctx_t *callback_info = user_data;
  axis_ASSERT(callback_info, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  if (err) {
    axis_go_error_from_error(&cgo_error, err);
  }

  axis_ASSERT(callback_info->callback_id != axis_GO_NO_RESPONSE_HANDLER,
             "Should not happen.");

  axis_go_axis_env_t *axis_env_bridge = axis_go_axis_env_wrap(axis_env);

  tenGoOnError(axis_env_bridge->bridge.go_instance, callback_info->callback_id,
               cgo_error);

  axis_go_callback_ctx_destroy(callback_info);
}

static void axis_env_proxy_notify_return_result(axis_env_t *axis_env,
                                               void *user_data) {
  axis_ASSERT(user_data, "Invalid argument.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_env_notify_return_result_ctx_t *ctx = user_data;
  axis_ASSERT(ctx, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_error_t err;
  axis_error_init(&err);

  bool rc = false;
  if (ctx->handler_id == axis_GO_NO_RESPONSE_HANDLER) {
    if (ctx->c_target_cmd) {
      rc = axis_env_return_result(axis_env, ctx->c_cmd, ctx->c_target_cmd, NULL,
                                 NULL, &err);
      axis_ASSERT(rc, "Should not happen.");
    } else {
      rc =
          axis_env_return_result_directly(axis_env, ctx->c_cmd, NULL, NULL, &err);
      axis_ASSERT(rc, "Should not happen.");
    }
  } else {
    axis_go_callback_ctx_t *callback_info =
        axis_go_callback_ctx_create(ctx->handler_id);
    if (ctx->c_target_cmd) {
      rc =
          axis_env_return_result(axis_env, ctx->c_cmd, ctx->c_target_cmd,
                                proxy_handle_return_error, callback_info, &err);
    } else {
      rc = axis_env_return_result_directly(
          axis_env, ctx->c_cmd, proxy_handle_return_error, callback_info, &err);
    }

    if (!rc) {
      // Prepare error information to pass to Go.
      axis_go_error_from_error(&cgo_error, &err);

      axis_go_axis_env_t *axis_env_bridge = axis_go_axis_env_wrap(axis_env);

      tenGoOnError(axis_env_bridge->bridge.go_instance, ctx->handler_id,
                   cgo_error);

      axis_go_callback_ctx_destroy(callback_info);
    }
  }

  axis_error_deinit(&err);

  axis_env_notify_return_result_ctx_destroy(ctx);
}

axis_go_error_t axis_go_axis_env_return_result(uintptr_t bridge_addr,
                                            uintptr_t cmd_result_bridge_addr,
                                            uintptr_t cmd_bridge_addr,
                                            axis_go_handle_t handler_id) {
  axis_go_axis_env_t *self = axis_go_axis_env_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");

  axis_go_msg_t *cmd = axis_go_msg_reinterpret(cmd_bridge_addr);
  axis_ASSERT(cmd && axis_go_msg_check_integrity(cmd), "Should not happen.");
  axis_ASSERT(axis_go_msg_c_msg(cmd), "Should not happen.");

  axis_go_msg_t *cmd_result = axis_go_msg_reinterpret(cmd_result_bridge_addr);
  axis_ASSERT(cmd_result && axis_go_msg_check_integrity(cmd_result),
             "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);
  axis_GO_axis_ENV_IS_ALIVE_REGION_BEGIN(
      self, { axis_go_error_set_errno(&cgo_error, axis_ERRNO_axis_IS_CLOSED); });

  axis_error_t err;
  axis_error_init(&err);

  axis_env_notify_return_result_ctx_t *return_result_info =
      axis_env_notify_return_result_ctx_create(
          axis_go_msg_move_c_msg(cmd_result), axis_go_msg_move_c_msg(cmd),
          handler_id <= 0 ? axis_GO_NO_RESPONSE_HANDLER : handler_id);

  if (!axis_env_proxy_notify(self->c_axis_env_proxy,
                            axis_env_proxy_notify_return_result,
                            return_result_info, false, &err)) {
    axis_env_notify_return_result_ctx_destroy(return_result_info);
    axis_go_error_from_error(&cgo_error, &err);
  }

  axis_error_deinit(&err);
  axis_GO_axis_ENV_IS_ALIVE_REGION_END(self);

axis_is_close:
  return cgo_error;
}

axis_go_error_t axis_go_axis_env_return_result_directly(
    uintptr_t bridge_addr, uintptr_t cmd_result_bridge_addr,
    axis_go_handle_t handler_id) {
  axis_go_axis_env_t *self = axis_go_axis_env_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");

  axis_go_msg_t *cmd_result = axis_go_msg_reinterpret(cmd_result_bridge_addr);
  axis_ASSERT(cmd_result && axis_go_msg_check_integrity(cmd_result),
             "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_GO_axis_ENV_IS_ALIVE_REGION_BEGIN(
      self, { axis_go_error_set_errno(&cgo_error, axis_ERRNO_axis_IS_CLOSED); });
  axis_error_t err;
  axis_error_init(&err);

  axis_env_notify_return_result_ctx_t *return_result_info =
      axis_env_notify_return_result_ctx_create(
          axis_go_msg_move_c_msg(cmd_result), NULL,
          handler_id <= 0 ? axis_GO_NO_RESPONSE_HANDLER : handler_id);

  if (!axis_env_proxy_notify(self->c_axis_env_proxy,
                            axis_env_proxy_notify_return_result,
                            return_result_info, false, &err)) {
    axis_env_notify_return_result_ctx_destroy(return_result_info);
    axis_go_error_from_error(&cgo_error, &err);
  }

  axis_error_deinit(&err);
  axis_GO_axis_ENV_IS_ALIVE_REGION_END(self);

axis_is_close:
  return cgo_error;
}
