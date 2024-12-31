//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/binding/nodejs/msg/audio_frame.h"
#include "include_internal/ten_runtime/binding/nodejs/ten_env/ten_env.h"
#include "ten_utils/macro/memory.h"

typedef struct ten_env_notify_send_audio_frame_info_t {
  ten_shared_ptr_t *c_audio_frame;
  ten_nodejs_tsfn_t *js_cb;
} ten_env_notify_send_audio_frame_info_t;

typedef struct ten_nodejs_send_audio_frame_callback_call_info_t {
  ten_nodejs_tsfn_t *js_cb;
  ten_error_t *error;
} ten_nodejs_send_audio_frame_callback_call_info_t;

static ten_env_notify_send_audio_frame_info_t *
ten_env_notify_send_audio_frame_info_create(ten_shared_ptr_t *c_audio_frame,
                                            ten_nodejs_tsfn_t *js_cb) {
  ten_env_notify_send_audio_frame_info_t *info =
      TEN_MALLOC(sizeof(ten_env_notify_send_audio_frame_info_t));
  TEN_ASSERT(info, "Failed to allocate memory.");

  info->c_audio_frame = c_audio_frame;
  info->js_cb = js_cb;

  return info;
}

static void ten_env_notify_send_audio_frame_info_destroy(
    ten_env_notify_send_audio_frame_info_t *info) {
  TEN_ASSERT(info, "Invalid argument.");

  if (info->c_audio_frame) {
    ten_shared_ptr_destroy(info->c_audio_frame);
    info->c_audio_frame = NULL;
  }

  info->js_cb = NULL;

  TEN_FREE(info);
}

static ten_nodejs_send_audio_frame_callback_call_info_t *
ten_nodejs_send_audio_frame_callback_call_info_create(ten_nodejs_tsfn_t *js_cb,
                                                      ten_error_t *error) {
  ten_nodejs_send_audio_frame_callback_call_info_t *info =
      TEN_MALLOC(sizeof(ten_nodejs_send_audio_frame_callback_call_info_t));
  TEN_ASSERT(info, "Failed to allocate memory.");

  info->js_cb = js_cb;
  info->error = error;

  return info;
}

static void ten_nodejs_send_audio_frame_callback_call_info_destroy(
    ten_nodejs_send_audio_frame_callback_call_info_t *info) {
  TEN_ASSERT(info, "Invalid argument.");

  if (info->error) {
    ten_error_destroy(info->error);
    info->error = NULL;
  }

  TEN_FREE(info);
}

static void tsfn_proxy_send_audio_frame_callback(napi_env env, napi_value js_cb,
                                                 void *context,
                                                 void *audio_frame) {
  ten_nodejs_send_audio_frame_callback_call_info_t *info = audio_frame;
  TEN_ASSERT(info, "Should not happen.");

  napi_value js_error = NULL;

  if (info->error) {
    js_error = ten_nodejs_create_error(env, info->error);
    ASSERT_IF_NAPI_FAIL(js_error, "Failed to create JS error", NULL);
  } else {
    js_error = js_undefined(env);
  }

  napi_value argv[] = {js_error};
  napi_status status =
      napi_call_function(env, js_undefined(env), js_cb, 1, argv, NULL);
  ASSERT_IF_NAPI_FAIL(status == napi_ok, "Failed to call JS callback", NULL);

  ten_nodejs_tsfn_release(info->js_cb);

  ten_nodejs_send_audio_frame_callback_call_info_destroy(info);
}

static void proxy_send_audio_frame_callback(ten_env_t *ten_env,
                                            ten_shared_ptr_t *msg,
                                            void *user_audio_frame,
                                            ten_error_t *err) {
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_env_notify_send_audio_frame_info_t *info = user_audio_frame;
  TEN_ASSERT(info, "Should not happen.");

  ten_nodejs_tsfn_t *js_cb = info->js_cb;
  TEN_ASSERT(js_cb && ten_nodejs_tsfn_check_integrity(js_cb, false),
             "Should not happen.");

  ten_error_t *cloned_error = NULL;
  if (err) {
    cloned_error = ten_error_create();
    ten_error_copy(cloned_error, err);
  }

  ten_nodejs_send_audio_frame_callback_call_info_t *call_info =
      ten_nodejs_send_audio_frame_callback_call_info_create(js_cb,
                                                            cloned_error);
  TEN_ASSERT(call_info, "Failed to create callback call info.");

  bool rc = ten_nodejs_tsfn_invoke(info->js_cb, call_info);
  TEN_ASSERT(rc, "Should not happen.");

  ten_env_notify_send_audio_frame_info_destroy(info);
}

static void ten_env_proxy_notify_send_audio_frame(ten_env_t *ten_env,
                                                  void *user_audio_frame) {
  TEN_ASSERT(user_audio_frame, "Invalid argument.");
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_env_notify_send_audio_frame_info_t *info = user_audio_frame;
  TEN_ASSERT(info, "Should not happen.");

  ten_error_t err;
  ten_error_init(&err);

  bool rc =
      ten_env_send_audio_frame(ten_env, info->c_audio_frame,
                               proxy_send_audio_frame_callback, info, &err);
  if (!rc) {
    proxy_send_audio_frame_callback(ten_env, NULL, info, &err);
  }

  ten_error_deinit(&err);
}

napi_value ten_nodejs_ten_env_send_audio_frame(napi_env env,
                                               napi_callback_info info) {
  const size_t argc = 3;
  napi_value args[argc];  // this, audio_frame, callback
  if (!ten_nodejs_get_js_func_args(env, info, args, argc)) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH,
                     "Incorrect number of parameters passed.",
                     NAPI_AUTO_LENGTH);
    TEN_ASSERT(0, "Should not happen.");
    return NULL;
  }

  ten_nodejs_ten_env_t *ten_env_bridge = NULL;
  napi_status status = napi_unwrap(env, args[0], (void **)&ten_env_bridge);
  RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok && ten_env_bridge != NULL,
                                "Failed to unwrap TenEnv object");

  TEN_ASSERT(ten_env_bridge &&
                 ten_nodejs_ten_env_check_integrity(ten_env_bridge, true),
             "Should not happen.");

  ten_nodejs_audio_frame_t *audio_frame_bridge = NULL;
  status = napi_unwrap(env, args[1], (void **)&audio_frame_bridge);
  RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok && audio_frame_bridge != NULL,
                                "Failed to unwrap audio_frame object");

  ten_nodejs_tsfn_t *cb_tsfn =
      ten_nodejs_tsfn_create(env, "[TSFN] TenEnv::send_audio_frame callback",
                             args[2], tsfn_proxy_send_audio_frame_callback);
  RETURN_UNDEFINED_IF_NAPI_FAIL(cb_tsfn, "Failed to create TSFN");

  ten_error_t err;
  ten_error_init(&err);

  ten_env_notify_send_audio_frame_info_t *notify_info =
      ten_env_notify_send_audio_frame_info_create(
          ten_shared_ptr_clone(audio_frame_bridge->msg.msg), cb_tsfn);
  TEN_ASSERT(notify_info, "Failed to create notify info.");

  bool rc = ten_env_proxy_notify(ten_env_bridge->c_ten_env_proxy,
                                 ten_env_proxy_notify_send_audio_frame,
                                 notify_info, false, &err);
  if (!rc) {
    ten_string_t code_str;
    ten_string_init_formatted(&code_str, "%d", ten_error_errno(&err));

    status = napi_throw_error(env, ten_string_get_raw_str(&code_str),
                              ten_error_errmsg(&err));
    RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok, "Failed to throw error");

    ten_string_deinit(&code_str);

    // The JS callback will not be called, so release the TSFN here.
    ten_nodejs_tsfn_release(cb_tsfn);

    ten_env_notify_send_audio_frame_info_destroy(notify_info);
  }

  ten_error_deinit(&err);

  return js_undefined(env);
}
