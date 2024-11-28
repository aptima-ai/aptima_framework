//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <stdlib.h>

#include "include_internal/ten_runtime/binding/go/internal/common.h"
#include "include_internal/ten_runtime/binding/go/msg/msg.h"
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env.h"
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env_internal.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "ten_runtime/binding/go/interface/ten/common.h"
#include "ten_runtime/binding/go/interface/ten/msg.h"
#include "ten_runtime/binding/go/interface/ten/ten_env.h"
#include "ten_runtime/ten_env_proxy/ten_env_proxy.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"

typedef struct ten_env_notify_send_video_frame_info_t {
  ten_shared_ptr_t *c_video_frame;
  ten_go_handle_t callback_handle;
} ten_env_notify_send_video_frame_info_t;

static ten_env_notify_send_video_frame_info_t *
ten_env_notify_send_video_frame_info_create(ten_shared_ptr_t *c_video_frame,
                                            ten_go_handle_t callback_handle) {
  TEN_ASSERT(c_video_frame, "Invalid argument.");

  ten_env_notify_send_video_frame_info_t *info =
      TEN_MALLOC(sizeof(ten_env_notify_send_video_frame_info_t));
  TEN_ASSERT(info, "Failed to allocate memory.");

  info->c_video_frame = c_video_frame;
  info->callback_handle = callback_handle;

  return info;
}

static void ten_env_notify_send_video_frame_info_destroy(
    ten_env_notify_send_video_frame_info_t *info) {
  TEN_ASSERT(info, "Invalid argument.");

  if (info->c_video_frame) {
    ten_shared_ptr_destroy(info->c_video_frame);
    info->c_video_frame = NULL;
  }

  info->callback_handle = 0;

  TEN_FREE(info);
}

static void proxy_handle_video_frame_error(
    ten_env_t *ten_env, TEN_UNUSED ten_shared_ptr_t *cmd_result,
    void *callback_info_, ten_error_t *err) {
  ten_go_callback_info_t *callback_info = callback_info_;
  TEN_ASSERT(callback_info, "Should not happen.");

  ten_go_error_t cgo_error;
  ten_go_error_init_with_errno(&cgo_error, TEN_ERRNO_OK);

  if (err) {
    ten_go_error_from_error(&cgo_error, err);
  }

  TEN_ASSERT(callback_info->callback_id != TEN_GO_NO_RESPONSE_HANDLER,
             "Should not happen.");

  ten_go_ten_env_t *ten_env_bridge = ten_go_ten_env_wrap(ten_env);

  tenGoOnError(ten_env_bridge->bridge.go_instance, callback_info->callback_id,
               cgo_error);

  ten_go_callback_info_destroy(callback_info);
}

static void ten_env_proxy_notify_send_video_frame(ten_env_t *ten_env,
                                                  void *user_video_frame) {
  TEN_ASSERT(user_video_frame, "Invalid argument.");
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_env_notify_send_video_frame_info_t *notify_info = user_video_frame;
  TEN_ASSERT(notify_info, "Should not happen.");

  ten_go_error_t cgo_error;
  ten_go_error_init_with_errno(&cgo_error, TEN_ERRNO_OK);

  ten_error_t err;
  ten_error_init(&err);

  bool res = false;

  if (notify_info->callback_handle == TEN_GO_NO_RESPONSE_HANDLER) {
    res = ten_env_send_video_frame(ten_env, notify_info->c_video_frame, NULL,
                                   NULL, &err);
    if (!res) {
      // The error cannot be handled by the developer, all we can do is to log
      // the error.
      TEN_LOGE(
          "Failed to send video frame, but no error handler is provided. "
          "errno: %d, errmsg: %s",
          ten_error_errno(&err), ten_error_errmsg(&err));
    }
  } else {
    ten_go_callback_info_t *info =
        ten_go_callback_info_create(notify_info->callback_handle);
    res = ten_env_send_video_frame(ten_env, notify_info->c_video_frame,
                                   proxy_handle_video_frame_error, info, &err);

    if (!res) {
      // Prepare error information to pass to Go.
      ten_go_error_from_error(&cgo_error, &err);

      ten_go_ten_env_t *ten_env_bridge = ten_go_ten_env_wrap(ten_env);

      tenGoOnError(ten_env_bridge->bridge.go_instance,
                   notify_info->callback_handle, cgo_error);

      ten_go_callback_info_destroy(info);
    }
  }

  ten_error_deinit(&err);

  ten_env_notify_send_video_frame_info_destroy(notify_info);
}

ten_go_error_t ten_go_ten_env_send_video_frame(
    uintptr_t bridge_addr, uintptr_t video_frame_bridge_addr,
    ten_go_handle_t handler_id) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");

  ten_go_msg_t *video_frame = ten_go_msg_reinterpret(video_frame_bridge_addr);
  TEN_ASSERT(video_frame && ten_go_msg_check_integrity(video_frame),
             "Should not happen.");

  ten_go_error_t cgo_error;
  ten_go_error_init_with_errno(&cgo_error, TEN_ERRNO_OK);

  TEN_GO_TEN_ENV_IS_ALIVE_REGION_BEGIN(
      self, { ten_go_error_set_errno(&cgo_error, TEN_ERRNO_TEN_IS_CLOSED); });

  ten_error_t err;
  ten_error_init(&err);

  ten_env_notify_send_video_frame_info_t *notify_info =
      ten_env_notify_send_video_frame_info_create(
          ten_go_msg_move_c_msg(video_frame),
          handler_id <= 0 ? TEN_GO_NO_RESPONSE_HANDLER : handler_id);

  if (!ten_env_proxy_notify(self->c_ten_env_proxy,
                            ten_env_proxy_notify_send_video_frame, notify_info,
                            false, &err)) {
    ten_env_notify_send_video_frame_info_destroy(notify_info);
    ten_go_error_from_error(&cgo_error, &err);
  }

  TEN_GO_TEN_ENV_IS_ALIVE_REGION_END(self);
  ten_error_deinit(&err);

ten_is_close:
  return cgo_error;
}
