//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env_internal.h"

#include <stdint.h>

#include "include_internal/ten_runtime/binding/go/msg/msg.h"
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd_base.h"
#include "ten_utils/macro/check.h"
#include "ten_runtime/binding/go/interface/ten/common.h"
#include "ten_runtime/binding/go/interface/ten/msg.h"
#include "ten_utils/lib/alloc.h"

extern void tenGoOnCmdResult(ten_go_handle_t ten_bridge,
                             ten_go_handle_t cmd_bridge,
                             ten_go_handle_t result_handler);

ten_go_callback_info_t *ten_go_callback_info_create(
    ten_go_handle_t handler_id) {
  ten_go_callback_info_t *info =
      (ten_go_callback_info_t *)TEN_MALLOC(sizeof(ten_go_callback_info_t));
  TEN_ASSERT(info, "Failed to allocate memory.");

  info->callback_id = handler_id;

  return info;
}

void ten_go_callback_info_destroy(ten_go_callback_info_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  TEN_FREE(self);
}

void proxy_send_xxx_callback(ten_extension_t *extension, ten_env_t *ten_env,
                             ten_shared_ptr_t *cmd_result,
                             void *callback_info) {
  TEN_ASSERT(extension && ten_extension_check_integrity(extension, true),
             "Should not happen.");
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");
  TEN_ASSERT(cmd_result && ten_cmd_base_check_integrity(cmd_result),
             "Should not happen.");
  TEN_ASSERT(callback_info, "Should not happen.");

  ten_go_ten_env_t *ten_bridge = ten_go_ten_env_wrap(ten_env);
  ten_go_handle_t handler_id =
      ((ten_go_callback_info_t *)callback_info)->callback_id;

  // Same as Extension::OnCmd, the GO cmd result is only used for the GO
  // extension, so it can be created in GO world. We do not need to call GO
  // function to create the GO cmd result in C.
  ten_go_msg_t *cmd_bridge = ten_go_msg_create(cmd_result);
  uintptr_t cmd_bridge_addr = (uintptr_t)cmd_bridge;

  tenGoOnCmdResult(ten_bridge->bridge.go_instance, cmd_bridge_addr, handler_id);

  ten_go_callback_info_destroy(callback_info);
}
