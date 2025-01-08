//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/binding/nodejs/axis_env/axis_env.h"
#include "axis_utils/lib/string.h"

static void tsfn_proxy_set_property_string_callback(napi_env env,
                                                    napi_value js_cb,
                                                    void *context, void *data) {
  axis_nodejs_set_property_call_ctx_t *ctx =
      (axis_nodejs_set_property_call_ctx_t *)data;
  axis_ASSERT(ctx, "Should not happen.");

  napi_value js_error = NULL;

  if (ctx->success) {
    js_error = js_undefined(env);
  } else {
    if (ctx->error) {
      js_error = axis_nodejs_create_error(env, ctx->error);
      ASSERT_IF_NAPI_FAIL(js_error, "Failed to create JS error", NULL);
    } else {
      axis_error_t err;
      axis_error_init(&err);
      axis_error_set(&err, axis_ERRNO_GENERIC, "Failed to set property value");
      js_error = axis_nodejs_create_error(env, &err);
      ASSERT_IF_NAPI_FAIL(js_error, "Failed to create JS error", NULL);
      axis_error_deinit(&err);
    }
  }

  napi_value args[] = {js_error};
  napi_value result = NULL;
  napi_status status =
      napi_call_function(env, js_error, js_cb, 1, args, &result);
  ASSERT_IF_NAPI_FAIL(
      status == napi_ok,
      "Failed to call JS callback of TenEnv::setPropertyNumber: %d", status);

  axis_nodejs_tsfn_release(ctx->cb_tsfn);

  axis_nodejs_set_property_call_ctx_destroy(ctx);
}

napi_value axis_nodejs_axis_env_set_property_string(napi_env env,
                                                  napi_callback_info info) {
  const size_t argc = 4;
  napi_value args[argc];  // axis_env, path, string, callback
  if (!axis_nodejs_get_js_func_args(env, info, args, argc)) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH,
                     "Incorrect number of parameters passed.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Should not happen.");
  }

  axis_nodejs_axis_env_t *axis_env_bridge = NULL;
  napi_status status = napi_unwrap(env, args[0], (void **)&axis_env_bridge);
  RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok && axis_env_bridge != NULL,
                                "Failed to get axis_env bridge: %d", status);
  axis_ASSERT(axis_env_bridge &&
                 axis_nodejs_axis_env_check_integrity(axis_env_bridge, true),
             "Should not happen.");

  axis_string_t path;
  axis_string_init(&path);

  bool rc = axis_nodejs_get_str_from_js(env, args[1], &path);
  RETURN_UNDEFINED_IF_NAPI_FAIL(rc, "Failed to get property path", NULL);

  axis_string_t property_value_str;
  axis_string_init(&property_value_str);

  rc = axis_nodejs_get_str_from_js(env, args[2], &property_value_str);
  RETURN_UNDEFINED_IF_NAPI_FAIL(rc, "Failed to get property value", NULL);

  axis_value_t *value =
      axis_value_create_string(axis_string_get_raw_str(&property_value_str));
  axis_ASSERT(value, "Failed to create string value.");

  axis_string_deinit(&property_value_str);

  axis_nodejs_tsfn_t *cb_tsfn =
      axis_nodejs_tsfn_create(env, "[TSFN] TenEnv::setPropertyString callback",
                             args[3], tsfn_proxy_set_property_string_callback);
  RETURN_UNDEFINED_IF_NAPI_FAIL(cb_tsfn, "Failed to create TSFN", NULL);

  axis_error_t err;
  axis_error_init(&err);

  rc = axis_nodejs_axis_env_set_property_value(
      axis_env_bridge, axis_string_get_raw_str(&path), value, cb_tsfn, &err);
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

  axis_string_deinit(&path);
  axis_error_deinit(&err);

  return js_undefined(env);
}
