//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env_internal.h"

#include <stdint.h>

#include "include_internal/ten_runtime/binding/go/internal/common.h"
#include "include_internal/ten_runtime/binding/go/msg/msg.h"
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd_base.h"
#include "ten_runtime/binding/go/interface/aptima/common.h"
#include "ten_runtime/binding/go/interface/aptima/msg.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/macro/check.h"

ten_go_callback_ctx_t *ten_go_callback_ctx_create(ten_go_handle_t handler_id) {
  ten_go_callback_ctx_t *ctx =
      (ten_go_callback_ctx_t *)TEN_MALLOC(sizeof(ten_go_callback_ctx_t));
  TEN_ASSERT(ctx, "Failed to allocate memory.");

  ctx->callback_id = handler_id;

  return ctx;
}

void ten_go_callback_ctx_destroy(ten_go_callback_ctx_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  TEN_FREE(self);
}

void proxy_send_xxx_callback(ten_env_t *ten_env, ten_shared_ptr_t *cmd_result,
                             void *callback_info, ten_error_t *err) {
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");
  TEN_ASSERT(cmd_result && ten_cmd_base_check_integrity(cmd_result),
             "Should not happen.");
  TEN_ASSERT(callback_info, "Should not happen.");

  ten_go_ten_env_t *ten_env_bridge = ten_go_ten_env_wrap(ten_env);
  ten_go_handle_t handler_id =
      ((ten_go_callback_ctx_t *)callback_info)->callback_id;

  // Same as Extension::OnCmd, the GO cmd result is only used for the GO
  // extension, so it can be created in GO world. We do not need to call GO
  // function to create the GO cmd result in C.
  ten_go_msg_t *cmd_result_bridge = ten_go_msg_create(cmd_result);
  uintptr_t cmd_result_bridge_addr = (uintptr_t)cmd_result_bridge;

  ten_go_error_t cgo_error;

  if (err) {
    ten_go_error_from_error(&cgo_error, err);
  } else {
    ten_go_error_init_with_errno(&cgo_error, TEN_ERRNO_OK);
  }

  tenGoOnCmdResult(ten_env_bridge->bridge.go_instance, cmd_result_bridge_addr,
                   handler_id, cgo_error);

  ten_go_callback_ctx_destroy(callback_info);
}
