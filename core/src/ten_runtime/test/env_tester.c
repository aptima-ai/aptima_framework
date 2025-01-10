//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/test/env_tester.h"

#include <inttypes.h>

#include "include_internal/ten_runtime/app/app.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd_base.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "include_internal/ten_runtime/ten_env_proxy/ten_env_proxy.h"
#include "include_internal/ten_runtime/test/extension_tester.h"
#include "ten_runtime/app/app.h"
#include "ten_runtime/extension/extension.h"
#include "ten_runtime/msg/cmd/close_app/cmd.h"
#include "ten_runtime/ten_env_proxy/ten_env_proxy.h"
#include "ten_runtime/test/env_tester.h"
#include "ten_utils/io/runloop.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/lib/signature.h"
#include "ten_utils/macro/mark.h"
#include "ten_utils/macro/memory.h"

bool ten_env_tester_check_integrity(ten_env_tester_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  if (ten_signature_get(&self->signature) !=
      (ten_signature_t)TEN_ENV_TESTER_SIGNATURE) {
    TEN_ASSERT(0, "Failed to pass ten_env_tester signature checking: %" PRId64,
               self->signature);
    return false;
  }

  // TODO(Wei): Currently, all calls to ten_env_tester must be made within the
  // tester thread. If we need to call the ten_env_tester API from a non-tester
  // thread, a mechanism similar to ten_env_tester_proxy must be created.
  if (!ten_sanitizer_thread_check_do_check(&self->tester->thread_check)) {
    return false;
  }

  return true;
}

ten_env_tester_t *ten_env_tester_create(ten_extension_tester_t *tester) {
  TEN_ASSERT(tester && ten_extension_tester_check_integrity(tester, true),
             "Invalid argument.");

  ten_env_tester_t *self = TEN_MALLOC(sizeof(ten_env_tester_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  self->binding_handle.me_in_target_lang = NULL;

  ten_signature_set(&self->signature, TEN_ENV_TESTER_SIGNATURE);

  self->tester = tester;

  self->close_handler = NULL;
  self->destroy_handler = NULL;

  return self;
}

void ten_env_tester_destroy(ten_env_tester_t *self) {
  TEN_ASSERT(self && ten_env_tester_check_integrity(self), "Invalid argument.");

  if (self->destroy_handler && self->binding_handle.me_in_target_lang) {
    self->destroy_handler(self->binding_handle.me_in_target_lang);
  }

  TEN_FREE(self);
}

typedef struct ten_extension_tester_send_cmd_info_t {
  ten_extension_tester_t *tester;
  ten_shared_ptr_t *cmd;
  ten_shared_ptr_t *cmd_result;
  ten_env_tester_cmd_result_handler_func_t handler;
  void *handler_user_data;
  ten_error_t *err;
} ten_env_tester_send_cmd_ctx_t;

typedef struct ten_extension_tester_send_msg_info_t {
  ten_extension_tester_t *tester;
  ten_shared_ptr_t *msg;
  ten_env_tester_error_handler_func_t handler;
  void *handler_user_data;
  ten_error_t *err;
} ten_env_tester_send_msg_ctx_t;

typedef struct ten_extension_tester_return_result_info_t {
  ten_extension_tester_t *tester;
  ten_shared_ptr_t *result;
  ten_shared_ptr_t *target_cmd;
  ten_env_tester_error_handler_func_t handler;
  void *handler_user_data;
  ten_error_t *err;
} ten_env_tester_return_result_ctx_t;

static ten_env_tester_send_cmd_ctx_t *ten_extension_tester_send_cmd_ctx_create(
    ten_extension_tester_t *tester, ten_shared_ptr_t *cmd,
    ten_env_tester_cmd_result_handler_func_t handler, void *handler_user_data) {
  TEN_ASSERT(
      tester && ten_extension_tester_check_integrity(tester, true) && cmd,
      "Invalid argument.");

  ten_env_tester_send_cmd_ctx_t *self =
      TEN_MALLOC(sizeof(ten_env_tester_send_cmd_ctx_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  self->tester = tester;
  self->cmd = cmd;
  self->cmd_result = NULL;
  self->handler = handler;
  self->handler_user_data = handler_user_data;
  self->err = NULL;

  return self;
}

static void ten_extension_tester_send_cmd_ctx_destroy(
    ten_env_tester_send_cmd_ctx_t *self) {
  TEN_ASSERT(self, "Invalid argument.");

  if (self->cmd) {
    ten_shared_ptr_destroy(self->cmd);
  }

  if (self->cmd_result) {
    ten_shared_ptr_destroy(self->cmd_result);
  }

  if (self->err) {
    ten_error_destroy(self->err);
  }

  TEN_FREE(self);
}

static ten_env_tester_send_msg_ctx_t *ten_extension_tester_send_msg_ctx_create(
    ten_extension_tester_t *tester, ten_shared_ptr_t *msg,
    ten_env_tester_error_handler_func_t handler, void *handler_user_data) {
  TEN_ASSERT(
      tester && ten_extension_tester_check_integrity(tester, true) && msg,
      "Invalid argument.");

  ten_env_tester_send_msg_ctx_t *self =
      TEN_MALLOC(sizeof(ten_env_tester_send_msg_ctx_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  self->tester = tester;
  self->msg = msg;
  self->handler = handler;
  self->handler_user_data = handler_user_data;
  self->err = NULL;

  return self;
}

static void ten_extension_tester_send_msg_ctx_destroy(
    ten_env_tester_send_msg_ctx_t *self) {
  TEN_ASSERT(self, "Invalid argument.");

  if (self->msg) {
    ten_shared_ptr_destroy(self->msg);
  }

  if (self->err) {
    ten_error_destroy(self->err);
  }

  TEN_FREE(self);
}

static ten_env_tester_return_result_ctx_t *
ten_extension_tester_return_result_ctx_create(
    ten_extension_tester_t *tester, ten_shared_ptr_t *result,
    ten_shared_ptr_t *target_cmd, ten_env_tester_error_handler_func_t handler,
    void *handler_user_data) {
  TEN_ASSERT(tester && ten_extension_tester_check_integrity(tester, true) &&
                 result && target_cmd,
             "Invalid argument.");

  ten_env_tester_return_result_ctx_t *self =
      TEN_MALLOC(sizeof(ten_env_tester_return_result_ctx_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  self->tester = tester;
  self->result = result;
  self->target_cmd = target_cmd;
  self->handler = handler;
  self->handler_user_data = handler_user_data;
  self->err = NULL;

  return self;
}

static void ten_extension_tester_return_result_ctx_destroy(
    ten_env_tester_return_result_ctx_t *self) {
  TEN_ASSERT(self, "Invalid argument.");

  if (self->result) {
    ten_shared_ptr_destroy(self->result);
  }

  if (self->target_cmd) {
    ten_shared_ptr_destroy(self->target_cmd);
  }

  if (self->err) {
    ten_error_destroy(self->err);
  }

  TEN_FREE(self);
}

static void ten_extension_tester_execute_error_handler_task(void *self,
                                                            void *arg) {
  ten_extension_tester_t *tester = self;
  TEN_ASSERT(tester && ten_extension_tester_check_integrity(tester, true),
             "Invalid argument.");

  ten_env_tester_send_msg_ctx_t *send_msg_info = arg;
  TEN_ASSERT(send_msg_info, "Invalid argument.");

  send_msg_info->handler(tester->ten_env_tester,
                         send_msg_info->handler_user_data, send_msg_info->err);

  ten_extension_tester_send_msg_ctx_destroy(send_msg_info);
}

static void ten_extension_tester_execute_cmd_result_handler_task(void *self,
                                                                 void *arg) {
  ten_extension_tester_t *tester = self;
  TEN_ASSERT(tester && ten_extension_tester_check_integrity(tester, true),
             "Invalid argument.");

  ten_env_tester_send_cmd_ctx_t *send_cmd_info = arg;
  TEN_ASSERT(send_cmd_info, "Invalid argument.");

  send_cmd_info->handler(tester->ten_env_tester, send_cmd_info->cmd_result,
                         send_cmd_info->handler_user_data, send_cmd_info->err);

  ten_extension_tester_send_cmd_ctx_destroy(send_cmd_info);
}

static void ten_extension_tester_execute_return_result_handler_task(void *self,
                                                                    void *arg) {
  ten_extension_tester_t *tester = self;
  TEN_ASSERT(tester && ten_extension_tester_check_integrity(tester, true),
             "Invalid argument.");

  ten_env_tester_return_result_ctx_t *return_result_info = arg;
  TEN_ASSERT(return_result_info, "Invalid argument.");

  return_result_info->handler(tester->ten_env_tester,
                              return_result_info->handler_user_data,
                              return_result_info->err);

  ten_extension_tester_return_result_ctx_destroy(return_result_info);
}

static void send_cmd_callback(ten_env_t *ten_env, ten_shared_ptr_t *cmd_result,
                              void *callback_user_data, ten_error_t *err) {
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");
  ten_env_tester_send_cmd_ctx_t *send_cmd_info = callback_user_data;
  TEN_ASSERT(send_cmd_info, "Invalid argument.");

  if (!send_cmd_info->handler) {
    ten_extension_tester_send_cmd_ctx_destroy(send_cmd_info);
    return;
  }

  if (err) {
    ten_error_t *err_clone = ten_error_create();
    ten_error_copy(err_clone, err);

    send_cmd_info->err = err_clone;
    // Inject cmd result into the extension_tester thread to ensure thread
    // safety.
    ten_runloop_post_task_tail(
        send_cmd_info->tester->tester_runloop,
        ten_extension_tester_execute_cmd_result_handler_task,
        send_cmd_info->tester, send_cmd_info);
  } else {
    TEN_ASSERT(cmd_result && ten_cmd_base_check_integrity(cmd_result),
               "Should not happen.");

    send_cmd_info->cmd_result = ten_shared_ptr_clone(cmd_result);
    // Inject cmd result into the extension_tester thread to ensure thread
    // safety.
    ten_runloop_post_task_tail(
        send_cmd_info->tester->tester_runloop,
        ten_extension_tester_execute_cmd_result_handler_task,
        send_cmd_info->tester, send_cmd_info);
  }
}

static void send_data_like_msg_callback(ten_env_t *ten_env,
                                        TEN_UNUSED ten_shared_ptr_t *cmd_result,
                                        void *callback_user_data,
                                        ten_error_t *err) {
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_env_tester_send_msg_ctx_t *send_msg_info = callback_user_data;
  TEN_ASSERT(send_msg_info, "Invalid argument.");

  if (!send_msg_info->handler) {
    ten_extension_tester_send_msg_ctx_destroy(send_msg_info);
    return;
  }

  if (err) {
    ten_error_t *err_clone = ten_error_create();
    ten_error_copy(err_clone, err);

    send_msg_info->err = err_clone;
  }

  ten_runloop_post_task_tail(send_msg_info->tester->tester_runloop,
                             ten_extension_tester_execute_error_handler_task,
                             send_msg_info->tester, send_msg_info);
}

static void return_result_callback(ten_env_t *self, void *user_data,
                                   ten_error_t *err) {
  TEN_ASSERT(self && ten_env_check_integrity(self, true), "Should not happen.");

  ten_env_tester_return_result_ctx_t *return_result_info = user_data;
  TEN_ASSERT(return_result_info, "Invalid argument.");

  if (!return_result_info->handler) {
    ten_extension_tester_return_result_ctx_destroy(return_result_info);
    return;
  }

  if (err) {
    ten_error_t *err_clone = ten_error_create();
    ten_error_copy(err_clone, err);

    return_result_info->err = err_clone;
  }

  ten_runloop_post_task_tail(
      return_result_info->tester->tester_runloop,
      ten_extension_tester_execute_return_result_handler_task,
      return_result_info->tester, return_result_info);
}

static void test_extension_ten_env_send_cmd(ten_env_t *ten_env,
                                            void *user_data) {
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_env_tester_send_cmd_ctx_t *send_cmd_info = user_data;
  TEN_ASSERT(send_cmd_info, "Invalid argument.");
  TEN_ASSERT(send_cmd_info->cmd && ten_msg_check_integrity(send_cmd_info->cmd),
             "Should not happen.");

  ten_error_t *err = ten_error_create();

  bool rc = ten_env_send_cmd(ten_env, send_cmd_info->cmd, send_cmd_callback,
                             send_cmd_info, err);
  if (!rc) {
    if (send_cmd_info->handler) {
      // Move the error to the send_cmd_info.
      send_cmd_info->err = err;
      err = NULL;

      // Inject cmd result into the extension_tester thread to ensure thread
      // safety.
      ten_runloop_post_task_tail(
          send_cmd_info->tester->tester_runloop,
          ten_extension_tester_execute_cmd_result_handler_task,
          send_cmd_info->tester, send_cmd_info);
    }
  }

  if (err) {
    ten_error_destroy(err);
  }

  if (!rc) {
    ten_extension_tester_send_cmd_ctx_destroy(send_cmd_info);
  }
}

static void test_extension_ten_env_return_result(ten_env_t *ten_env,
                                                 void *user_data) {
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_env_tester_return_result_ctx_t *return_result_info = user_data;
  TEN_ASSERT(return_result_info, "Invalid argument.");

  ten_error_t *err = ten_error_create();

  bool rc = ten_env_return_result(
      ten_env, return_result_info->result, return_result_info->target_cmd,
      return_result_callback, return_result_info, err);
  if (!rc) {
    if (return_result_info->handler) {
      // Move the error to the return_result_info.
      return_result_info->err = err;
      err = NULL;

      // Inject cmd result into the extension_tester thread to ensure thread
      // safety.
      ten_runloop_post_task_tail(
          return_result_info->tester->tester_runloop,
          ten_extension_tester_execute_return_result_handler_task,
          return_result_info->tester, return_result_info);
    }
  }

  if (err) {
    ten_error_destroy(err);
  }

  if (!rc) {
    ten_extension_tester_return_result_ctx_destroy(return_result_info);
  }
}

static void test_extension_ten_env_send_data(ten_env_t *ten_env,
                                             void *user_data) {
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_env_tester_send_msg_ctx_t *send_msg_info = user_data;
  TEN_ASSERT(send_msg_info, "Invalid argument.");
  TEN_ASSERT(send_msg_info->msg && ten_msg_check_integrity(send_msg_info->msg),
             "Should not happen.");

  ten_error_t *err = ten_error_create();

  bool rc = ten_env_send_data(ten_env, send_msg_info->msg,
                              send_data_like_msg_callback, send_msg_info, err);
  if (!rc) {
    if (send_msg_info->handler) {
      // Move the error to the send_msg_info.
      send_msg_info->err = err;
      err = NULL;

      // Inject cmd result into the extension_tester thread to ensure thread
      // safety.
      ten_runloop_post_task_tail(
          send_msg_info->tester->tester_runloop,
          ten_extension_tester_execute_error_handler_task,
          send_msg_info->tester, send_msg_info);
    }
  }

  if (err) {
    ten_error_destroy(err);
  }

  if (!rc) {
    ten_extension_tester_send_msg_ctx_destroy(send_msg_info);
  }
}

static void test_extension_ten_env_send_audio_frame(ten_env_t *ten_env,
                                                    void *user_audio_frame) {
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_env_tester_send_msg_ctx_t *send_msg_info = user_audio_frame;
  TEN_ASSERT(send_msg_info, "Invalid argument.");
  TEN_ASSERT(send_msg_info->msg && ten_msg_check_integrity(send_msg_info->msg),
             "Should not happen.");

  ten_error_t *err = ten_error_create();

  bool rc =
      ten_env_send_audio_frame(ten_env, send_msg_info->msg,
                               send_data_like_msg_callback, send_msg_info, err);
  if (!rc) {
    if (send_msg_info->handler) {
      // Move the error to the send_msg_info.
      send_msg_info->err = err;
      err = NULL;

      // Inject cmd result into the extension_tester thread to ensure thread
      // safety.
      ten_runloop_post_task_tail(
          send_msg_info->tester->tester_runloop,
          ten_extension_tester_execute_error_handler_task,
          send_msg_info->tester, send_msg_info);
    }
  }

  if (err) {
    ten_error_destroy(err);
  }

  if (!rc) {
    ten_extension_tester_send_msg_ctx_destroy(send_msg_info);
  }
}

static void test_extension_ten_env_send_video_frame(ten_env_t *ten_env,
                                                    void *user_video_frame) {
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_env_tester_send_msg_ctx_t *send_msg_info = user_video_frame;
  TEN_ASSERT(send_msg_info, "Invalid argument.");
  TEN_ASSERT(send_msg_info->msg && ten_msg_check_integrity(send_msg_info->msg),
             "Should not happen.");

  ten_error_t *err = ten_error_create();

  bool rc =
      ten_env_send_video_frame(ten_env, send_msg_info->msg,
                               send_data_like_msg_callback, send_msg_info, err);
  if (!rc) {
    if (send_msg_info->handler) {
      // Move the error to the send_msg_info.
      send_msg_info->err = err;
      err = NULL;

      // Inject cmd result into the extension_tester thread to ensure thread
      // safety.
      ten_runloop_post_task_tail(
          send_msg_info->tester->tester_runloop,
          ten_extension_tester_execute_error_handler_task,
          send_msg_info->tester, send_msg_info);
    }
  }

  if (err) {
    ten_error_destroy(err);
  }

  if (!rc) {
    ten_extension_tester_send_msg_ctx_destroy(send_msg_info);
  }
}

bool ten_env_tester_send_cmd(ten_env_tester_t *self, ten_shared_ptr_t *cmd,
                             ten_env_tester_cmd_result_handler_func_t handler,
                             void *user_data, ten_error_t *err) {
  TEN_ASSERT(self && ten_env_tester_check_integrity(self), "Invalid argument.");
  ten_env_tester_send_cmd_ctx_t *send_cmd_info =
      ten_extension_tester_send_cmd_ctx_create(
          self->tester, ten_shared_ptr_clone(cmd), handler, user_data);
  TEN_ASSERT(self->tester->test_extension_ten_env_proxy, "Invalid argument.");

  bool rc = ten_env_proxy_notify(self->tester->test_extension_ten_env_proxy,
                                 test_extension_ten_env_send_cmd, send_cmd_info,
                                 false, err);
  if (!rc) {
    ten_extension_tester_send_cmd_ctx_destroy(send_cmd_info);
  }

  return rc;
}

bool ten_env_tester_return_result(
    ten_env_tester_t *self, ten_shared_ptr_t *result,
    ten_shared_ptr_t *target_cmd,
    ten_env_tester_error_handler_func_t error_handler, void *user_data,
    ten_error_t *error) {
  TEN_ASSERT(self && ten_env_tester_check_integrity(self), "Invalid argument.");

  ten_env_tester_return_result_ctx_t *return_result_info =
      ten_extension_tester_return_result_ctx_create(
          self->tester, ten_shared_ptr_clone(result),
          ten_shared_ptr_clone(target_cmd), error_handler, user_data);
  TEN_ASSERT(return_result_info, "Allocation failed.");

  TEN_ASSERT(self->tester->test_extension_ten_env_proxy, "Invalid argument.");
  bool rc = ten_env_proxy_notify(self->tester->test_extension_ten_env_proxy,
                                 test_extension_ten_env_return_result,
                                 return_result_info, false, error);
  if (!rc) {
    ten_extension_tester_return_result_ctx_destroy(return_result_info);
  }

  return rc;
}

bool ten_env_tester_send_data(ten_env_tester_t *self, ten_shared_ptr_t *data,
                              ten_env_tester_error_handler_func_t handler,
                              void *user_data, ten_error_t *err) {
  TEN_ASSERT(self && ten_env_tester_check_integrity(self), "Invalid argument.");

  ten_env_tester_send_msg_ctx_t *send_msg_info =
      ten_extension_tester_send_msg_ctx_create(
          self->tester, ten_shared_ptr_clone(data), handler, user_data);

  TEN_ASSERT(self->tester->test_extension_ten_env_proxy, "Invalid argument.");
  bool rc = ten_env_proxy_notify(self->tester->test_extension_ten_env_proxy,
                                 test_extension_ten_env_send_data,
                                 send_msg_info, false, err);
  if (!rc) {
    ten_extension_tester_send_msg_ctx_destroy(send_msg_info);
  }

  return rc;
}

bool ten_env_tester_send_audio_frame(
    ten_env_tester_t *self, ten_shared_ptr_t *audio_frame,
    ten_env_tester_error_handler_func_t handler, void *user_data,
    ten_error_t *err) {
  TEN_ASSERT(self && ten_env_tester_check_integrity(self), "Invalid argument.");

  ten_env_tester_send_msg_ctx_t *send_msg_info =
      ten_extension_tester_send_msg_ctx_create(
          self->tester, ten_shared_ptr_clone(audio_frame), handler, user_data);

  TEN_ASSERT(self->tester->test_extension_ten_env_proxy, "Invalid argument.");
  bool rc = ten_env_proxy_notify(self->tester->test_extension_ten_env_proxy,
                                 test_extension_ten_env_send_audio_frame,
                                 send_msg_info, false, err);
  if (!rc) {
    ten_extension_tester_send_msg_ctx_destroy(send_msg_info);
  }

  return rc;
}

bool ten_env_tester_send_video_frame(
    ten_env_tester_t *self, ten_shared_ptr_t *video_frame,
    ten_env_tester_error_handler_func_t handler, void *user_data,
    ten_error_t *err) {
  TEN_ASSERT(self && ten_env_tester_check_integrity(self), "Invalid argument.");

  ten_env_tester_send_msg_ctx_t *send_msg_info =
      ten_extension_tester_send_msg_ctx_create(
          self->tester, ten_shared_ptr_clone(video_frame), handler, user_data);

  TEN_ASSERT(self->tester->test_extension_ten_env_proxy, "Invalid argument.");
  bool rc = ten_env_proxy_notify(self->tester->test_extension_ten_env_proxy,
                                 test_extension_ten_env_send_video_frame,
                                 send_msg_info, false, err);
  if (!rc) {
    ten_extension_tester_send_msg_ctx_destroy(send_msg_info);
  }

  return rc;
}

static void test_app_ten_env_send_close_app_cmd(ten_env_t *ten_env,
                                                void *user_data) {
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_app_t *app = ten_env->attached_target.app;
  TEN_ASSERT(app && ten_app_check_integrity(app, true), "Should not happen.");

  ten_shared_ptr_t *close_app_cmd = ten_cmd_close_app_create();
  TEN_ASSERT(close_app_cmd, "Should not happen.");

  bool rc = ten_msg_clear_and_set_dest(close_app_cmd, ten_app_get_uri(app),
                                       NULL, NULL, NULL, NULL);
  TEN_ASSERT(rc, "Should not happen.");

  rc = ten_env_send_cmd(ten_env, close_app_cmd, NULL, NULL, NULL);
  TEN_ASSERT(rc, "Should not happen.");
}

bool ten_env_tester_stop_test(ten_env_tester_t *self, ten_error_t *err) {
  TEN_ASSERT(self && ten_env_tester_check_integrity(self), "Invalid argument.");
  return ten_env_proxy_notify(self->tester->test_app_ten_env_proxy,
                              test_app_ten_env_send_close_app_cmd, NULL, false,
                              err);
}

bool ten_env_tester_on_start_done(ten_env_tester_t *self, ten_error_t *err) {
  TEN_ASSERT(self && ten_env_tester_check_integrity(self), "Invalid argument.");

  ten_extension_tester_on_start_done(self->tester);

  return true;
}

void ten_env_tester_set_close_handler_in_target_lang(
    ten_env_tester_t *self,
    ten_env_tester_close_handler_in_target_lang_func_t close_handler) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_env_tester_check_integrity(self),
             "Invalid use of ten_env_tester %p.", self);

  self->close_handler = close_handler;
}

void ten_env_tester_set_destroy_handler_in_target_lang(
    ten_env_tester_t *self,
    ten_env_tester_destroy_handler_in_target_lang_func_t destroy_handler) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_env_tester_check_integrity(self),
             "Invalid use of ten_env_tester %p.", self);

  self->destroy_handler = destroy_handler;
}
