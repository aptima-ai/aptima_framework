//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/binding/nodejs/common/common.h"
#include "include_internal/axis_runtime/binding/nodejs/axis_env/axis_env.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/memory.h"

typedef struct axis_env_notify_init_property_from_json_ctx_t {
  axis_string_t json_str;
  axis_nodejs_tsfn_t *js_cb;
} axis_env_notify_init_property_from_json_ctx_t;

typedef struct axis_nodejs_init_property_from_json_call_ctx_t {
  axis_nodejs_tsfn_t *js_cb;
  axis_error_t *error;
} axis_nodejs_init_property_from_json_call_ctx_t;

static axis_env_notify_init_property_from_json_ctx_t *
axis_env_notify_init_property_from_json_ctx_create(axis_nodejs_tsfn_t *js_cb) {
  axis_env_notify_init_property_from_json_ctx_t *ctx =
      axis_MALLOC(sizeof(axis_env_notify_init_property_from_json_ctx_t));
  axis_ASSERT(ctx, "Failed to allocate memory.");

  axis_string_init(&ctx->json_str);
  ctx->js_cb = js_cb;

  return ctx;
}

static void axis_env_notify_init_property_from_json_ctx_destroy(
    axis_env_notify_init_property_from_json_ctx_t *ctx) {
  axis_ASSERT(ctx, "Invalid argument.");

  axis_string_deinit(&ctx->json_str);
  ctx->js_cb = NULL;

  axis_FREE(ctx);
}

static void tsfn_proxy_init_property_from_json_callback(napi_env env,
                                                        napi_value js_cb,
                                                        void *context,
                                                        void *data) {
  axis_nodejs_init_property_from_json_call_ctx_t *call_info =
      (axis_nodejs_init_property_from_json_call_ctx_t *)data;
  axis_ASSERT(call_info, "Should not happen.");

  napi_value js_error = NULL;

  if (call_info->error) {
    js_error = axis_nodejs_create_error(env, call_info->error);
    ASSERT_IF_NAPI_FAIL(js_error, "Failed to create JS error", NULL);
  } else {
    js_error = js_undefined(env);
  }

  napi_value argv[] = {js_error};
  napi_value result = NULL;
  napi_status status =
      napi_call_function(env, js_error, js_cb, 1, argv, &result);
  ASSERT_IF_NAPI_FAIL(status == napi_ok, "Failed to call JS callback", NULL);

  if (call_info->error) {
    axis_error_destroy(call_info->error);
  }

  axis_nodejs_tsfn_release(call_info->js_cb);

  axis_FREE(call_info);
}

static void axis_env_proxy_notify_init_property_from_json(axis_env_t *axis_env,
                                                         void *user_data) {
  axis_ASSERT(user_data, "Invalid argument.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_env_notify_init_property_from_json_ctx_t *ctx = user_data;
  axis_ASSERT(ctx, "Should not happen.");

  axis_nodejs_tsfn_t *js_cb = ctx->js_cb;
  axis_ASSERT(js_cb && axis_nodejs_tsfn_check_integrity(js_cb, false),
             "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  bool rc = axis_env_init_property_from_json(
      axis_env, axis_string_get_raw_str(&ctx->json_str), &err);

  axis_error_t *cloned_error = NULL;
  if (!rc) {
    cloned_error = axis_error_create();
    axis_error_copy(cloned_error, &err);
  }

  axis_nodejs_init_property_from_json_call_ctx_t *call_info =
      axis_MALLOC(sizeof(axis_nodejs_init_property_from_json_call_ctx_t));
  axis_ASSERT(call_info, "Failed to allocate memory.");

  call_info->js_cb = js_cb;
  call_info->error = cloned_error;

  rc = axis_nodejs_tsfn_invoke(js_cb, call_info);
  axis_ASSERT(rc, "Should not happen.");

  axis_error_deinit(&err);

  axis_env_notify_init_property_from_json_ctx_destroy(ctx);
}

napi_value axis_nodejs_axis_env_init_property_from_json(napi_env env,
                                                      napi_callback_info info) {
  axis_ASSERT(env, "Should not happen.");

  const size_t argc = 3;
  napi_value args[argc];  // axis_env, json_str, callback

  if (!axis_nodejs_get_js_func_args(env, info, args, argc)) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH,
                     "Incorrect number of parameters passed.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Should not happen.");
  }

  axis_nodejs_axis_env_t *axis_env_bridge = NULL;
  napi_status status = napi_unwrap(env, args[0], (void **)&axis_env_bridge);
  RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok && axis_env_bridge != NULL,
                                "Failed to get rte bridge: %d", status);
  axis_ASSERT(axis_env_bridge &&
                 axis_nodejs_axis_env_check_integrity(axis_env_bridge, true),
             "Should not happen.");

  axis_nodejs_tsfn_t *cb_tsfn = axis_nodejs_tsfn_create(
      env, "[TSFN] TenEnv::initPropertyFromJson callback", args[2],
      tsfn_proxy_init_property_from_json_callback);
  RETURN_UNDEFINED_IF_NAPI_FAIL(cb_tsfn, "Failed to create TSFN.");

  axis_env_notify_init_property_from_json_ctx_t *notify_info =
      axis_env_notify_init_property_from_json_ctx_create(cb_tsfn);
  axis_ASSERT(notify_info, "Failed to create notify info.");

  bool rc = axis_nodejs_get_str_from_js(env, args[1], &notify_info->json_str);
  RETURN_UNDEFINED_IF_NAPI_FAIL(rc, "Failed to get JSON string from JS.");

  axis_error_t err;
  axis_error_init(&err);

  rc = axis_env_proxy_notify(axis_env_bridge->c_axis_env_proxy,
                            axis_env_proxy_notify_init_property_from_json,
                            notify_info, false, &err);
  if (!rc) {
    axis_string_t code_str;
    axis_string_init_formatted(&code_str, "%d", axis_error_errno(&err));

    status = napi_throw_error(env, axis_string_get_raw_str(&code_str),
                              axis_error_errmsg(&err));
    ASSERT_IF_NAPI_FAIL(status == napi_ok, "Failed to throw JS exception: %d",
                        status);

    axis_string_deinit(&code_str);

    axis_env_notify_init_property_from_json_ctx_destroy(notify_info);

    // The JS callback will not be called, so we need to clean up the tsfn.
    axis_nodejs_tsfn_release(cb_tsfn);
  }

  axis_error_deinit(&err);

  return js_undefined(env);
}
