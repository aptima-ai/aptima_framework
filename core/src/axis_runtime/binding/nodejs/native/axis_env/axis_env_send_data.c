//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/binding/nodejs/msg/data.h"
#include "include_internal/axis_runtime/binding/nodejs/axis_env/axis_env.h"
#include "axis_utils/macro/memory.h"

typedef struct axis_env_notify_send_data_ctx_t {
  axis_shared_ptr_t *c_data;
  axis_nodejs_tsfn_t *js_cb;
} axis_env_notify_send_data_ctx_t;

typedef struct axis_nodejs_send_data_callback_call_ctx_t {
  axis_nodejs_tsfn_t *js_cb;
  axis_error_t *error;
} axis_nodejs_send_data_callback_call_ctx_t;

static axis_env_notify_send_data_ctx_t *axis_env_notify_send_data_ctx_create(
    axis_shared_ptr_t *c_data, axis_nodejs_tsfn_t *js_cb) {
  axis_env_notify_send_data_ctx_t *ctx =
      axis_MALLOC(sizeof(axis_env_notify_send_data_ctx_t));
  axis_ASSERT(ctx, "Failed to allocate memory.");

  ctx->c_data = c_data;
  ctx->js_cb = js_cb;

  return ctx;
}

static void axis_env_notify_send_data_ctx_destroy(
    axis_env_notify_send_data_ctx_t *ctx) {
  axis_ASSERT(ctx, "Invalid argument.");

  if (ctx->c_data) {
    axis_shared_ptr_destroy(ctx->c_data);
    ctx->c_data = NULL;
  }

  ctx->js_cb = NULL;

  axis_FREE(ctx);
}

static axis_nodejs_send_data_callback_call_ctx_t *
axis_nodejs_send_data_callback_call_ctx_create(axis_nodejs_tsfn_t *js_cb,
                                              axis_error_t *error) {
  axis_nodejs_send_data_callback_call_ctx_t *ctx =
      axis_MALLOC(sizeof(axis_nodejs_send_data_callback_call_ctx_t));
  axis_ASSERT(ctx, "Failed to allocate memory.");

  ctx->js_cb = js_cb;
  ctx->error = error;

  return ctx;
}

static void axis_nodejs_send_data_callback_call_ctx_destroy(
    axis_nodejs_send_data_callback_call_ctx_t *ctx) {
  axis_ASSERT(ctx, "Invalid argument.");

  if (ctx->error) {
    axis_error_destroy(ctx->error);
    ctx->error = NULL;
  }

  axis_FREE(ctx);
}

static void tsfn_proxy_send_data_callback(napi_env env, napi_value js_cb,
                                          void *context, void *data) {
  axis_nodejs_send_data_callback_call_ctx_t *ctx = data;
  axis_ASSERT(ctx, "Should not happen.");

  napi_value js_error = NULL;

  if (ctx->error) {
    js_error = axis_nodejs_create_error(env, ctx->error);
    ASSERT_IF_NAPI_FAIL(js_error, "Failed to create JS error", NULL);
  } else {
    js_error = js_undefined(env);
  }

  napi_value argv[] = {js_error};
  napi_status status =
      napi_call_function(env, js_undefined(env), js_cb, 1, argv, NULL);
  ASSERT_IF_NAPI_FAIL(status == napi_ok, "Failed to call JS callback", NULL);

  axis_nodejs_tsfn_release(ctx->js_cb);

  axis_nodejs_send_data_callback_call_ctx_destroy(ctx);
}

static void proxy_send_data_callback(axis_env_t *axis_env, axis_shared_ptr_t *msg,
                                     void *user_data, axis_error_t *err) {
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_env_notify_send_data_ctx_t *ctx = user_data;
  axis_ASSERT(ctx, "Should not happen.");

  axis_nodejs_tsfn_t *js_cb = ctx->js_cb;
  axis_ASSERT(js_cb && axis_nodejs_tsfn_check_integrity(js_cb, false),
             "Should not happen.");

  axis_error_t *cloned_error = NULL;
  if (err) {
    cloned_error = axis_error_create();
    axis_error_copy(cloned_error, err);
  }

  axis_nodejs_send_data_callback_call_ctx_t *call_info =
      axis_nodejs_send_data_callback_call_ctx_create(js_cb, cloned_error);
  axis_ASSERT(call_info, "Failed to create callback call info.");

  bool rc = axis_nodejs_tsfn_invoke(ctx->js_cb, call_info);
  axis_ASSERT(rc, "Should not happen.");

  axis_env_notify_send_data_ctx_destroy(ctx);
}

static void axis_env_proxy_notify_send_data(axis_env_t *axis_env,
                                           void *user_data) {
  axis_ASSERT(user_data, "Invalid argument.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_env_notify_send_data_ctx_t *ctx = user_data;
  axis_ASSERT(ctx, "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  bool rc = axis_env_send_data(axis_env, ctx->c_data, proxy_send_data_callback,
                              ctx, &err);
  if (!rc) {
    proxy_send_data_callback(axis_env, NULL, ctx, &err);
  }

  axis_error_deinit(&err);
}

napi_value axis_nodejs_axis_env_send_data(napi_env env, napi_callback_info info) {
  const size_t argc = 3;
  napi_value args[argc];  // this, data, callback
  if (!axis_nodejs_get_js_func_args(env, info, args, argc)) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH,
                     "Incorrect number of parameters passed.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Should not happen.");
    return NULL;
  }

  axis_nodejs_axis_env_t *axis_env_bridge = NULL;
  napi_status status = napi_unwrap(env, args[0], (void **)&axis_env_bridge);
  RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok && axis_env_bridge != NULL,
                                "Failed to unwrap TenEnv object");

  axis_ASSERT(axis_env_bridge &&
                 axis_nodejs_axis_env_check_integrity(axis_env_bridge, true),
             "Should not happen.");

  axis_nodejs_data_t *data_bridge = NULL;
  status = napi_unwrap(env, args[1], (void **)&data_bridge);
  RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok && data_bridge != NULL,
                                "Failed to unwrap Data object");

  axis_nodejs_tsfn_t *cb_tsfn =
      axis_nodejs_tsfn_create(env, "[TSFN] TenEnv::send_data callback", args[2],
                             tsfn_proxy_send_data_callback);
  RETURN_UNDEFINED_IF_NAPI_FAIL(cb_tsfn, "Failed to create TSFN");

  axis_error_t err;
  axis_error_init(&err);

  axis_env_notify_send_data_ctx_t *notify_info =
      axis_env_notify_send_data_ctx_create(
          axis_shared_ptr_clone(data_bridge->msg.msg), cb_tsfn);
  axis_ASSERT(notify_info, "Failed to create notify info.");

  bool rc = axis_env_proxy_notify(axis_env_bridge->c_axis_env_proxy,
                                 axis_env_proxy_notify_send_data, notify_info,
                                 false, &err);
  if (!rc) {
    axis_string_t code_str;
    axis_string_init_formatted(&code_str, "%d", axis_error_errno(&err));

    status = napi_throw_error(env, axis_string_get_raw_str(&code_str),
                              axis_error_errmsg(&err));
    RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok, "Failed to throw error");

    axis_string_deinit(&code_str);

    // The JS callback will not be called, so release the TSFN here.
    axis_nodejs_tsfn_release(cb_tsfn);

    axis_env_notify_send_data_ctx_destroy(notify_info);
  }

  axis_error_deinit(&err);

  return js_undefined(env);
}
