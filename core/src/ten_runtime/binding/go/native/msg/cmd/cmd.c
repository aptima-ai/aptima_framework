//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "ten_runtime/binding/go/interface/ten/cmd.h"

#include <stdlib.h>

#include "include_internal/ten_runtime/binding/go/internal/common.h"
#include "include_internal/ten_runtime/binding/go/internal/json.h"
#include "include_internal/ten_runtime/binding/go/msg/msg.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd/cmd.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd/custom/cmd.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "ten_utils/macro/check.h"
#include "ten_runtime/binding/go/interface/ten/common.h"
#include "ten_runtime/binding/go/interface/ten/msg.h"
#include "ten_runtime/common/status_code.h"
#include "ten_runtime/msg/cmd_result/cmd_result.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/lib/smart_ptr.h"

ten_go_handle_t tenGoCreateCmdResult(uintptr_t);

ten_go_status_t ten_go_cmd_create_cmd(const void *cmd_name, int cmd_name_len,
                                      uintptr_t *bridge) {
  TEN_ASSERT(cmd_name && cmd_name_len > 0, "Should not happen.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_shared_ptr_t *cmd = ten_cmd_custom_create_empty();
  TEN_ASSERT(cmd && ten_cmd_check_integrity(cmd), "Should not happen.");

  ten_msg_set_name_with_size(cmd, cmd_name, cmd_name_len, NULL);

  ten_go_msg_t *msg_bridge = ten_go_msg_create(cmd);
  TEN_ASSERT(msg_bridge, "Should not happen.");

  *bridge = (uintptr_t)msg_bridge;
  ten_shared_ptr_destroy(cmd);

  return status;
}

ten_go_status_t ten_go_cmd_create_cmd_from_json(const void *json_bytes,
                                                int json_bytes_len,
                                                uintptr_t *bridge) {
  TEN_ASSERT(json_bytes && json_bytes_len > 0, "Should not happen.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_json_t *json = ten_go_json_loads(json_bytes, json_bytes_len, &status);
  if (!json) {
    return status;
  }

  ten_error_t err;
  ten_error_init(&err);

  ten_cmd_t *raw_cmd = ten_raw_cmd_custom_create_from_json(json, &err);
  ten_shared_ptr_t *cmd = ten_shared_ptr_create(raw_cmd, ten_raw_msg_destroy);

  ten_json_destroy(json);
  if (cmd == NULL) {
    ten_go_status_from_error(&status, &err);
    goto done;
  }

  ten_go_msg_t *cmd_bridge = ten_go_msg_create(cmd);
  TEN_ASSERT(cmd_bridge, "Should not happen.");

  *bridge = (uintptr_t)cmd_bridge;
  ten_shared_ptr_destroy(cmd);

done:
  ten_error_deinit(&err);
  return status;
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
