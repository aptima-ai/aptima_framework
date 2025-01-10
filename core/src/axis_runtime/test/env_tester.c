//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/test/env_tester.h"

#include <inttypes.h>

#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd_base.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "include_internal/axis_runtime/axis_env_proxy/axis_env_proxy.h"
#include "include_internal/axis_runtime/test/extension_tester.h"
#include "axis_runtime/app/app.h"
#include "axis_runtime/extension/extension.h"
#include "axis_runtime/msg/cmd/close_app/cmd.h"
#include "axis_runtime/axis_env_proxy/axis_env_proxy.h"
#include "axis_runtime/test/env_tester.h"
#include "axis_utils/io/runloop.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/macro/mark.h"
#include "axis_utils/macro/memory.h"

bool axis_env_tester_check_integrity(axis_env_tester_t *self) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_ENV_TESTER_SIGNATURE) {
    axis_ASSERT(0, "Failed to pass axis_env_tester signature checking: %" PRId64,
               self->signature);
    return false;
  }

  // TODO(Wei): Currently, all calls to axis_env_tester must be made within the
  // tester thread. If we need to call the axis_env_tester API from a non-tester
  // thread, a mechanism similar to axis_env_tester_proxy must be created.
  if (!axis_sanitizer_thread_check_do_check(&self->tester->thread_check)) {
    return false;
  }

  return true;
}

axis_env_tester_t *axis_env_tester_create(axis_extension_tester_t *tester) {
  axis_ASSERT(tester && axis_extension_tester_check_integrity(tester, true),
             "Invalid argument.");

  axis_env_tester_t *self = axis_MALLOC(sizeof(axis_env_tester_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  self->binding_handle.me_in_target_lang = NULL;

  axis_signature_set(&self->signature, axis_ENV_TESTER_SIGNATURE);

  self->tester = tester;

  self->close_handler = NULL;
  self->destroy_handler = NULL;

  return self;
}

void axis_env_tester_destroy(axis_env_tester_t *self) {
  axis_ASSERT(self && axis_env_tester_check_integrity(self), "Invalid argument.");

  if (self->destroy_handler && self->binding_handle.me_in_target_lang) {
    self->destroy_handler(self->binding_handle.me_in_target_lang);
  }

  axis_FREE(self);
}

typedef struct axis_extension_tester_send_cmd_info_t {
  axis_extension_tester_t *tester;
  axis_shared_ptr_t *cmd;
  axis_shared_ptr_t *cmd_result;
  axis_env_tester_cmd_result_handler_func_t handler;
  void *handler_user_data;
  axis_error_t *err;
} axis_env_tester_send_cmd_ctx_t;

typedef struct axis_extension_tester_send_msg_info_t {
  axis_extension_tester_t *tester;
  axis_shared_ptr_t *msg;
  axis_env_tester_error_handler_func_t handler;
  void *handler_user_data;
  axis_error_t *err;
} axis_env_tester_send_msg_ctx_t;

typedef struct axis_extension_tester_return_result_info_t {
  axis_extension_tester_t *tester;
  axis_shared_ptr_t *result;
  axis_shared_ptr_t *target_cmd;
  axis_env_tester_error_handler_func_t handler;
  void *handler_user_data;
  axis_error_t *err;
} axis_env_tester_return_result_ctx_t;

static axis_env_tester_send_cmd_ctx_t *axis_extension_tester_send_cmd_ctx_create(
    axis_extension_tester_t *tester, axis_shared_ptr_t *cmd,
    axis_env_tester_cmd_result_handler_func_t handler, void *handler_user_data) {
  axis_ASSERT(
      tester && axis_extension_tester_check_integrity(tester, true) && cmd,
      "Invalid argument.");

  axis_env_tester_send_cmd_ctx_t *self =
      axis_MALLOC(sizeof(axis_env_tester_send_cmd_ctx_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  self->tester = tester;
  self->cmd = cmd;
  self->cmd_result = NULL;
  self->handler = handler;
  self->handler_user_data = handler_user_data;
  self->err = NULL;

  return self;
}

static void axis_extension_tester_send_cmd_ctx_destroy(
    axis_env_tester_send_cmd_ctx_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  if (self->cmd) {
    axis_shared_ptr_destroy(self->cmd);
  }

  if (self->cmd_result) {
    axis_shared_ptr_destroy(self->cmd_result);
  }

  if (self->err) {
    axis_error_destroy(self->err);
  }

  axis_FREE(self);
}

static axis_env_tester_send_msg_ctx_t *axis_extension_tester_send_msg_ctx_create(
    axis_extension_tester_t *tester, axis_shared_ptr_t *msg,
    axis_env_tester_error_handler_func_t handler, void *handler_user_data) {
  axis_ASSERT(
      tester && axis_extension_tester_check_integrity(tester, true) && msg,
      "Invalid argument.");

  axis_env_tester_send_msg_ctx_t *self =
      axis_MALLOC(sizeof(axis_env_tester_send_msg_ctx_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  self->tester = tester;
  self->msg = msg;
  self->handler = handler;
  self->handler_user_data = handler_user_data;
  self->err = NULL;

  return self;
}

static void axis_extension_tester_send_msg_ctx_destroy(
    axis_env_tester_send_msg_ctx_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  if (self->msg) {
    axis_shared_ptr_destroy(self->msg);
  }

  if (self->err) {
    axis_error_destroy(self->err);
  }

  axis_FREE(self);
}

static axis_env_tester_return_result_ctx_t *
axis_extension_tester_return_result_ctx_create(
    axis_extension_tester_t *tester, axis_shared_ptr_t *result,
    axis_shared_ptr_t *target_cmd, axis_env_tester_error_handler_func_t handler,
    void *handler_user_data) {
  axis_ASSERT(tester && axis_extension_tester_check_integrity(tester, true) &&
                 result && target_cmd,
             "Invalid argument.");

  axis_env_tester_return_result_ctx_t *self =
      axis_MALLOC(sizeof(axis_env_tester_return_result_ctx_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  self->tester = tester;
  self->result = result;
  self->target_cmd = target_cmd;
  self->handler = handler;
  self->handler_user_data = handler_user_data;
  self->err = NULL;

  return self;
}

static void axis_extension_tester_return_result_ctx_destroy(
    axis_env_tester_return_result_ctx_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  if (self->result) {
    axis_shared_ptr_destroy(self->result);
  }

  if (self->target_cmd) {
    axis_shared_ptr_destroy(self->target_cmd);
  }

  if (self->err) {
    axis_error_destroy(self->err);
  }

  axis_FREE(self);
}

static void axis_extension_tester_execute_error_handler_task(void *self,
                                                            void *arg) {
  axis_extension_tester_t *tester = self;
  axis_ASSERT(tester && axis_extension_tester_check_integrity(tester, true),
             "Invalid argument.");

  axis_env_tester_send_msg_ctx_t *send_msg_info = arg;
  axis_ASSERT(send_msg_info, "Invalid argument.");

  send_msg_info->handler(tester->axis_env_tester,
                         send_msg_info->handler_user_data, send_msg_info->err);

  axis_extension_tester_send_msg_ctx_destroy(send_msg_info);
}

static void axis_extension_tester_execute_cmd_result_handler_task(void *self,
                                                                 void *arg) {
  axis_extension_tester_t *tester = self;
  axis_ASSERT(tester && axis_extension_tester_check_integrity(tester, true),
             "Invalid argument.");

  axis_env_tester_send_cmd_ctx_t *send_cmd_info = arg;
  axis_ASSERT(send_cmd_info, "Invalid argument.");

  send_cmd_info->handler(tester->axis_env_tester, send_cmd_info->cmd_result,
                         send_cmd_info->handler_user_data, send_cmd_info->err);

  axis_extension_tester_send_cmd_ctx_destroy(send_cmd_info);
}

static void axis_extension_tester_execute_return_result_handler_task(void *self,
                                                                    void *arg) {
  axis_extension_tester_t *tester = self;
  axis_ASSERT(tester && axis_extension_tester_check_integrity(tester, true),
             "Invalid argument.");

  axis_env_tester_return_result_ctx_t *return_result_info = arg;
  axis_ASSERT(return_result_info, "Invalid argument.");

  return_result_info->handler(tester->axis_env_tester,
                              return_result_info->handler_user_data,
                              return_result_info->err);

  axis_extension_tester_return_result_ctx_destroy(return_result_info);
}

static void send_cmd_callback(axis_env_t *axis_env, axis_shared_ptr_t *cmd_result,
                              void *callback_user_data, axis_error_t *err) {
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");
  axis_env_tester_send_cmd_ctx_t *send_cmd_info = callback_user_data;
  axis_ASSERT(send_cmd_info, "Invalid argument.");

  if (!send_cmd_info->handler) {
    axis_extension_tester_send_cmd_ctx_destroy(send_cmd_info);
    return;
  }

  if (err) {
    axis_error_t *err_clone = axis_error_create();
    axis_error_copy(err_clone, err);

    send_cmd_info->err = err_clone;
    // Inject cmd result into the extension_tester thread to ensure thread
    // safety.
    axis_runloop_post_task_tail(
        send_cmd_info->tester->tester_runloop,
        axis_extension_tester_execute_cmd_result_handler_task,
        send_cmd_info->tester, send_cmd_info);
  } else {
    axis_ASSERT(cmd_result && axis_cmd_base_check_integrity(cmd_result),
               "Should not happen.");

    send_cmd_info->cmd_result = axis_shared_ptr_clone(cmd_result);
    // Inject cmd result into the extension_tester thread to ensure thread
    // safety.
    axis_runloop_post_task_tail(
        send_cmd_info->tester->tester_runloop,
        axis_extension_tester_execute_cmd_result_handler_task,
        send_cmd_info->tester, send_cmd_info);
  }
}

static void send_data_like_msg_callback(axis_env_t *axis_env,
                                        axis_UNUSED axis_shared_ptr_t *cmd_result,
                                        void *callback_user_data,
                                        axis_error_t *err) {
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_env_tester_send_msg_ctx_t *send_msg_info = callback_user_data;
  axis_ASSERT(send_msg_info, "Invalid argument.");

  if (!send_msg_info->handler) {
    axis_extension_tester_send_msg_ctx_destroy(send_msg_info);
    return;
  }

  if (err) {
    axis_error_t *err_clone = axis_error_create();
    axis_error_copy(err_clone, err);

    send_msg_info->err = err_clone;
  }

  axis_runloop_post_task_tail(send_msg_info->tester->tester_runloop,
                             axis_extension_tester_execute_error_handler_task,
                             send_msg_info->tester, send_msg_info);
}

static void return_result_callback(axis_env_t *self, void *user_data,
                                   axis_error_t *err) {
  axis_ASSERT(self && axis_env_check_integrity(self, true), "Should not happen.");

  axis_env_tester_return_result_ctx_t *return_result_info = user_data;
  axis_ASSERT(return_result_info, "Invalid argument.");

  if (!return_result_info->handler) {
    axis_extension_tester_return_result_ctx_destroy(return_result_info);
    return;
  }

  if (err) {
    axis_error_t *err_clone = axis_error_create();
    axis_error_copy(err_clone, err);

    return_result_info->err = err_clone;
  }

  axis_runloop_post_task_tail(
      return_result_info->tester->tester_runloop,
      axis_extension_tester_execute_return_result_handler_task,
      return_result_info->tester, return_result_info);
}

static void test_extension_axis_env_send_cmd(axis_env_t *axis_env,
                                            void *user_data) {
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_env_tester_send_cmd_ctx_t *send_cmd_info = user_data;
  axis_ASSERT(send_cmd_info, "Invalid argument.");
  axis_ASSERT(send_cmd_info->cmd && axis_msg_check_integrity(send_cmd_info->cmd),
             "Should not happen.");

  axis_error_t *err = axis_error_create();

  bool rc = axis_env_send_cmd(axis_env, send_cmd_info->cmd, send_cmd_callback,
                             send_cmd_info, err);
  if (!rc) {
    if (send_cmd_info->handler) {
      // Move the error to the send_cmd_info.
      send_cmd_info->err = err;
      err = NULL;

      // Inject cmd result into the extension_tester thread to ensure thread
      // safety.
      axis_runloop_post_task_tail(
          send_cmd_info->tester->tester_runloop,
          axis_extension_tester_execute_cmd_result_handler_task,
          send_cmd_info->tester, send_cmd_info);
    }
  }

  if (err) {
    axis_error_destroy(err);
  }

  if (!rc) {
    axis_extension_tester_send_cmd_ctx_destroy(send_cmd_info);
  }
}

static void test_extension_axis_env_return_result(axis_env_t *axis_env,
                                                 void *user_data) {
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_env_tester_return_result_ctx_t *return_result_info = user_data;
  axis_ASSERT(return_result_info, "Invalid argument.");

  axis_error_t *err = axis_error_create();

  bool rc = axis_env_return_result(
      axis_env, return_result_info->result, return_result_info->target_cmd,
      return_result_callback, return_result_info, err);
  if (!rc) {
    if (return_result_info->handler) {
      // Move the error to the return_result_info.
      return_result_info->err = err;
      err = NULL;

      // Inject cmd result into the extension_tester thread to ensure thread
      // safety.
      axis_runloop_post_task_tail(
          return_result_info->tester->tester_runloop,
          axis_extension_tester_execute_return_result_handler_task,
          return_result_info->tester, return_result_info);
    }
  }

  if (err) {
    axis_error_destroy(err);
  }

  if (!rc) {
    axis_extension_tester_return_result_ctx_destroy(return_result_info);
  }
}

static void test_extension_axis_env_send_data(axis_env_t *axis_env,
                                             void *user_data) {
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_env_tester_send_msg_ctx_t *send_msg_info = user_data;
  axis_ASSERT(send_msg_info, "Invalid argument.");
  axis_ASSERT(send_msg_info->msg && axis_msg_check_integrity(send_msg_info->msg),
             "Should not happen.");

  axis_error_t *err = axis_error_create();

  bool rc = axis_env_send_data(axis_env, send_msg_info->msg,
                              send_data_like_msg_callback, send_msg_info, err);
  if (!rc) {
    if (send_msg_info->handler) {
      // Move the error to the send_msg_info.
      send_msg_info->err = err;
      err = NULL;

      // Inject cmd result into the extension_tester thread to ensure thread
      // safety.
      axis_runloop_post_task_tail(
          send_msg_info->tester->tester_runloop,
          axis_extension_tester_execute_error_handler_task,
          send_msg_info->tester, send_msg_info);
    }
  }

  if (err) {
    axis_error_destroy(err);
  }

  if (!rc) {
    axis_extension_tester_send_msg_ctx_destroy(send_msg_info);
  }
}

static void test_extension_axis_env_send_audio_frame(axis_env_t *axis_env,
                                                    void *user_audio_frame) {
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_env_tester_send_msg_ctx_t *send_msg_info = user_audio_frame;
  axis_ASSERT(send_msg_info, "Invalid argument.");
  axis_ASSERT(send_msg_info->msg && axis_msg_check_integrity(send_msg_info->msg),
             "Should not happen.");

  axis_error_t *err = axis_error_create();

  bool rc =
      axis_env_send_audio_frame(axis_env, send_msg_info->msg,
                               send_data_like_msg_callback, send_msg_info, err);
  if (!rc) {
    if (send_msg_info->handler) {
      // Move the error to the send_msg_info.
      send_msg_info->err = err;
      err = NULL;

      // Inject cmd result into the extension_tester thread to ensure thread
      // safety.
      axis_runloop_post_task_tail(
          send_msg_info->tester->tester_runloop,
          axis_extension_tester_execute_error_handler_task,
          send_msg_info->tester, send_msg_info);
    }
  }

  if (err) {
    axis_error_destroy(err);
  }

  if (!rc) {
    axis_extension_tester_send_msg_ctx_destroy(send_msg_info);
  }
}

static void test_extension_axis_env_send_video_frame(axis_env_t *axis_env,
                                                    void *user_video_frame) {
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_env_tester_send_msg_ctx_t *send_msg_info = user_video_frame;
  axis_ASSERT(send_msg_info, "Invalid argument.");
  axis_ASSERT(send_msg_info->msg && axis_msg_check_integrity(send_msg_info->msg),
             "Should not happen.");

  axis_error_t *err = axis_error_create();

  bool rc =
      axis_env_send_video_frame(axis_env, send_msg_info->msg,
                               send_data_like_msg_callback, send_msg_info, err);
  if (!rc) {
    if (send_msg_info->handler) {
      // Move the error to the send_msg_info.
      send_msg_info->err = err;
      err = NULL;

      // Inject cmd result into the extension_tester thread to ensure thread
      // safety.
      axis_runloop_post_task_tail(
          send_msg_info->tester->tester_runloop,
          axis_extension_tester_execute_error_handler_task,
          send_msg_info->tester, send_msg_info);
    }
  }

  if (err) {
    axis_error_destroy(err);
  }

  if (!rc) {
    axis_extension_tester_send_msg_ctx_destroy(send_msg_info);
  }
}

bool axis_env_tester_send_cmd(axis_env_tester_t *self, axis_shared_ptr_t *cmd,
                             axis_env_tester_cmd_result_handler_func_t handler,
                             void *user_data, axis_error_t *err) {
  axis_ASSERT(self && axis_env_tester_check_integrity(self), "Invalid argument.");
  axis_env_tester_send_cmd_ctx_t *send_cmd_info =
      axis_extension_tester_send_cmd_ctx_create(
          self->tester, axis_shared_ptr_clone(cmd), handler, user_data);
  axis_ASSERT(self->tester->test_extension_axis_env_proxy, "Invalid argument.");

  bool rc = axis_env_proxy_notify(self->tester->test_extension_axis_env_proxy,
                                 test_extension_axis_env_send_cmd, send_cmd_info,
                                 false, err);
  if (!rc) {
    axis_extension_tester_send_cmd_ctx_destroy(send_cmd_info);
  }

  return rc;
}

bool axis_env_tester_return_result(
    axis_env_tester_t *self, axis_shared_ptr_t *result,
    axis_shared_ptr_t *target_cmd,
    axis_env_tester_error_handler_func_t error_handler, void *user_data,
    axis_error_t *error) {
  axis_ASSERT(self && axis_env_tester_check_integrity(self), "Invalid argument.");

  axis_env_tester_return_result_ctx_t *return_result_info =
      axis_extension_tester_return_result_ctx_create(
          self->tester, axis_shared_ptr_clone(result),
          axis_shared_ptr_clone(target_cmd), error_handler, user_data);
  axis_ASSERT(return_result_info, "Allocation failed.");

  axis_ASSERT(self->tester->test_extension_axis_env_proxy, "Invalid argument.");
  bool rc = axis_env_proxy_notify(self->tester->test_extension_axis_env_proxy,
                                 test_extension_axis_env_return_result,
                                 return_result_info, false, error);
  if (!rc) {
    axis_extension_tester_return_result_ctx_destroy(return_result_info);
  }

  return rc;
}

bool axis_env_tester_send_data(axis_env_tester_t *self, axis_shared_ptr_t *data,
                              axis_env_tester_error_handler_func_t handler,
                              void *user_data, axis_error_t *err) {
  axis_ASSERT(self && axis_env_tester_check_integrity(self), "Invalid argument.");

  axis_env_tester_send_msg_ctx_t *send_msg_info =
      axis_extension_tester_send_msg_ctx_create(
          self->tester, axis_shared_ptr_clone(data), handler, user_data);

  axis_ASSERT(self->tester->test_extension_axis_env_proxy, "Invalid argument.");
  bool rc = axis_env_proxy_notify(self->tester->test_extension_axis_env_proxy,
                                 test_extension_axis_env_send_data,
                                 send_msg_info, false, err);
  if (!rc) {
    axis_extension_tester_send_msg_ctx_destroy(send_msg_info);
  }

  return rc;
}

bool axis_env_tester_send_audio_frame(
    axis_env_tester_t *self, axis_shared_ptr_t *audio_frame,
    axis_env_tester_error_handler_func_t handler, void *user_data,
    axis_error_t *err) {
  axis_ASSERT(self && axis_env_tester_check_integrity(self), "Invalid argument.");

  axis_env_tester_send_msg_ctx_t *send_msg_info =
      axis_extension_tester_send_msg_ctx_create(
          self->tester, axis_shared_ptr_clone(audio_frame), handler, user_data);

  axis_ASSERT(self->tester->test_extension_axis_env_proxy, "Invalid argument.");
  bool rc = axis_env_proxy_notify(self->tester->test_extension_axis_env_proxy,
                                 test_extension_axis_env_send_audio_frame,
                                 send_msg_info, false, err);
  if (!rc) {
    axis_extension_tester_send_msg_ctx_destroy(send_msg_info);
  }

  return rc;
}

bool axis_env_tester_send_video_frame(
    axis_env_tester_t *self, axis_shared_ptr_t *video_frame,
    axis_env_tester_error_handler_func_t handler, void *user_data,
    axis_error_t *err) {
  axis_ASSERT(self && axis_env_tester_check_integrity(self), "Invalid argument.");

  axis_env_tester_send_msg_ctx_t *send_msg_info =
      axis_extension_tester_send_msg_ctx_create(
          self->tester, axis_shared_ptr_clone(video_frame), handler, user_data);

  axis_ASSERT(self->tester->test_extension_axis_env_proxy, "Invalid argument.");
  bool rc = axis_env_proxy_notify(self->tester->test_extension_axis_env_proxy,
                                 test_extension_axis_env_send_video_frame,
                                 send_msg_info, false, err);
  if (!rc) {
    axis_extension_tester_send_msg_ctx_destroy(send_msg_info);
  }

  return rc;
}

static void test_app_axis_env_send_close_app_cmd(axis_env_t *axis_env,
                                                void *user_data) {
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_app_t *app = axis_env->attached_target.app;
  axis_ASSERT(app && axis_app_check_integrity(app, true), "Should not happen.");

  axis_shared_ptr_t *close_app_cmd = axis_cmd_close_app_create();
  axis_ASSERT(close_app_cmd, "Should not happen.");

  bool rc = axis_msg_clear_and_set_dest(close_app_cmd, axis_app_get_uri(app),
                                       NULL, NULL, NULL, NULL);
  axis_ASSERT(rc, "Should not happen.");

  rc = axis_env_send_cmd(axis_env, close_app_cmd, NULL, NULL, NULL);
  axis_ASSERT(rc, "Should not happen.");
}

bool axis_env_tester_stop_test(axis_env_tester_t *self, axis_error_t *err) {
  axis_ASSERT(self && axis_env_tester_check_integrity(self), "Invalid argument.");
  return axis_env_proxy_notify(self->tester->test_app_axis_env_proxy,
                              test_app_axis_env_send_close_app_cmd, NULL, false,
                              err);
}

bool axis_env_tester_on_start_done(axis_env_tester_t *self, axis_error_t *err) {
  axis_ASSERT(self && axis_env_tester_check_integrity(self), "Invalid argument.");

  axis_extension_tester_on_start_done(self->tester);

  return true;
}

void axis_env_tester_set_close_handler_in_target_lang(
    axis_env_tester_t *self,
    axis_env_tester_close_handler_in_target_lang_func_t close_handler) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_env_tester_check_integrity(self),
             "Invalid use of axis_env_tester %p.", self);

  self->close_handler = close_handler;
}

void axis_env_tester_set_destroy_handler_in_target_lang(
    axis_env_tester_t *self,
    axis_env_tester_destroy_handler_in_target_lang_func_t destroy_handler) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_env_tester_check_integrity(self),
             "Invalid use of axis_env_tester %p.", self);

  self->destroy_handler = destroy_handler;
}
