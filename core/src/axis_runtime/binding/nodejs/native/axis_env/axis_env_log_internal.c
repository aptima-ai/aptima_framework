//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/binding/nodejs/common/common.h"
#include "include_internal/axis_runtime/binding/nodejs/axis_env/axis_env.h"
#include "axis_runtime/axis_env_proxy/axis_env_proxy.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/macro/memory.h"

typedef struct axis_env_notify_log_ctx_t {
  int32_t level;
  axis_string_t func_name;
  axis_string_t file_name;
  int32_t line_no;
  axis_string_t msg;
  axis_event_t *completed;
} axis_env_notify_log_ctx_t;

static axis_env_notify_log_ctx_t *axis_env_notify_log_ctx_create(void) {
  axis_env_notify_log_ctx_t *ctx = axis_MALLOC(sizeof(axis_env_notify_log_ctx_t));
  axis_ASSERT(ctx, "Failed to allocate memory.");

  ctx->level = 0;
  axis_string_init(&ctx->func_name);
  axis_string_init(&ctx->file_name);
  ctx->line_no = 0;
  axis_string_init(&ctx->msg);
  ctx->completed = axis_event_create(0, 1);

  return ctx;
}

static void axis_env_notify_log_ctx_destroy(axis_env_notify_log_ctx_t *ctx) {
  axis_ASSERT(ctx, "Invalid argument.");

  axis_string_deinit(&ctx->func_name);
  axis_string_deinit(&ctx->file_name);
  axis_string_deinit(&ctx->msg);
  axis_event_destroy(ctx->completed);

  axis_FREE(ctx);
}

static void axis_env_proxy_notify_log(axis_env_t *axis_env, void *user_data) {
  axis_ASSERT(user_data, "Invalid argument.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_env_notify_log_ctx_t *ctx = user_data;
  axis_ASSERT(ctx, "Should not happen.");

  axis_env_log(axis_env, ctx->level, axis_string_get_raw_str(&ctx->func_name),
              axis_string_get_raw_str(&ctx->file_name), ctx->line_no,
              axis_string_get_raw_str(&ctx->msg));

  axis_event_set(ctx->completed);
}

napi_value axis_nodejs_axis_env_log_internal(napi_env env,
                                           napi_callback_info info) {
  const size_t argc = 6;
  napi_value args[argc];  // axis_env, level, func_name, file_name, line_no, msg
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

  axis_env_notify_log_ctx_t *notify_info = axis_env_notify_log_ctx_create();
  axis_ASSERT(notify_info, "Failed to create log notify_info.");

  status = napi_get_value_int32(env, args[1], &notify_info->level);
  RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok,
                                "Failed to get log level: %d", status);

  bool rc = axis_nodejs_get_str_from_js(env, args[2], &notify_info->func_name);
  RETURN_UNDEFINED_IF_NAPI_FAIL(rc, "Failed to get function name.");

  rc = axis_nodejs_get_str_from_js(env, args[3], &notify_info->file_name);
  RETURN_UNDEFINED_IF_NAPI_FAIL(rc, "Failed to get file name.");

  status = napi_get_value_int32(env, args[4], &notify_info->line_no);

  rc = axis_nodejs_get_str_from_js(env, args[5], &notify_info->msg);
  RETURN_UNDEFINED_IF_NAPI_FAIL(rc, "Failed to get message.");

  axis_error_t err;
  axis_error_init(&err);

  rc = axis_env_proxy_notify(axis_env_bridge->c_axis_env_proxy,
                            axis_env_proxy_notify_log, notify_info, false, &err);
  if (!rc) {
    axis_string_t code_str;
    axis_string_init_formatted(&code_str, "%d", axis_error_errno(&err));

    status = napi_throw_error(env, axis_string_get_raw_str(&code_str),
                              axis_error_errmsg(&err));
    ASSERT_IF_NAPI_FAIL(status == napi_ok, "Failed to throw error: %d", status);

    axis_string_deinit(&code_str);
  } else {
    axis_event_wait(notify_info->completed, -1);
  }

  axis_error_deinit(&err);
  axis_env_notify_log_ctx_destroy(notify_info);

  return js_undefined(env);
}
