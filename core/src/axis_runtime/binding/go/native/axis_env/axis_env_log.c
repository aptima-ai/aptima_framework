//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "include_internal/axis_runtime/binding/go/axis_env/axis_env.h"
#include "include_internal/axis_runtime/binding/go/axis_env/axis_env_internal.h"
#include "include_internal/axis_runtime/axis_env/log.h"
#include "axis_runtime/binding/go/interface/ten/common.h"
#include "axis_runtime/binding/go/interface/ten/axis_env.h"
#include "axis_runtime/axis_env/internal/log.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/memory.h"

typedef struct axis_env_notify_log_ctx_t {
  int32_t level;
  const char *func_name;
  size_t func_name_len;
  const char *file_name;
  size_t file_name_len;
  size_t line_no;
  const char *msg;
  size_t msg_len;
  axis_event_t *completed;
} axis_env_notify_log_ctx_t;

static axis_env_notify_log_ctx_t *axis_env_notify_log_ctx_create(
    int32_t level, const char *func_name, size_t func_name_len,
    const char *file_name, size_t file_name_len, size_t line_no,
    const char *msg, size_t msg_len) {
  axis_env_notify_log_ctx_t *ctx = axis_MALLOC(sizeof(axis_env_notify_log_ctx_t));
  axis_ASSERT(ctx, "Failed to allocate memory.");

  ctx->level = level;
  ctx->func_name = func_name;
  ctx->func_name_len = func_name_len;
  ctx->file_name = file_name;
  ctx->file_name_len = file_name_len;
  ctx->line_no = line_no;
  ctx->msg = msg;
  ctx->msg_len = msg_len;
  ctx->completed = axis_event_create(0, 1);

  return ctx;
}

static void axis_env_notify_log_ctx_destroy(axis_env_notify_log_ctx_t *ctx) {
  axis_ASSERT(ctx, "Invalid argument.");

  axis_event_destroy(ctx->completed);

  axis_FREE(ctx);
}

static void axis_env_proxy_notify_log(axis_env_t *axis_env, void *user_data) {
  axis_ASSERT(user_data, "Invalid argument.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_env_notify_log_ctx_t *ctx = user_data;
  axis_ASSERT(ctx, "Should not happen.");

  axis_env_log_with_size_formatted(
      axis_env, ctx->level, ctx->func_name, ctx->func_name_len, ctx->file_name,
      ctx->file_name_len, ctx->line_no, "%.*s", ctx->msg_len, ctx->msg);

  axis_event_set(ctx->completed);
}

void axis_go_axis_env_log(uintptr_t bridge_addr, int level, const void *func_name,
                        int func_name_len, const void *file_name,
                        int file_name_len, int line_no, const void *msg,
                        int msg_len) {
  axis_go_axis_env_t *self = axis_go_axis_env_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");

  // According to the document of `unsafe.StringData()`, the underlying data
  // (i.e., value here) of an empty GO string is unspecified. So it's unsafe to
  // access. We should handle this case explicitly.
  const char *func_name_value = NULL;
  if (func_name_len > 0) {
    func_name_value = (const char *)func_name;
  }

  const char *file_name_value = NULL;
  if (file_name_len > 0) {
    file_name_value = (const char *)file_name;
  }

  const char *msg_value = NULL;
  if (msg_len > 0) {
    msg_value = (const char *)msg;
  }

  axis_env_notify_log_ctx_t *ctx = axis_env_notify_log_ctx_create(
      level, func_name_value, func_name_len, file_name_value, file_name_len,
      line_no, msg_value, msg_len);

  axis_error_t err;
  axis_error_init(&err);

  if (self->c_axis_env->attach_to == axis_ENV_ATTACH_TO_ADDON) {
    // TODO(Wei): This function is currently specifically designed for the addon
    // because the addon currently does not have a main thread, so it's unable
    // to check thread safety. Once the main thread for the addon is determined
    // in the future, these hacks made specifically for the addon can be
    // completely removed, and comprehensive thread safety checking can be
    // implemented.
    axis_env_log_with_size_formatted_without_check_thread(
        self->c_axis_env, ctx->level, ctx->func_name, ctx->func_name_len,
        ctx->file_name, ctx->file_name_len, ctx->line_no, "%.*s", ctx->msg_len,
        ctx->msg);
  } else {
    if (!axis_env_proxy_notify(self->c_axis_env_proxy, axis_env_proxy_notify_log,
                              ctx, false, &err)) {
      goto done;
    }
    axis_event_wait(ctx->completed, -1);
  }

done:
  axis_error_deinit(&err);
  axis_env_notify_log_ctx_destroy(ctx);
}
