//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/binding/nodejs/common/common.h"
#include "include_internal/ten_runtime/binding/nodejs/ten_env/ten_env.h"
#include "ten_runtime/ten_env_proxy/ten_env_proxy.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/macro/memory.h"

typedef struct ten_env_notify_log_ctx_t {
  int32_t level;
  ten_string_t func_name;
  ten_string_t file_name;
  int32_t line_no;
  ten_string_t msg;
  ten_event_t *completed;
} ten_env_notify_log_ctx_t;

static ten_env_notify_log_ctx_t *ten_env_notify_log_ctx_create(void) {
  ten_env_notify_log_ctx_t *ctx = TEN_MALLOC(sizeof(ten_env_notify_log_ctx_t));
  TEN_ASSERT(ctx, "Failed to allocate memory.");

  ctx->level = 0;
  ten_string_init(&ctx->func_name);
  ten_string_init(&ctx->file_name);
  ctx->line_no = 0;
  ten_string_init(&ctx->msg);
  ctx->completed = ten_event_create(0, 1);

  return ctx;
}

static void ten_env_notify_log_ctx_destroy(ten_env_notify_log_ctx_t *ctx) {
  TEN_ASSERT(ctx, "Invalid argument.");

  ten_string_deinit(&ctx->func_name);
  ten_string_deinit(&ctx->file_name);
  ten_string_deinit(&ctx->msg);
  ten_event_destroy(ctx->completed);

  TEN_FREE(ctx);
}

static void ten_env_proxy_notify_log(ten_env_t *ten_env, void *user_data) {
  TEN_ASSERT(user_data, "Invalid argument.");
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_env_notify_log_ctx_t *ctx = user_data;
  TEN_ASSERT(ctx, "Should not happen.");

  ten_env_log(ten_env, ctx->level, ten_string_get_raw_str(&ctx->func_name),
              ten_string_get_raw_str(&ctx->file_name), ctx->line_no,
              ten_string_get_raw_str(&ctx->msg));

  ten_event_set(ctx->completed);
}

napi_value ten_nodejs_ten_env_log_internal(napi_env env,
                                           napi_callback_info info) {
  const size_t argc = 6;
  napi_value args[argc];  // ten_env, level, func_name, file_name, line_no, msg
  if (!ten_nodejs_get_js_func_args(env, info, args, argc)) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH,
                     "Incorrect number of parameters passed.",
                     NAPI_AUTO_LENGTH);
    TEN_ASSERT(0, "Should not happen.");
  }

  ten_nodejs_ten_env_t *ten_env_bridge = NULL;
  napi_status status = napi_unwrap(env, args[0], (void **)&ten_env_bridge);
  RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok && ten_env_bridge != NULL,
                                "Failed to get ten_env bridge: %d", status);

  ten_env_notify_log_ctx_t *notify_info = ten_env_notify_log_ctx_create();
  TEN_ASSERT(notify_info, "Failed to create log notify_info.");

  status = napi_get_value_int32(env, args[1], &notify_info->level);
  RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok,
                                "Failed to get log level: %d", status);

  bool rc = ten_nodejs_get_str_from_js(env, args[2], &notify_info->func_name);
  RETURN_UNDEFINED_IF_NAPI_FAIL(rc, "Failed to get function name.");

  rc = ten_nodejs_get_str_from_js(env, args[3], &notify_info->file_name);
  RETURN_UNDEFINED_IF_NAPI_FAIL(rc, "Failed to get file name.");

  status = napi_get_value_int32(env, args[4], &notify_info->line_no);

  rc = ten_nodejs_get_str_from_js(env, args[5], &notify_info->msg);
  RETURN_UNDEFINED_IF_NAPI_FAIL(rc, "Failed to get message.");

  ten_error_t err;
  ten_error_init(&err);

  rc = ten_env_proxy_notify(ten_env_bridge->c_ten_env_proxy,
                            ten_env_proxy_notify_log, notify_info, false, &err);
  if (!rc) {
    ten_string_t code_str;
    ten_string_init_formatted(&code_str, "%d", ten_error_errno(&err));

    status = napi_throw_error(env, ten_string_get_raw_str(&code_str),
                              ten_error_errmsg(&err));
    ASSERT_IF_NAPI_FAIL(status == napi_ok, "Failed to throw error: %d", status);

    ten_string_deinit(&code_str);
  } else {
    ten_event_wait(notify_info->completed, -1);
  }

  ten_error_deinit(&err);
  ten_env_notify_log_ctx_destroy(notify_info);

  return js_undefined(env);
}
