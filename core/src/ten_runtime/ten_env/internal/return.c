//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "ten_runtime/ten_env/internal/return.h"

#include "include_internal/ten_runtime/common/loc.h"
#include "include_internal/ten_runtime/extension/extension.h"
#include "include_internal/ten_runtime/extension_thread/extension_thread.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd_base.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "ten_runtime/ten_env/ten_env.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/macro/check.h"

static bool ten_env_return_result_internal(ten_env_t *self,
                                           ten_shared_ptr_t *result_cmd,
                                           const char *cmd_id,
                                           const char *seq_id,
                                           ten_error_t *err) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_env_check_integrity(self, true), "Invalid use of ten_env %p.",
             self);
  TEN_ASSERT(result_cmd && ten_cmd_base_check_integrity(result_cmd),
             "Should not happen.");
  TEN_ASSERT(ten_msg_get_type(result_cmd) == TEN_MSG_TYPE_CMD_RESULT,
             "Should not happen.");

  bool err_new_created = false;
  if (!err) {
    err = ten_error_create();
    err_new_created = true;
  }

  // cmd_id is very critical in the way finding.
  if (cmd_id) {
    ten_cmd_base_set_cmd_id(result_cmd, cmd_id);
  }

  // seq_id is important if the target of the 'cmd' is a client outside TEN.
  if (seq_id) {
    ten_cmd_base_set_seq_id(result_cmd, seq_id);
  }

  ten_extension_t *extension = ten_env_get_attached_extension(self);
  TEN_ASSERT(extension && ten_extension_check_integrity(extension, true),
             "Invalid use of extension %p.", extension);

  bool result = true;

  if (extension->state >= TEN_EXTENSION_STATE_CLOSING) {
    TEN_LOGW("Cannot return results after on_stop_done.");
    ten_error_set(err, TEN_ERRNO_GENERIC,
                  "Cannot return results after on_stop_done.");
    result = false;
    goto done;
  }

  result = ten_extension_handle_out_msg(extension, result_cmd, err);

done:
  if (err_new_created) {
    ten_error_destroy(err);
  }

  return result;
}

// If the 'cmd' has already been a command in the backward path, a extension
// could use this API to return the 'cmd' further.
bool ten_env_return_result_directly(ten_env_t *self,
                                    ten_shared_ptr_t *result_cmd,
                                    ten_error_t *err) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_env_check_integrity(self, true), "Invalid use of ten_env %p.",
             self);
  TEN_ASSERT(result_cmd && ten_cmd_base_check_integrity(result_cmd),
             "Should not happen.");
  TEN_ASSERT(ten_msg_get_type(result_cmd) == TEN_MSG_TYPE_CMD_RESULT,
             "The target cmd must be a cmd result.");

  return ten_env_return_result_internal(self, result_cmd, NULL, NULL, err);
}

bool ten_env_return_result(ten_env_t *self, ten_shared_ptr_t *result_cmd,
                           ten_shared_ptr_t *target_cmd, ten_error_t *err) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_env_check_integrity(self, true), "Invalid use of ten_env %p.",
             self);
  TEN_ASSERT(result_cmd && ten_cmd_base_check_integrity(result_cmd),
             "Should not happen.");
  TEN_ASSERT((target_cmd ? ten_cmd_base_check_integrity(target_cmd) : true),
             "Should not happen.");
  TEN_ASSERT(ten_msg_get_type(target_cmd) != TEN_MSG_TYPE_CMD_RESULT,
             "The target cmd should not be a cmd result.");

  return ten_env_return_result_internal(
      self, result_cmd, ten_cmd_base_get_cmd_id(target_cmd),
      ten_cmd_base_get_seq_id(target_cmd), err);
}
