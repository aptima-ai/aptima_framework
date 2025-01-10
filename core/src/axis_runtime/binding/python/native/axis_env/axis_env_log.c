//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <string.h>

#include "include_internal/axis_runtime/binding/python/common/error.h"
#include "include_internal/axis_runtime/binding/python/axis_env/axis_env.h"
#include "include_internal/axis_runtime/axis_env/log.h"
#include "axis_runtime/axis_env/internal/log.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/memory.h"

typedef struct axis_env_notify_log_ctx_t {
  int32_t level;
  const char *func_name;
  const char *file_name;
  size_t line_no;
  const char *msg;
  axis_event_t *completed;
} axis_env_notify_log_ctx_t;

static axis_env_notify_log_ctx_t *axis_env_notify_log_ctx_create(
    int32_t level, const char *func_name, const char *file_name, size_t line_no,
    const char *msg) {
  axis_env_notify_log_ctx_t *ctx = axis_MALLOC(sizeof(axis_env_notify_log_ctx_t));
  axis_ASSERT(ctx, "Failed to allocate memory.");

  ctx->level = level;
  ctx->func_name = func_name;
  ctx->file_name = file_name;
  ctx->line_no = line_no;
  ctx->msg = msg;
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

  axis_env_log(axis_env, ctx->level, ctx->func_name, ctx->file_name, ctx->line_no,
              ctx->msg);

  axis_event_set(ctx->completed);
}

PyObject *axis_py_axis_env_log(PyObject *self, PyObject *args) {
  axis_py_axis_env_t *py_axis_env = (axis_py_axis_env_t *)self;
  axis_ASSERT(py_axis_env && axis_py_axis_env_check_integrity(py_axis_env),
             "Invalid argument.");

  if (PyTuple_GET_SIZE(args) != 5) {
    return axis_py_raise_py_value_error_exception(
        "Invalid argument count when axis_env.log.");
  }

  axis_LOG_LEVEL level = axis_LOG_LEVEL_INVALID;
  const char *func_name = NULL;
  const char *file_name = NULL;
  size_t line_no = 0;
  const char *msg = NULL;
  if (!PyArg_ParseTuple(args, "izzis", &level, &func_name, &file_name, &line_no,
                        &msg)) {
    return axis_py_raise_py_value_error_exception(
        "Failed to parse argument when axis_env.log.");
  }

  axis_error_t err;
  axis_error_init(&err);

  axis_env_notify_log_ctx_t *ctx =
      axis_env_notify_log_ctx_create(level, func_name, file_name, line_no, msg);

  if (py_axis_env->c_axis_env->attach_to == axis_ENV_ATTACH_TO_ADDON) {
    // TODO(Wei): This function is currently specifically designed for the addon
    // because the addon currently does not have a main thread, so it's unable
    // to check thread safety. Once the main thread for the addon is determined
    // in the future, these hacks made specifically for the addon can be
    // completely removed, and comprehensive thread safety checking can be
    // implemented.
    axis_env_log_without_check_thread(py_axis_env->c_axis_env, ctx->level,
                                     ctx->func_name, ctx->file_name,
                                     ctx->line_no, ctx->msg);
  } else {
    if (!axis_env_proxy_notify(py_axis_env->c_axis_env_proxy,
                              axis_env_proxy_notify_log, ctx, false, &err)) {
      goto done;
    }

    PyThreadState *saved_py_thread_state = PyEval_SaveThread();
    axis_event_wait(ctx->completed, -1);
    PyEval_RestoreThread(saved_py_thread_state);
  }

done:
  axis_error_deinit(&err);
  axis_env_notify_log_ctx_destroy(ctx);

  Py_RETURN_NONE;
}
