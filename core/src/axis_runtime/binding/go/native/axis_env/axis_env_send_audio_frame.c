//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <stdlib.h>

#include "include_internal/axis_runtime/binding/go/internal/common.h"
#include "include_internal/axis_runtime/binding/go/msg/msg.h"
#include "include_internal/axis_runtime/binding/go/axis_env/axis_env.h"
#include "include_internal/axis_runtime/binding/go/axis_env/axis_env_internal.h"
#include "axis_runtime/binding/go/interface/ten/common.h"
#include "axis_runtime/binding/go/interface/ten/msg.h"
#include "axis_runtime/binding/go/interface/ten/axis_env.h"
#include "axis_runtime/axis_env_proxy/axis_env_proxy.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"

typedef struct axis_env_notify_send_audio_frame_ctx_t {
  axis_shared_ptr_t *c_audio_frame;
  axis_go_handle_t callback_handle;
} axis_env_notify_send_audio_frame_ctx_t;

static axis_env_notify_send_audio_frame_ctx_t *
axis_env_notify_send_audio_frame_ctx_create(axis_shared_ptr_t *c_audio_frame,
                                           axis_go_handle_t callback_handle) {
  axis_ASSERT(c_audio_frame, "Invalid argument.");

  axis_env_notify_send_audio_frame_ctx_t *ctx =
      axis_MALLOC(sizeof(axis_env_notify_send_audio_frame_ctx_t));
  axis_ASSERT(ctx, "Failed to allocate memory.");

  ctx->c_audio_frame = c_audio_frame;
  ctx->callback_handle = callback_handle;

  return ctx;
}

static void axis_env_notify_send_audio_frame_ctx_destroy(
    axis_env_notify_send_audio_frame_ctx_t *ctx) {
  axis_ASSERT(ctx, "Invalid argument.");

  if (ctx->c_audio_frame) {
    axis_shared_ptr_destroy(ctx->c_audio_frame);
    ctx->c_audio_frame = NULL;
  }

  ctx->callback_handle = 0;

  axis_FREE(ctx);
}

static void proxy_handle_audio_frame_error(
    axis_env_t *axis_env, axis_UNUSED axis_shared_ptr_t *cmd_result,
    void *callback_info_, axis_error_t *err) {
  axis_go_callback_ctx_t *callback_info = callback_info_;
  axis_ASSERT(callback_info, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  if (err) {
    axis_go_error_from_error(&cgo_error, err);
  }

  axis_ASSERT(callback_info->callback_id != axis_GO_NO_RESPONSE_HANDLER,
             "Should not happen.");

  axis_go_axis_env_t *axis_env_bridge = axis_go_axis_env_wrap(axis_env);

  tenGoOnError(axis_env_bridge->bridge.go_instance, callback_info->callback_id,
               cgo_error);

  axis_go_callback_ctx_destroy(callback_info);
}

static void axis_env_proxy_notify_send_audio_frame(axis_env_t *axis_env,
                                                  void *user_data) {
  axis_ASSERT(user_data, "Invalid argument.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_env_notify_send_audio_frame_ctx_t *notify_info = user_data;
  axis_ASSERT(notify_info, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_error_t err;
  axis_error_init(&err);

  bool res = false;

  if (notify_info->callback_handle == axis_GO_NO_RESPONSE_HANDLER) {
    res = axis_env_send_audio_frame(axis_env, notify_info->c_audio_frame, NULL,
                                   NULL, &err);
  } else {
    axis_go_callback_ctx_t *ctx =
        axis_go_callback_ctx_create(notify_info->callback_handle);

    res = axis_env_send_audio_frame(axis_env, notify_info->c_audio_frame,
                                   proxy_handle_audio_frame_error, notify_info,
                                   &err);

    if (!res) {
      // Prepare error information to pass to Go.
      axis_go_error_from_error(&cgo_error, &err);

      axis_go_axis_env_t *axis_env_bridge = axis_go_axis_env_wrap(axis_env);

      tenGoOnError(axis_env_bridge->bridge.go_instance,
                   notify_info->callback_handle, cgo_error);

      axis_go_callback_ctx_destroy(ctx);
    }
  }

  axis_error_deinit(&err);

  axis_env_notify_send_audio_frame_ctx_destroy(notify_info);
}

axis_go_error_t axis_go_axis_env_send_audio_frame(
    uintptr_t bridge_addr, uintptr_t audio_frame_bridge_addr,
    axis_go_handle_t handler_id) {
  axis_go_axis_env_t *self = axis_go_axis_env_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");

  axis_go_msg_t *audio_frame = axis_go_msg_reinterpret(audio_frame_bridge_addr);
  axis_ASSERT(audio_frame && axis_go_msg_check_integrity(audio_frame),
             "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_GO_axis_ENV_IS_ALIVE_REGION_BEGIN(
      self, { axis_go_error_set_errno(&cgo_error, axis_ERRNO_axis_IS_CLOSED); });

  axis_error_t err;
  axis_error_init(&err);

  axis_env_notify_send_audio_frame_ctx_t *notify_info =
      axis_env_notify_send_audio_frame_ctx_create(
          axis_go_msg_move_c_msg(audio_frame),
          handler_id <= 0 ? axis_GO_NO_RESPONSE_HANDLER : handler_id);

  if (!axis_env_proxy_notify(self->c_axis_env_proxy,
                            axis_env_proxy_notify_send_audio_frame, notify_info,
                            false, &err)) {
    axis_env_notify_send_audio_frame_ctx_destroy(notify_info);
    axis_go_error_from_error(&cgo_error, &err);
  }

  axis_error_deinit(&err);
  axis_GO_axis_ENV_IS_ALIVE_REGION_END(self);

axis_is_close:
  return cgo_error;
}
