//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/binding/go/axis_env/axis_env_internal.h"

#include <stdint.h>

#include "include_internal/axis_runtime/binding/go/internal/common.h"
#include "include_internal/axis_runtime/binding/go/msg/msg.h"
#include "include_internal/axis_runtime/binding/go/axis_env/axis_env.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd_base.h"
#include "axis_runtime/binding/go/interface/ten/common.h"
#include "axis_runtime/binding/go/interface/ten/msg.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/macro/check.h"

axis_go_callback_ctx_t *axis_go_callback_ctx_create(axis_go_handle_t handler_id) {
  axis_go_callback_ctx_t *ctx =
      (axis_go_callback_ctx_t *)axis_MALLOC(sizeof(axis_go_callback_ctx_t));
  axis_ASSERT(ctx, "Failed to allocate memory.");

  ctx->callback_id = handler_id;

  return ctx;
}

void axis_go_callback_ctx_destroy(axis_go_callback_ctx_t *self) {
  axis_ASSERT(self, "Should not happen.");

  axis_FREE(self);
}

void proxy_send_xxx_callback(axis_env_t *axis_env, axis_shared_ptr_t *cmd_result,
                             void *callback_info, axis_error_t *err) {
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");
  axis_ASSERT(cmd_result && axis_cmd_base_check_integrity(cmd_result),
             "Should not happen.");
  axis_ASSERT(callback_info, "Should not happen.");

  axis_go_axis_env_t *axis_env_bridge = axis_go_axis_env_wrap(axis_env);
  axis_go_handle_t handler_id =
      ((axis_go_callback_ctx_t *)callback_info)->callback_id;

  // Same as Extension::OnCmd, the GO cmd result is only used for the GO
  // extension, so it can be created in GO world. We do not need to call GO
  // function to create the GO cmd result in C.
  axis_go_msg_t *cmd_result_bridge = axis_go_msg_create(cmd_result);
  uintptr_t cmd_result_bridge_addr = (uintptr_t)cmd_result_bridge;

  axis_go_error_t cgo_error;

  if (err) {
    axis_go_error_from_error(&cgo_error, err);
  } else {
    axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);
  }

  tenGoOnCmdResult(axis_env_bridge->bridge.go_instance, cmd_result_bridge_addr,
                   handler_id, cgo_error);

  axis_go_callback_ctx_destroy(callback_info);
}
