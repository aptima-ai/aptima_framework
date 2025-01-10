//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_runtime/binding/go/interface/aptima/cmd.h"

#include <stdlib.h>

#include "include_internal/axis_runtime/binding/go/internal/common.h"
#include "include_internal/axis_runtime/binding/go/msg/msg.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/cmd.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/custom/cmd.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "axis_runtime/binding/go/interface/aptima/common.h"
#include "axis_runtime/binding/go/interface/aptima/msg.h"
#include "axis_runtime/common/status_code.h"
#include "axis_runtime/msg/cmd_result/cmd_result.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/macro/check.h"

axis_go_handle_t tenGoCreateCmdResult(uintptr_t);

axis_go_error_t axis_go_cmd_create_cmd(const void *name, int name_len,
                                     uintptr_t *bridge) {
  axis_ASSERT(name && name_len > 0, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_shared_ptr_t *cmd =
      axis_cmd_custom_create_with_name_len(name, name_len, NULL);
  axis_ASSERT(cmd && axis_cmd_check_integrity(cmd), "Should not happen.");

  axis_go_msg_t *msg_bridge = axis_go_msg_create(cmd);
  axis_ASSERT(msg_bridge, "Should not happen.");

  *bridge = (uintptr_t)msg_bridge;
  axis_shared_ptr_destroy(cmd);

  return cgo_error;
}

uintptr_t axis_go_cmd_create_cmd_result(int status_code) {
  axis_ASSERT(
      status_code == axis_STATUS_CODE_OK || status_code == axis_STATUS_CODE_ERROR,
      "Should not happen.");

  axis_STATUS_CODE code = (axis_STATUS_CODE)status_code;

  axis_shared_ptr_t *c_cmd = axis_cmd_result_create(code);
  axis_ASSERT(c_cmd, "Should not happen.");

  axis_go_msg_t *msg_bridge = axis_go_msg_create(c_cmd);
  axis_ASSERT(msg_bridge, "Should not happen.");

  axis_shared_ptr_destroy(c_cmd);

  return (uintptr_t)msg_bridge;
}

int axis_go_cmd_result_get_status_code(uintptr_t bridge_addr) {
  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");

  return axis_cmd_result_get_status_code(axis_go_msg_c_msg(self));
}

axis_go_error_t axis_go_cmd_result_set_final(uintptr_t bridge_addr,
                                           bool is_final) {
  axis_ASSERT(bridge_addr, "Invalid argument.");

  axis_go_msg_t *msg_bridge = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(msg_bridge && axis_go_msg_check_integrity(msg_bridge),
             "Should not happen.");

  axis_shared_ptr_t *c_cmd = axis_go_msg_c_msg(msg_bridge);
  axis_ASSERT(c_cmd, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_error_t err;
  axis_error_init(&err);

  bool success =
      axis_cmd_result_set_final(axis_go_msg_c_msg(msg_bridge), is_final, &err);

  if (!axis_error_is_success(&err)) {
    axis_ASSERT(!success, "Should not happen.");
    axis_go_error_set(&cgo_error, axis_error_errno(&err), axis_error_errmsg(&err));
  }

  axis_error_deinit(&err);
  return cgo_error;
}

axis_go_error_t axis_go_cmd_result_is_final(uintptr_t bridge_addr,
                                          bool *is_final) {
  axis_ASSERT(bridge_addr && is_final, "Invalid argument.");

  axis_go_msg_t *msg_bridge = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(msg_bridge && axis_go_msg_check_integrity(msg_bridge),
             "Should not happen.");

  axis_shared_ptr_t *c_cmd = axis_go_msg_c_msg(msg_bridge);
  axis_ASSERT(c_cmd, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_error_t err;
  axis_error_init(&err);

  bool is_final_ = axis_cmd_result_is_final(axis_go_msg_c_msg(msg_bridge), &err);

  if (!axis_error_is_success(&err)) {
    axis_go_error_set(&cgo_error, axis_error_errno(&err), axis_error_errmsg(&err));
  } else {
    *is_final = is_final_;
  }

  axis_error_deinit(&err);
  return cgo_error;
}

axis_go_error_t axis_go_cmd_result_is_completed(uintptr_t bridge_addr,
                                              bool *is_completed) {
  axis_ASSERT(bridge_addr && is_completed, "Invalid argument.");

  axis_go_msg_t *msg_bridge = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(msg_bridge && axis_go_msg_check_integrity(msg_bridge),
             "Should not happen.");

  axis_shared_ptr_t *c_cmd = axis_go_msg_c_msg(msg_bridge);
  axis_ASSERT(c_cmd, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_error_t err;
  axis_error_init(&err);

  bool is_completed_ =
      axis_cmd_result_is_completed(axis_go_msg_c_msg(msg_bridge), &err);

  if (!axis_error_is_success(&err)) {
    axis_go_error_set(&cgo_error, axis_error_errno(&err), axis_error_errmsg(&err));
  } else {
    *is_completed = is_completed_;
  }

  axis_error_deinit(&err);
  return cgo_error;
}
