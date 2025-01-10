//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_runtime/axis_env/internal/return.h"

#include "include_internal/axis_runtime/common/loc.h"
#include "include_internal/axis_runtime/engine/engine.h"
#include "include_internal/axis_runtime/engine/msg_interface/common.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension_group/extension_group.h"
#include "include_internal/axis_runtime/extension_group/msg_interface/common.h"
#include "include_internal/axis_runtime/extension_thread/extension_thread.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd_base.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/macro/check.h"

static bool axis_env_return_result_internal(
    axis_env_t *self, axis_shared_ptr_t *result_cmd, const char *cmd_id,
    const char *seq_id,
    axis_env_return_result_error_handler_func_t error_handler,
    void *error_handler_user_data, axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_env_check_integrity(self, true), "Invalid use of axis_env %p.",
             self);
  axis_ASSERT(result_cmd && axis_cmd_base_check_integrity(result_cmd),
             "Should not happen.");
  axis_ASSERT(axis_msg_get_type(result_cmd) == axis_MSG_TYPE_CMD_RESULT,
             "Should not happen.");

  bool err_new_created = false;
  if (!err) {
    err = axis_error_create();
    err_new_created = true;
  }

  // cmd_id is very critical in the way finding.
  if (cmd_id) {
    axis_cmd_base_set_cmd_id(result_cmd, cmd_id);
  }

  // seq_id is important if the target of the 'cmd' is a client outside APTIMA.
  if (seq_id) {
    axis_cmd_base_set_seq_id(result_cmd, seq_id);
  }

  bool result = true;

  switch (axis_env_get_attach_to(self)) {
    case axis_ENV_ATTACH_TO_EXTENSION: {
      axis_extension_t *extension = axis_env_get_attached_extension(self);
      axis_ASSERT(extension && axis_extension_check_integrity(extension, true),
                 "Invalid use of extension %p.", extension);

      result = axis_extension_dispatch_msg(extension, result_cmd, err);
      break;
    }

    case axis_ENV_ATTACH_TO_EXTENSION_GROUP: {
      axis_extension_group_t *extension_group =
          axis_env_get_attached_extension_group(self);
      axis_ASSERT(extension_group &&
                     axis_extension_group_check_integrity(extension_group, true),
                 "Invalid use of extension_group %p.", extension_group);

      result =
          axis_extension_group_dispatch_msg(extension_group, result_cmd, err);
      break;
    }

    case axis_ENV_ATTACH_TO_ENGINE: {
      axis_engine_t *engine = axis_env_get_attached_engine(self);
      axis_ASSERT(engine && axis_engine_check_integrity(engine, true),
                 "Invalid use of engine %p.", engine);

      result = axis_engine_dispatch_msg(engine, result_cmd);
      break;
    }

    default:
      axis_ASSERT(0, "Handle this condition.");
      break;
  }

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
    axis_error_destroy(err);
  }

  return result;
}

// If the 'cmd' has already been a command in the backward path, a extension
// could use this API to return the 'cmd' further.
bool axis_env_return_result_directly(
    axis_env_t *self, axis_shared_ptr_t *result_cmd,
    axis_env_return_result_error_handler_func_t error_handler,
    void *error_handler_user_data, axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_env_check_integrity(self, true), "Invalid use of axis_env %p.",
             self);
  axis_ASSERT(result_cmd && axis_cmd_base_check_integrity(result_cmd),
             "Should not happen.");
  axis_ASSERT(axis_msg_get_type(result_cmd) == axis_MSG_TYPE_CMD_RESULT,
             "The target cmd must be a cmd result.");

  return axis_env_return_result_internal(self, result_cmd, NULL, NULL,
                                        error_handler, error_handler_user_data,
                                        err);
}

bool axis_env_return_result(
    axis_env_t *self, axis_shared_ptr_t *result_cmd, axis_shared_ptr_t *target_cmd,
    axis_env_return_result_error_handler_func_t error_handler,
    void *error_handler_user_data, axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_env_check_integrity(self, true), "Invalid use of axis_env %p.",
             self);
  axis_ASSERT(result_cmd && axis_cmd_base_check_integrity(result_cmd),
             "Should not happen.");
  axis_ASSERT((target_cmd ? axis_cmd_base_check_integrity(target_cmd) : true),
             "Should not happen.");
  axis_ASSERT(axis_msg_get_type(target_cmd) != axis_MSG_TYPE_CMD_RESULT,
             "The target cmd should not be a cmd result.");

  return axis_env_return_result_internal(
      self, result_cmd, axis_cmd_base_get_cmd_id(target_cmd),
      axis_cmd_base_get_seq_id(target_cmd), error_handler,
      error_handler_user_data, err);
}
