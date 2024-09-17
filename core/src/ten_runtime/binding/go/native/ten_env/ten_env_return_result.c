//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env_return_result.h"

#include <stdlib.h>

#include "include_internal/ten_runtime/binding/go/internal/common.h"
#include "include_internal/ten_runtime/binding/go/msg/msg.h"
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env.h"
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env_internal.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "ten_utils/macro/check.h"
#include "ten_runtime/binding/go/interface/ten/common.h"
#include "ten_runtime/binding/go/interface/ten/msg.h"
#include "ten_runtime/binding/go/interface/ten/ten_env.h"
#include "ten_runtime/common/errno.h"
#include "ten_runtime/common/status_code.h"
#include "ten_runtime/msg/cmd_result/cmd_result.h"
#include "ten_runtime/ten_env/internal/return.h"
#include "ten_runtime/ten_env_proxy/ten_env_proxy.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/value/value.h"

typedef struct ten_env_notify_return_result_info_t {
  ten_shared_ptr_t *c_cmd;
  ten_shared_ptr_t *c_target_cmd;
} ten_env_notify_return_result_info_t;

static ten_env_notify_return_result_info_t *
ten_env_notify_return_result_info_create(ten_shared_ptr_t *c_cmd,
                                         ten_shared_ptr_t *c_target_cmd) {
  TEN_ASSERT(c_cmd, "Invalid argument.");

  ten_env_notify_return_result_info_t *info =
      TEN_MALLOC(sizeof(ten_env_notify_return_result_info_t));
  TEN_ASSERT(info, "Failed to allocate memory.");

  info->c_cmd = c_cmd;
  info->c_target_cmd = c_target_cmd;

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

  TEN_FREE(info);
}

static void ten_notify_return_result(ten_env_t *ten_env, void *user_data) {
  TEN_ASSERT(user_data, "Invalid argument.");
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_env_notify_return_result_info_t *info = user_data;

  ten_error_t err;
  ten_error_init(&err);

  bool rc = false;
  if (info->c_target_cmd) {
    rc = ten_env_return_result(ten_env, info->c_cmd, info->c_target_cmd, &err);
    TEN_ASSERT(rc, "Should not happen.");
  } else {
    rc = ten_env_return_result_directly(ten_env, info->c_cmd, &err);
    TEN_ASSERT(rc, "Should not happen.");
  }

  ten_error_deinit(&err);

  ten_env_notify_return_result_info_destroy(info);
}

ten_go_status_t ten_go_ten_env_return_result(uintptr_t bridge_addr,
                                             uintptr_t cmd_result_bridge_addr,
                                             uintptr_t cmd_bridge_addr) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");

  ten_go_msg_t *cmd = ten_go_msg_reinterpret(cmd_bridge_addr);
  TEN_ASSERT(cmd && ten_go_msg_check_integrity(cmd), "Should not happen.");
  TEN_ASSERT(ten_go_msg_c_msg(cmd), "Should not happen.");

  ten_go_msg_t *cmd_result = ten_go_msg_reinterpret(cmd_result_bridge_addr);
  TEN_ASSERT(cmd_result && ten_go_msg_check_integrity(cmd_result),
             "Should not happen.");

  ten_go_status_t api_status;
  ten_go_status_init_with_errno(&api_status, TEN_ERRNO_OK);
  TEN_GO_TEN_IS_ALIVE_REGION_BEGIN(
      self, { ten_go_status_set_errno(&api_status, TEN_ERRNO_TEN_IS_CLOSED); });

  ten_error_t err;
  ten_error_init(&err);

  ten_env_notify_return_result_info_t *return_result_info =
      ten_env_notify_return_result_info_create(
          ten_go_msg_move_c_msg(cmd_result), ten_go_msg_move_c_msg(cmd));

  if (!ten_env_proxy_notify(self->c_ten_env_proxy, ten_notify_return_result,
                            return_result_info, false, &err)) {
    ten_env_notify_return_result_info_destroy(return_result_info);
    ten_go_status_from_error(&api_status, &err);
  }

  ten_error_deinit(&err);
  TEN_GO_TEN_IS_ALIVE_REGION_END(self);

ten_is_close:
  return api_status;
}

ten_go_status_t ten_go_ten_env_return_result_directly(
    uintptr_t bridge_addr, uintptr_t cmd_result_bridge_addr) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");

  ten_go_msg_t *cmd_result = ten_go_msg_reinterpret(cmd_result_bridge_addr);
  TEN_ASSERT(cmd_result && ten_go_msg_check_integrity(cmd_result),
             "Should not happen.");

  ten_go_status_t api_status;
  ten_go_status_init_with_errno(&api_status, TEN_ERRNO_OK);

  TEN_GO_TEN_IS_ALIVE_REGION_BEGIN(
      self, { ten_go_status_set_errno(&api_status, TEN_ERRNO_TEN_IS_CLOSED); });
  ten_error_t err;
  ten_error_init(&err);

  ten_env_notify_return_result_info_t *return_result_info =
      ten_env_notify_return_result_info_create(
          ten_go_msg_move_c_msg(cmd_result), NULL);

  if (!ten_env_proxy_notify(self->c_ten_env_proxy, ten_notify_return_result,
                            return_result_info, false, &err)) {
    ten_env_notify_return_result_info_destroy(return_result_info);
    ten_go_status_from_error(&api_status, &err);
  }

  ten_error_deinit(&err);
  TEN_GO_TEN_IS_ALIVE_REGION_END(self);

ten_is_close:
  return api_status;
}

bool ten_go_ten_return_status_value(ten_go_ten_env_t *self, ten_go_msg_t *cmd,
                                    TEN_STATUS_CODE status_code,
                                    ten_value_t *status_value,
                                    ten_go_status_t *api_status) {
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");
  TEN_ASSERT(cmd && ten_go_msg_check_integrity(cmd), "Should not happen.");
  TEN_ASSERT(status_value && ten_value_check_integrity(status_value),
             "Should not happen.");
  TEN_ASSERT(api_status, "Should not happen.");

  bool result = true;

  ten_error_t err;
  ten_error_init(&err);

  ten_shared_ptr_t *cmd_result = ten_cmd_result_create(status_code);

  TEN_GO_TEN_IS_ALIVE_REGION_BEGIN(
      self, { ten_go_status_set_errno(api_status, TEN_ERRNO_TEN_IS_CLOSED); });

  ten_env_notify_return_result_info_t *return_result_info =
      ten_env_notify_return_result_info_create(cmd_result,
                                               ten_go_msg_move_c_msg(cmd));
  if (!ten_env_proxy_notify(self->c_ten_env_proxy, ten_notify_return_result,
                            return_result_info, false, &err)) {
    result = false;
    ten_env_notify_return_result_info_destroy(return_result_info);
    ten_go_status_from_error(api_status, &err);
  }

  TEN_GO_TEN_IS_ALIVE_REGION_END(self);

  ten_error_deinit(&err);

ten_is_close:
  return result;
}
