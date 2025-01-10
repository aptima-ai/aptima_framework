//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/binding/nodejs/axis_env/axis_env.h"

static void tsfn_proxy_get_property_string_callback(napi_env env,
                                                    napi_value js_cb,
                                                    void *context, void *data) {
  axis_nodejs_get_property_call_ctx_t *ctx =
      (axis_nodejs_get_property_call_ctx_t *)data;
  axis_ASSERT(ctx, "Should not happen.");

  napi_value js_res = NULL;
  napi_value js_error = NULL;

  axis_error_t err;
  axis_error_init(&err);

  axis_value_t *value = ctx->value;
  if (!value) {
    if (ctx->error) {
      js_error = axis_nodejs_create_error(env, ctx->error);
      ASSERT_IF_NAPI_FAIL(js_error, "Failed to create JS error", NULL);
    } else {
      axis_error_set(&err, axis_ERRNO_GENERIC, "Failed to get property value");
      js_error = axis_nodejs_create_error(env, &err);
      ASSERT_IF_NAPI_FAIL(js_error, "Failed to create JS error", NULL);
    }
  } else {
    js_res = axis_nodejs_create_value_string(env, value, &err);
    if (!js_res) {
      js_error = axis_nodejs_create_error(env, &err);
      ASSERT_IF_NAPI_FAIL(js_error, "Failed to create JS error", NULL);
    }
  }

  if (!js_res) {
    js_res = js_undefined(env);
  }

  if (!js_error) {
    js_error = js_undefined(env);
  }

  napi_value args[] = {js_res, js_error};
  napi_value result = NULL;
  napi_status status = napi_call_function(env, js_res, js_cb, 2, args, &result);
  ASSERT_IF_NAPI_FAIL(
      status == napi_ok,
      "Failed to call JS callback of TenEnv::getPropertyString: %d", status);

  axis_nodejs_tsfn_release(ctx->cb_tsfn);

  axis_nodejs_get_property_call_ctx_destroy(ctx);
}

napi_value axis_nodejs_axis_env_get_property_string(napi_env env,
                                                  napi_callback_info info) {
  axis_ASSERT(env, "Should not happen.");

  const size_t argc = 3;
  napi_value args[argc];  // axis_env, path, callback
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

  axis_string_t name;
  axis_string_init(&name);

  bool rc = axis_nodejs_get_str_from_js(env, args[1], &name);
  RETURN_UNDEFINED_IF_NAPI_FAIL(rc, "Failed to get property name", NULL);

  axis_nodejs_tsfn_t *cb_tsfn =
      axis_nodejs_tsfn_create(env, "[TSFN] TenEnv::getPropertyString callback",
                             args[2], tsfn_proxy_get_property_string_callback);
  RETURN_UNDEFINED_IF_NAPI_FAIL(cb_tsfn, "Failed to create TSFN", NULL);

  axis_error_t err;
  axis_error_init(&err);

  rc = axis_nodejs_axis_env_get_property_value(
      axis_env_bridge, axis_string_get_raw_str(&name), cb_tsfn, &err);
  if (!rc) {
    axis_string_t code_str;
    axis_string_init_formatted(&code_str, "%d", axis_error_errno(&err));

    status = napi_throw_error(env, axis_string_get_raw_str(&code_str),
                              axis_error_errmsg(&err));
    ASSERT_IF_NAPI_FAIL(status == napi_ok, "Failed to throw error: %d", status);

    axis_string_deinit(&code_str);

    // The JS callback will not be called, so we need to clean up the tsfn.
    axis_nodejs_tsfn_release(cb_tsfn);
  }

  axis_error_deinit(&err);
  axis_string_deinit(&name);

  return js_undefined(env);
}
