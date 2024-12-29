//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "ten_runtime/binding/go/interface/ten/cmd.h"

#include <stdlib.h>

#include "include_internal/ten_runtime/binding/go/internal/common.h"
#include "include_internal/ten_runtime/binding/go/msg/msg.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd/cmd.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd/custom/cmd.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "ten_runtime/binding/go/interface/ten/common.h"
#include "ten_runtime/binding/go/interface/ten/msg.h"
#include "ten_runtime/common/status_code.h"
#include "ten_runtime/msg/cmd_result/cmd_result.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/lib/smart_ptr.h"
#include "ten_utils/macro/check.h"

ten_go_handle_t tenGoCreateCmdResult(uintptr_t);

ten_go_error_t ten_go_cmd_create_cmd(const void *name, int name_len,
                                     uintptr_t *bridge) {
  TEN_ASSERT(name && name_len > 0, "Should not happen.");

  ten_go_error_t cgo_error;
  ten_go_error_init_with_errno(&cgo_error, TEN_ERRNO_OK);

  ten_shared_ptr_t *cmd =
      ten_cmd_custom_create_with_name_len(name, name_len, NULL);
  TEN_ASSERT(cmd && ten_cmd_check_integrity(cmd), "Should not happen.");

  ten_go_msg_t *msg_bridge = ten_go_msg_create(cmd);
  TEN_ASSERT(msg_bridge, "Should not happen.");

  *bridge = (uintptr_t)msg_bridge;
  ten_shared_ptr_destroy(cmd);

  return cgo_error;
}

uintptr_t ten_go_cmd_create_cmd_result(int status_code) {
  TEN_ASSERT(
      status_code == TEN_STATUS_CODE_OK || status_code == TEN_STATUS_CODE_ERROR,
      "Should not happen.");

  TEN_STATUS_CODE code = (TEN_STATUS_CODE)status_code;

  ten_shared_ptr_t *c_cmd = ten_cmd_result_create(code);
  TEN_ASSERT(c_cmd, "Should not happen.");

  ten_go_msg_t *msg_bridge = ten_go_msg_create(c_cmd);
  TEN_ASSERT(msg_bridge, "Should not happen.");

  ten_shared_ptr_destroy(c_cmd);

  return (uintptr_t)msg_bridge;
}

int ten_go_cmd_result_get_status_code(uintptr_t bridge_addr) {
  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");

  return ten_cmd_result_get_status_code(ten_go_msg_c_msg(self));
}

ten_go_error_t ten_go_cmd_result_set_final(uintptr_t bridge_addr,
                                           bool is_final) {
  TEN_ASSERT(bridge_addr, "Invalid argument.");

  ten_go_msg_t *msg_bridge = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(msg_bridge && ten_go_msg_check_integrity(msg_bridge),
             "Should not happen.");

  ten_shared_ptr_t *c_cmd = ten_go_msg_c_msg(msg_bridge);
  TEN_ASSERT(c_cmd, "Should not happen.");

  ten_go_error_t cgo_error;
  ten_go_error_init_with_errno(&cgo_error, TEN_ERRNO_OK);

  ten_error_t err;
  ten_error_init(&err);

  bool success =
      ten_cmd_result_set_final(ten_go_msg_c_msg(msg_bridge), is_final, &err);

  if (!ten_error_is_success(&err)) {
    TEN_ASSERT(!success, "Should not happen.");
    ten_go_error_set(&cgo_error, ten_error_errno(&err), ten_error_errmsg(&err));
  }

  ten_error_deinit(&err);
  return cgo_error;
}

ten_go_error_t ten_go_cmd_result_is_final(uintptr_t bridge_addr,
                                          bool *is_final) {
  TEN_ASSERT(bridge_addr && is_final, "Invalid argument.");

  ten_go_msg_t *msg_bridge = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(msg_bridge && ten_go_msg_check_integrity(msg_bridge),
             "Should not happen.");

  ten_shared_ptr_t *c_cmd = ten_go_msg_c_msg(msg_bridge);
  TEN_ASSERT(c_cmd, "Should not happen.");

  ten_go_error_t cgo_error;
  ten_go_error_init_with_errno(&cgo_error, TEN_ERRNO_OK);

  ten_error_t err;
  ten_error_init(&err);

  bool is_final_ = ten_cmd_result_is_final(ten_go_msg_c_msg(msg_bridge), &err);

  if (!ten_error_is_success(&err)) {
    ten_go_error_set(&cgo_error, ten_error_errno(&err), ten_error_errmsg(&err));
  } else {
    *is_final = is_final_;
  }

  ten_error_deinit(&err);
  return cgo_error;
}

ten_go_error_t ten_go_cmd_result_is_completed(uintptr_t bridge_addr,
                                              bool *is_completed) {
  TEN_ASSERT(bridge_addr && is_completed, "Invalid argument.");

  ten_go_msg_t *msg_bridge = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(msg_bridge && ten_go_msg_check_integrity(msg_bridge),
             "Should not happen.");

  ten_shared_ptr_t *c_cmd = ten_go_msg_c_msg(msg_bridge);
  TEN_ASSERT(c_cmd, "Should not happen.");

  ten_go_error_t cgo_error;
  ten_go_error_init_with_errno(&cgo_error, TEN_ERRNO_OK);

  ten_error_t err;
  ten_error_init(&err);

  bool is_completed_ =
      ten_cmd_result_is_completed(ten_go_msg_c_msg(msg_bridge), &err);

  if (!ten_error_is_success(&err)) {
    ten_go_error_set(&cgo_error, ten_error_errno(&err), ten_error_errmsg(&err));
  } else {
    *is_completed = is_completed_;
  }

  ten_error_deinit(&err);
  return cgo_error;
}
