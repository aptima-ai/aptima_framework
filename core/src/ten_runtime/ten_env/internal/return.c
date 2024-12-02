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

static bool ten_env_return_result_internal(
    ten_env_t *self, ten_shared_ptr_t *result_cmd, const char *cmd_id,
    const char *seq_id,
    ten_env_return_result_error_handler_func_t error_handler,
    void *error_handler_user_data, ten_error_t *err) {
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

  bool result = ten_extension_handle_out_msg(extension, result_cmd, err);
  if (result && error_handler) {
    // If the method synchronously returns true, it means that the callback must
    // be called.
    //
    // We temporarily assume that the message enqueue represents success;
    // therefore, in this case, we set the error to NULL to indicate that the
    // returning was successful.
    error_handler(self, error_handler_user_data, NULL);
  }

  if (err_new_created) {
    ten_error_destroy(err);
  }

  return result;
}

// If the 'cmd' has already been a command in the backward path, a extension
// could use this API to return the 'cmd' further.
bool ten_env_return_result_directly(
    ten_env_t *self, ten_shared_ptr_t *result_cmd,
    ten_env_return_result_error_handler_func_t error_handler,
    void *error_handler_user_data, ten_error_t *err) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_env_check_integrity(self, true), "Invalid use of ten_env %p.",
             self);
  TEN_ASSERT(result_cmd && ten_cmd_base_check_integrity(result_cmd),
             "Should not happen.");
  TEN_ASSERT(ten_msg_get_type(result_cmd) == TEN_MSG_TYPE_CMD_RESULT,
             "The target cmd must be a cmd result.");

  return ten_env_return_result_internal(self, result_cmd, NULL, NULL,
                                        error_handler, error_handler_user_data,
                                        err);
}

bool ten_env_return_result(
    ten_env_t *self, ten_shared_ptr_t *result_cmd, ten_shared_ptr_t *target_cmd,
    ten_env_return_result_error_handler_func_t error_handler,
    void *error_handler_user_data, ten_error_t *err) {
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
      ten_cmd_base_get_seq_id(target_cmd), error_handler,
      error_handler_user_data, err);
}
