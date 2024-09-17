//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/binding/go/internal/common.h"
#include "include_internal/ten_runtime/binding/go/internal/json.h"
#include "include_internal/ten_runtime/binding/go/msg/msg.h"
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env.h"
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env_internal.h"
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env_return_result.h"
#include "include_internal/ten_runtime/binding/go/value/value.h"
#include "ten_utils/macro/check.h"
#include "ten_runtime/binding/go/interface/ten/msg.h"
#include "ten_runtime/binding/go/interface/ten/ten_env.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/value/value.h"

ten_go_status_t ten_go_ten_env_return_string(uintptr_t bridge_addr,
                                             int status_code,
                                             const void *detail, int detail_len,
                                             uintptr_t cmd_bridge_addr) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");

  ten_go_msg_t *cmd = ten_go_msg_reinterpret(cmd_bridge_addr);
  TEN_ASSERT(cmd && ten_go_msg_check_integrity(cmd), "Should not happen.");

  ten_go_status_t api_status;
  ten_go_status_init_with_errno(&api_status, TEN_ERRNO_OK);

  if (!detail) {
    detail = "";
    detail_len = 0;
  }

  ten_value_t *detail_value =
      ten_value_create_string_with_size(detail, detail_len);
  TEN_ASSERT(detail_value && ten_value_check_integrity(detail_value),
             "Should not happen.");

  ten_go_ten_return_status_value(self, cmd, status_code, detail_value,
                                 &api_status);

  return api_status;
}

ten_go_status_t ten_go_ten_env_return_json_bytes(uintptr_t bridge_addr,
                                                 int status_code,
                                                 const void *detail,
                                                 int detail_len,
                                                 uintptr_t cmd_bridge_addr) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");

  ten_go_msg_t *cmd = ten_go_msg_reinterpret(cmd_bridge_addr);
  TEN_ASSERT(cmd && ten_go_msg_check_integrity(cmd), "Should not happen.");

  ten_go_status_t api_status;
  ten_go_status_init_with_errno(&api_status, TEN_ERRNO_OK);

  ten_json_t *json = ten_go_json_loads(detail, detail_len, &api_status);
  if (json == NULL) {
    return api_status;
  }

  ten_value_t *detail_value = ten_value_from_json(json);
  ten_json_destroy(json);
  TEN_ASSERT(detail_value && ten_value_check_integrity(detail_value),
             "Should not happen.");

  ten_go_ten_return_status_value(self, cmd, status_code, detail_value,
                                 &api_status);

  return api_status;
}

ten_go_status_t ten_go_ten_env_return_bool(uintptr_t bridge_addr,
                                           int status_code, bool detail,
                                           uintptr_t cmd_bridge_addr) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");

  ten_go_msg_t *cmd = ten_go_msg_reinterpret(cmd_bridge_addr);
  TEN_ASSERT(cmd && ten_go_msg_check_integrity(cmd), "Should not happen.");

  ten_go_status_t api_status;
  ten_go_status_init_with_errno(&api_status, TEN_ERRNO_OK);

  ten_value_t *detail_value = ten_value_create_bool(detail);
  TEN_ASSERT(detail_value && ten_value_check_integrity(detail_value),
             "Should not happen.");

  ten_go_ten_return_status_value(self, cmd, status_code, detail_value,
                                 &api_status);

  return api_status;
}

ten_go_status_t ten_go_ten_env_return_int8(uintptr_t bridge_addr,
                                           int status_code, int8_t detail,
                                           uintptr_t cmd_bridge_addr) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");

  ten_go_msg_t *cmd = ten_go_msg_reinterpret(cmd_bridge_addr);
  TEN_ASSERT(cmd && ten_go_msg_check_integrity(cmd), "Should not happen.");

  ten_go_status_t api_status;
  ten_go_status_init_with_errno(&api_status, TEN_ERRNO_OK);

  ten_value_t *detail_value = ten_value_create_int8(detail);
  TEN_ASSERT(detail_value && ten_value_check_integrity(detail_value),
             "Should not happen.");

  ten_go_ten_return_status_value(self, cmd, status_code, detail_value,
                                 &api_status);

  return api_status;
}

ten_go_status_t ten_go_ten_env_return_int16(uintptr_t bridge_addr,
                                            int status_code, int16_t detail,
                                            uintptr_t cmd_bridge_addr) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");

  ten_go_msg_t *cmd = ten_go_msg_reinterpret(cmd_bridge_addr);
  TEN_ASSERT(cmd && ten_go_msg_check_integrity(cmd), "Should not happen.");

  ten_go_status_t api_status;
  ten_go_status_init_with_errno(&api_status, TEN_ERRNO_OK);

  ten_value_t *detail_value = ten_value_create_int16(detail);
  TEN_ASSERT(detail_value && ten_value_check_integrity(detail_value),
             "Should not happen.");

  ten_go_ten_return_status_value(self, cmd, status_code, detail_value,
                                 &api_status);

  return api_status;
}

ten_go_status_t ten_go_ten_env_return_int32(uintptr_t bridge_addr,
                                            int status_code, int32_t detail,
                                            uintptr_t cmd_bridge_addr) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");

  ten_go_msg_t *cmd = ten_go_msg_reinterpret(cmd_bridge_addr);
  TEN_ASSERT(cmd && ten_go_msg_check_integrity(cmd), "Should not happen.");

  ten_go_status_t api_status;
  ten_go_status_init_with_errno(&api_status, TEN_ERRNO_OK);

  ten_value_t *detail_value = ten_value_create_int32(detail);
  TEN_ASSERT(detail_value && ten_value_check_integrity(detail_value),
             "Should not happen.");

  ten_go_ten_return_status_value(self, cmd, status_code, detail_value,
                                 &api_status);

  return api_status;
}

ten_go_status_t ten_go_ten_env_return_int64(uintptr_t bridge_addr,
                                            int status_code, int64_t detail,
                                            uintptr_t cmd_bridge_addr) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");

  ten_go_msg_t *cmd = ten_go_msg_reinterpret(cmd_bridge_addr);
  TEN_ASSERT(cmd && ten_go_msg_check_integrity(cmd), "Should not happen.");

  ten_go_status_t api_status;
  ten_go_status_init_with_errno(&api_status, TEN_ERRNO_OK);

  ten_value_t *detail_value = ten_value_create_int64(detail);
  TEN_ASSERT(detail_value && ten_value_check_integrity(detail_value),
             "Should not happen.");

  ten_go_ten_return_status_value(self, cmd, status_code, detail_value,
                                 &api_status);

  return api_status;
}

ten_go_status_t ten_go_ten_env_return_uint8(uintptr_t bridge_addr,
                                            int status_code, uint8_t detail,
                                            uintptr_t cmd_bridge_addr) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");

  ten_go_msg_t *cmd = ten_go_msg_reinterpret(cmd_bridge_addr);
  TEN_ASSERT(cmd && ten_go_msg_check_integrity(cmd), "Should not happen.");

  ten_go_status_t api_status;
  ten_go_status_init_with_errno(&api_status, TEN_ERRNO_OK);

  ten_value_t *detail_value = ten_value_create_uint8(detail);
  TEN_ASSERT(detail_value && ten_value_check_integrity(detail_value),
             "Should not happen.");

  ten_go_ten_return_status_value(self, cmd, status_code, detail_value,
                                 &api_status);

  return api_status;
}

ten_go_status_t ten_go_ten_env_return_uint16(uintptr_t bridge_addr,
                                             int status_code, uint16_t detail,
                                             uintptr_t cmd_bridge_addr) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");

  ten_go_msg_t *cmd = ten_go_msg_reinterpret(cmd_bridge_addr);
  TEN_ASSERT(cmd && ten_go_msg_check_integrity(cmd), "Should not happen.");

  ten_go_status_t api_status;
  ten_go_status_init_with_errno(&api_status, TEN_ERRNO_OK);

  ten_value_t *detail_value = ten_value_create_uint16(detail);
  TEN_ASSERT(detail_value && ten_value_check_integrity(detail_value),
             "Should not happen.");

  ten_go_ten_return_status_value(self, cmd, status_code, detail_value,
                                 &api_status);

  return api_status;
}

ten_go_status_t ten_go_ten_env_return_uint32(uintptr_t bridge_addr,
                                             int status_code, uint32_t detail,
                                             uintptr_t cmd_bridge_addr) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");

  ten_go_msg_t *cmd = ten_go_msg_reinterpret(cmd_bridge_addr);
  TEN_ASSERT(cmd && ten_go_msg_check_integrity(cmd), "Should not happen.");

  ten_go_status_t api_status;
  ten_go_status_init_with_errno(&api_status, TEN_ERRNO_OK);

  ten_value_t *detail_value = ten_value_create_uint32(detail);
  TEN_ASSERT(detail_value && ten_value_check_integrity(detail_value),
             "Should not happen.");

  ten_go_ten_return_status_value(self, cmd, status_code, detail_value,
                                 &api_status);

  return api_status;
}

ten_go_status_t ten_go_ten_env_return_uint64(uintptr_t bridge_addr,
                                             int status_code, uint64_t detail,
                                             uintptr_t cmd_bridge_addr) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");

  ten_go_msg_t *cmd = ten_go_msg_reinterpret(cmd_bridge_addr);
  TEN_ASSERT(cmd && ten_go_msg_check_integrity(cmd), "Should not happen.");

  ten_go_status_t api_status;
  ten_go_status_init_with_errno(&api_status, TEN_ERRNO_OK);

  ten_value_t *detail_value = ten_value_create_uint64(detail);
  TEN_ASSERT(detail_value && ten_value_check_integrity(detail_value),
             "Should not happen.");

  ten_go_ten_return_status_value(self, cmd, status_code, detail_value,
                                 &api_status);

  return api_status;
}

ten_go_status_t ten_go_ten_env_return_float32(uintptr_t bridge_addr,
                                              int status_code, float detail,
                                              uintptr_t cmd_bridge_addr) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");

  ten_go_msg_t *cmd = ten_go_msg_reinterpret(cmd_bridge_addr);
  TEN_ASSERT(cmd && ten_go_msg_check_integrity(cmd), "Should not happen.");

  ten_go_status_t api_status;
  ten_go_status_init_with_errno(&api_status, TEN_ERRNO_OK);

  ten_value_t *detail_value = ten_value_create_float32(detail);
  TEN_ASSERT(detail_value && ten_value_check_integrity(detail_value),
             "Should not happen.");

  ten_go_ten_return_status_value(self, cmd, status_code, detail_value,
                                 &api_status);

  return api_status;
}

ten_go_status_t ten_go_ten_env_return_float64(uintptr_t bridge_addr,
                                              int status_code, double detail,
                                              uintptr_t cmd_bridge_addr) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");

  ten_go_msg_t *cmd = ten_go_msg_reinterpret(cmd_bridge_addr);
  TEN_ASSERT(cmd && ten_go_msg_check_integrity(cmd), "Should not happen.");

  ten_go_status_t api_status;
  ten_go_status_init_with_errno(&api_status, TEN_ERRNO_OK);

  ten_value_t *detail_value = ten_value_create_float64(detail);
  TEN_ASSERT(detail_value && ten_value_check_integrity(detail_value),
             "Should not happen.");

  ten_go_ten_return_status_value(self, cmd, status_code, detail_value,
                                 &api_status);

  return api_status;
}

ten_go_status_t ten_go_ten_env_return_buf(uintptr_t bridge_addr,
                                          int status_code, void *detail,
                                          int detail_len,
                                          uintptr_t cmd_bridge_addr) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");
  TEN_ASSERT(detail, "Should not happen");

  ten_go_msg_t *cmd = ten_go_msg_reinterpret(cmd_bridge_addr);
  TEN_ASSERT(cmd && ten_go_msg_check_integrity(cmd), "Should not happen.");

  ten_go_status_t api_status;
  ten_go_status_init_with_errno(&api_status, TEN_ERRNO_OK);

  ten_value_t *detail_value = ten_go_ten_value_create_buf(detail, detail_len);
  TEN_ASSERT(detail_value && ten_value_check_integrity(detail_value),
             "Should not happen.");

  ten_go_ten_return_status_value(self, cmd, status_code, detail_value,
                                 &api_status);

  return api_status;
}
