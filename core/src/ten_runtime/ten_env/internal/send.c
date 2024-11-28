//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "ten_runtime/ten_env/internal/send.h"

#include <stdbool.h>

#include "include_internal/ten_runtime/app/msg_interface/common.h"
#include "include_internal/ten_runtime/common/loc.h"
#include "include_internal/ten_runtime/extension/extension.h"
#include "include_internal/ten_runtime/extension_context/extension_context.h"
#include "include_internal/ten_runtime/extension_thread/extension_thread.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd_base.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "include_internal/ten_runtime/ten_env/send.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "ten_runtime/common/errno.h"
#include "ten_runtime/msg/cmd_result/cmd_result.h"
#include "ten_runtime/msg/msg.h"
#include "ten_runtime/ten_env/ten_env.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/memory.h"

/**
 * @brief All message sending code flows will eventually fall into this
 * function.
 */
static bool ten_send_msg_internal(
    ten_env_t *self, ten_shared_ptr_t *msg,
    ten_env_msg_result_handler_func_t result_handler,
    void *result_handler_user_data, ten_error_t *err) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_env_check_integrity(self, true), "Invalid use of ten_env %p.",
             self);
  TEN_ASSERT(msg && ten_msg_check_integrity(msg), "Should not happen.");

  const bool msg_is_cmd = ten_msg_is_cmd(msg);

  bool err_new_created = false;
  if (!err) {
    err = ten_error_create();
    err_new_created = true;
  }

  bool result = true;

  if (!msg) {
    ten_error_set(err, TEN_ERRNO_INVALID_ARGUMENT, "Msg is empty.");
    result = false;
    goto done;
  }

  if (ten_msg_get_type(msg) == TEN_MSG_TYPE_CMD_RESULT) {
    // The only way to send out the 'status' command from extension is through
    // various 'return_xxx' functions.
    TEN_LOGE("Result commands should be delivered through 'returning'.");
    ten_error_set(err, TEN_ERRNO_GENERIC,
                  "Result commands should be delivered through 'returning'.");
    result = false;
    goto done;
  }

  if (ten_msg_has_locked_res(msg)) {
    ten_error_set(err, TEN_ERRNO_GENERIC,
                  "Locked resources are not allowed in messages sent from an "
                  "extension.");
    result = false;
    goto done;
  }

  if (msg_is_cmd) {
    // 'command' has the mechanism of 'result'.
    ten_cmd_base_set_result_handler(msg, result_handler,
                                    result_handler_user_data);

    // @{
    // All commands sent from an extension will eventually go to this function,
    // and command ID plays a critical role in the TEN runtime, therefore, when
    // a command is sent from an extension, and doesn't contain a command ID,
    // TEN runtime must assign it a command ID _before_ sending it to the TEN
    // runtime deeper.
    const char *cmd_id = ten_cmd_base_get_cmd_id(msg);
    if (!strlen(cmd_id)) {
      cmd_id = ten_cmd_base_gen_new_cmd_id_forcibly(msg);
    }
    // @}
  }

  switch (ten_env_get_attach_to(self)) {
    case TEN_ENV_ATTACH_TO_EXTENSION: {
      ten_extension_t *extension = ten_env_get_attached_extension(self);
      TEN_ASSERT(extension, "Should not happen.");

      result = ten_extension_handle_out_msg(extension, msg, err);
      break;
    }

    case TEN_ENV_ATTACH_TO_APP: {
      ten_app_t *app = ten_env_get_attached_app(self);
      TEN_ASSERT(app, "Should not happen.");

      result = ten_app_handle_out_msg(app, msg, err);
      break;
    }

    default:
      TEN_ASSERT(0, "Handle more conditions: %d", ten_env_get_attach_to(self));
      break;
  }

done:
  if (err_new_created) {
    ten_error_destroy(err);
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

static ten_cmd_result_handler_for_send_cmd_ctx_t *
ten_cmd_result_handler_for_send_cmd_ctx_create(
    ten_env_msg_result_handler_func_t result_handler,
    void *result_handler_user_data) {
  ten_cmd_result_handler_for_send_cmd_ctx_t *self =
      TEN_MALLOC(sizeof(ten_cmd_result_handler_for_send_cmd_ctx_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  self->result_handler = result_handler;
  self->result_handler_user_data = result_handler_user_data;

  return self;
}

static void ten_cmd_result_handler_for_send_cmd_ctx_destroy(
    ten_cmd_result_handler_for_send_cmd_ctx_t *self) {
  TEN_ASSERT(self, "Invalid argument.");

  TEN_FREE(self);
}

static void cmd_result_handler_for_send_cmd(ten_env_t *ten_env,
                                            ten_shared_ptr_t *cmd_result,
                                            void *cmd_result_handler_user_data,
                                            ten_error_t *err) {
  ten_cmd_result_handler_for_send_cmd_ctx_t *ctx = cmd_result_handler_user_data;
  TEN_ASSERT(ctx, "Invalid argument.");
  TEN_ASSERT(ctx->result_handler, "Should not happen.");

  // The differences between `send_cmd` and `send_cmd_ex` is that `send_cmd`
  // will only return the final `result` of `is_completed`. If other behaviors
  // are needed, users can use `send_cmd_ex`.
  if (ten_cmd_result_is_completed(cmd_result, NULL)) {
    ctx->result_handler(ten_env, cmd_result, ctx->result_handler_user_data,
                        err);

    ten_cmd_result_handler_for_send_cmd_ctx_destroy(ctx);
  }
}

bool ten_env_send_cmd(ten_env_t *self, ten_shared_ptr_t *cmd,
                      ten_env_msg_result_handler_func_t result_handler,
                      void *result_handler_user_data, ten_error_t *err) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_env_check_integrity(self, true), "Invalid use of ten_env %p.",
             self);
  TEN_ASSERT(cmd, "Should not happen.");

  if (result_handler) {
    ten_cmd_result_handler_for_send_cmd_ctx_t *ctx =
        ten_cmd_result_handler_for_send_cmd_ctx_create(
            result_handler, result_handler_user_data);

    return ten_send_msg_internal(self, cmd, cmd_result_handler_for_send_cmd,
                                 ctx, err);
  } else {
    return ten_send_msg_internal(self, cmd, NULL, result_handler_user_data,
                                 err);
  }
}

bool ten_env_send_cmd_ex(ten_env_t *self, ten_shared_ptr_t *cmd,
                         ten_env_msg_result_handler_func_t result_handler,
                         void *result_handler_user_data, ten_error_t *err) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_env_check_integrity(self, true), "Invalid use of ten_env %p.",
             self);
  TEN_ASSERT(cmd, "Should not happen.");

  return ten_send_msg_internal(self, cmd, result_handler,
                               result_handler_user_data, err);
}

bool ten_env_send_data(ten_env_t *self, ten_shared_ptr_t *data,
                       ten_env_msg_result_handler_func_t result_handler,
                       void *result_handler_user_data, ten_error_t *err) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_env_check_integrity(self, true), "Invalid use of ten_env %p.",
             self);
  TEN_ASSERT(data, "Should not happen.");

  return ten_send_msg_internal(self, data, result_handler,
                               result_handler_user_data, err);
}

bool ten_env_send_video_frame(ten_env_t *self, ten_shared_ptr_t *frame,
                              ten_env_msg_result_handler_func_t result_handler,
                              void *result_handler_user_data,
                              ten_error_t *err) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_env_check_integrity(self, true), "Invalid use of ten_env %p.",
             self);
  TEN_ASSERT(frame, "Should not happen.");

  return ten_send_msg_internal(self, frame, result_handler,
                               result_handler_user_data, err);
}

bool ten_env_send_audio_frame(ten_env_t *self, ten_shared_ptr_t *frame,
                              ten_env_msg_result_handler_func_t result_handler,
                              void *result_handler_user_data,
                              ten_error_t *err) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_env_check_integrity(self, true), "Invalid use of ten_env %p.",
             self);
  TEN_ASSERT(frame, "Should not happen.");

  return ten_send_msg_internal(self, frame, result_handler,
                               result_handler_user_data, err);
}
