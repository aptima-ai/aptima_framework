//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_runtime/axis_env/internal/send.h"

#include <stdbool.h>

#include "include_internal/axis_runtime/app/msg_interface/common.h"
#include "include_internal/axis_runtime/common/loc.h"
#include "include_internal/axis_runtime/engine/msg_interface/common.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension/msg_not_connected_cnt.h"
#include "include_internal/axis_runtime/extension_context/extension_context.h"
#include "include_internal/axis_runtime/extension_group/msg_interface/common.h"
#include "include_internal/axis_runtime/extension_thread/extension_thread.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd_base.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/axis_env/send.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "axis_runtime/app/app.h"
#include "axis_runtime/common/errno.h"
#include "axis_runtime/msg/cmd_result/cmd_result.h"
#include "axis_runtime/msg/msg.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/log/log.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/memory.h"

/**
 * @brief All message-sending code paths will ultimately converge in this
 * function.
 */
static bool axis_send_msg_internal(
    axis_env_t *self, axis_shared_ptr_t *msg,
    axis_env_msg_result_handler_func_t result_handler,
    void *result_handler_user_data, axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_env_check_integrity(self, true), "Invalid use of axis_env %p.",
             self);
  axis_ASSERT(msg && axis_msg_check_integrity(msg), "Should not happen.");

  const bool msg_is_cmd = axis_msg_is_cmd(msg);

  // Even if the user does not pass in the `err` parameter, since different
  // error scenarios require different handling, we need to create a temporary
  // one to obtain the actual error information.
  bool err_new_created = false;
  if (!err) {
    err = axis_error_create();
    err_new_created = true;
  }

  bool result = true;

  if (!msg) {
    axis_error_set(err, axis_ERRNO_INVALID_ARGUMENT, "Msg is empty.");
    result = false;
    goto done;
  }

  if (axis_msg_get_type(msg) == axis_MSG_TYPE_CMD_RESULT) {
    // The only way to send out the 'status' command from extension is through
    // various 'return_xxx' functions.
    axis_LOGE("Result commands should be delivered through 'returning'.");
    axis_error_set(err, axis_ERRNO_GENERIC,
                  "Result commands should be delivered through 'returning'.");
    result = false;
    goto done;
  }

  if (axis_msg_has_locked_res(msg)) {
    axis_error_set(err, axis_ERRNO_GENERIC,
                  "Locked resources are not allowed in messages sent from an "
                  "extension.");
    result = false;
    goto done;
  }

  if (msg_is_cmd) {
    // 'command' has the mechanism of 'result'.
    axis_cmd_base_set_result_handler(msg, result_handler,
                                    result_handler_user_data);

    // @{
    // All commands sent from an extension will eventually go to this function,
    // and command ID plays a critical role in the TEN runtime, therefore, when
    // a command is sent from an extension, and doesn't contain a command ID,
    // TEN runtime must assign it a command ID _before_ sending it to the TEN
    // runtime deeper.
    const char *cmd_id = axis_cmd_base_get_cmd_id(msg);
    if (!strlen(cmd_id)) {
      cmd_id = axis_cmd_base_gen_new_cmd_id_forcibly(msg);
    }
    // @}
  }

  switch (axis_env_get_attach_to(self)) {
    case axis_ENV_ATTACH_TO_EXTENSION: {
      axis_extension_t *extension = axis_env_get_attached_extension(self);
      axis_ASSERT(extension, "Should not happen.");

      result = axis_extension_dispatch_msg(extension, msg, err);
      break;
    }

    case axis_ENV_ATTACH_TO_EXTENSION_GROUP: {
      axis_extension_group_t *extension_group =
          axis_env_get_attached_extension_group(self);
      axis_ASSERT(extension_group, "Should not happen.");

      result = axis_extension_group_dispatch_msg(extension_group, msg, err);
      break;
    }

    case axis_ENV_ATTACH_TO_ENGINE: {
      axis_engine_t *engine = axis_env_get_attached_engine(self);
      axis_ASSERT(engine, "Should not happen.");

      result = axis_engine_dispatch_msg(engine, msg);
      break;
    }

    case axis_ENV_ATTACH_TO_APP: {
      axis_app_t *app = axis_env_get_attached_app(self);
      axis_ASSERT(app, "Should not happen.");

      result = axis_app_dispatch_msg(app, msg, err);
      break;
    }

    default:
      axis_ASSERT(0, "Handle more conditions: %d", axis_env_get_attach_to(self));
      break;
  }

done:
  if (!result) {
    if (axis_error_errno(err) == axis_ERRNO_MSG_NOT_CONNECTED) {
      if (axis_env_get_attach_to(self) == axis_ENV_ATTACH_TO_EXTENSION) {
        axis_extension_t *extension = axis_env_get_attached_extension(self);
        axis_ASSERT(extension, "Should not happen.");

        if (axis_extension_increment_msg_not_connected_count(
                extension, axis_msg_get_name(msg))) {
          axis_LOGW("Failed to send message: %s", axis_error_errmsg(err));
        }
      } else {
        axis_LOGE("Failed to send message: %s", axis_error_errmsg(err));
      }
    } else {
      axis_LOGE("Failed to send message: %s", axis_error_errmsg(err));
    }
  }

  if (err_new_created) {
    axis_error_destroy(err);
  }

  if (result && result_handler && !msg_is_cmd) {
    // If the method synchronously returns true, it means that the callback must
    // be called.
    //
    // For command-type messages, its result handler will be invoked when the
    // target extension returns a response.
    //
    // For other types of messages, such as data/audio_frame/video_frame, we
    // temporarily consider the sending to be successful right after the message
    // is enqueued (In the future, this time point might change, but overall, as
    // long as the result handler is provided, it must be invoked in this case).
    // Therefore, it is necessary to execute the callback function and set the
    // error to NULL to indicate that no error has occurred.
    result_handler(self, NULL, result_handler_user_data, NULL);
  }

  return result;
}

static axis_cmd_result_handler_for_send_cmd_ctx_t *
axis_cmd_result_handler_for_send_cmd_ctx_create(
    axis_env_msg_result_handler_func_t result_handler,
    void *result_handler_user_data) {
  axis_cmd_result_handler_for_send_cmd_ctx_t *self =
      axis_MALLOC(sizeof(axis_cmd_result_handler_for_send_cmd_ctx_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  self->result_handler = result_handler;
  self->result_handler_user_data = result_handler_user_data;

  return self;
}

static void axis_cmd_result_handler_for_send_cmd_ctx_destroy(
    axis_cmd_result_handler_for_send_cmd_ctx_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_FREE(self);
}

static void cmd_result_handler_for_send_cmd(axis_env_t *axis_env,
                                            axis_shared_ptr_t *cmd_result,
                                            void *cmd_result_handler_user_data,
                                            axis_error_t *err) {
  axis_cmd_result_handler_for_send_cmd_ctx_t *ctx = cmd_result_handler_user_data;
  axis_ASSERT(ctx, "Invalid argument.");
  axis_ASSERT(ctx->result_handler, "Should not happen.");

  // The differences between `send_cmd` and `send_cmd_ex` is that `send_cmd`
  // will only return the final `result` of `is_completed`. If other behaviors
  // are needed, users can use `send_cmd_ex`.
  if (axis_cmd_result_is_completed(cmd_result, NULL)) {
    ctx->result_handler(axis_env, cmd_result, ctx->result_handler_user_data,
                        err);

    axis_cmd_result_handler_for_send_cmd_ctx_destroy(ctx);
  }
}

bool axis_env_send_cmd(axis_env_t *self, axis_shared_ptr_t *cmd,
                      axis_env_msg_result_handler_func_t result_handler,
                      void *result_handler_user_data, axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_env_check_integrity(self, true), "Invalid use of axis_env %p.",
             self);
  axis_ASSERT(cmd, "Should not happen.");

  bool rc = false;

  if (result_handler) {
    axis_cmd_result_handler_for_send_cmd_ctx_t *ctx =
        axis_cmd_result_handler_for_send_cmd_ctx_create(
            result_handler, result_handler_user_data);

    rc = axis_send_msg_internal(self, cmd, cmd_result_handler_for_send_cmd, ctx,
                               err);
    if (!rc) {
      axis_cmd_result_handler_for_send_cmd_ctx_destroy(ctx);
    }
  } else {
    axis_ASSERT(!result_handler_user_data, "Should not happen.");

    rc = axis_send_msg_internal(self, cmd, NULL, NULL, err);
  }

  return rc;
}

bool axis_env_send_cmd_ex(axis_env_t *self, axis_shared_ptr_t *cmd,
                         axis_env_msg_result_handler_func_t result_handler,
                         void *result_handler_user_data, axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_env_check_integrity(self, true), "Invalid use of axis_env %p.",
             self);
  axis_ASSERT(cmd, "Should not happen.");

  return axis_send_msg_internal(self, cmd, result_handler,
                               result_handler_user_data, err);
}

bool axis_env_send_data(axis_env_t *self, axis_shared_ptr_t *data,
                       axis_env_msg_result_handler_func_t result_handler,
                       void *result_handler_user_data, axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_env_check_integrity(self, true), "Invalid use of axis_env %p.",
             self);
  axis_ASSERT(data, "Should not happen.");

  return axis_send_msg_internal(self, data, result_handler,
                               result_handler_user_data, err);
}

bool axis_env_send_video_frame(axis_env_t *self, axis_shared_ptr_t *frame,
                              axis_env_msg_result_handler_func_t result_handler,
                              void *result_handler_user_data,
                              axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_env_check_integrity(self, true), "Invalid use of axis_env %p.",
             self);
  axis_ASSERT(frame, "Should not happen.");

  return axis_send_msg_internal(self, frame, result_handler,
                               result_handler_user_data, err);
}

bool axis_env_send_audio_frame(axis_env_t *self, axis_shared_ptr_t *frame,
                              axis_env_msg_result_handler_func_t result_handler,
                              void *result_handler_user_data,
                              axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_env_check_integrity(self, true), "Invalid use of axis_env %p.",
             self);
  axis_ASSERT(frame, "Should not happen.");

  return axis_send_msg_internal(self, frame, result_handler,
                               result_handler_user_data, err);
}
