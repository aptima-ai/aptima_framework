//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <string.h>

#include "include_internal/axis_runtime/binding/python/common/common.h"
#include "include_internal/axis_runtime/binding/python/common/error.h"
#include "include_internal/axis_runtime/binding/python/axis_env/axis_env.h"
#include "axis_runtime/axis_env/internal/metadata.h"
#include "axis_runtime/axis_env_proxy/axis_env_proxy.h"
#include "axis_utils/macro/memory.h"

typedef struct axis_env_notify_init_property_ctx_t {
  axis_string_t value;
  axis_event_t *completed;
} axis_env_notify_init_property_ctx_t;

typedef struct axis_env_notify_init_property_async_ctx_t {
  axis_string_t value;
  PyObject *py_cb_func;
} axis_env_notify_init_property_async_ctx_t;

static axis_env_notify_init_property_ctx_t *
axis_env_notify_init_property_ctx_create(const void *value, size_t value_len) {
  axis_env_notify_init_property_ctx_t *ctx =
      axis_MALLOC(sizeof(axis_env_notify_init_property_ctx_t));
  axis_ASSERT(ctx, "Failed to allocate memory.");

  axis_string_init_formatted(&ctx->value, "%.*s", value_len, value);
  ctx->completed = axis_event_create(0, 1);

  return ctx;
}

static void axis_env_notify_init_property_ctx_destroy(
    axis_env_notify_init_property_ctx_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_string_deinit(&self->value);
  axis_event_destroy(self->completed);

  axis_FREE(self);
}

static axis_env_notify_init_property_async_ctx_t *
axis_env_notify_init_property_async_ctx_create(const void *value,
                                              size_t value_len,
                                              PyObject *py_cb_func) {
  axis_env_notify_init_property_async_ctx_t *ctx =
      axis_MALLOC(sizeof(axis_env_notify_init_property_async_ctx_t));
  axis_ASSERT(ctx, "Failed to allocate memory.");

  axis_string_init_formatted(&ctx->value, "%.*s", value_len, value);
  ctx->py_cb_func = py_cb_func;

  if (py_cb_func != NULL) {
    Py_INCREF(py_cb_func);
  }

  return ctx;
}

static void axis_env_notify_init_property_async_ctx_destroy(
    axis_env_notify_init_property_async_ctx_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_string_deinit(&self->value);
  self->py_cb_func = NULL;

  axis_FREE(self);
}

static void axis_env_proxy_notify_init_property_from_json(axis_env_t *axis_env,
                                                         void *user_data) {
  axis_ASSERT(user_data, "Invalid argument.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_env_notify_init_property_ctx_t *ctx = user_data;
  axis_ASSERT(ctx, "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  axis_env_init_property_from_json(axis_env, axis_string_get_raw_str(&ctx->value),
                                  &err);

  axis_event_set(ctx->completed);

  axis_error_deinit(&err);
}

static void axis_env_proxy_notify_init_property_from_json_async(
    axis_env_t *axis_env, void *user_data) {
  axis_ASSERT(user_data, "Invalid argument.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_env_notify_init_property_async_ctx_t *ctx = user_data;
  axis_ASSERT(ctx, "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  bool rc = axis_env_init_property_from_json(
      axis_env, axis_string_get_raw_str(&ctx->value), &err);

  // About to call the Python function, so it's necessary to ensure that the
  // GIL
  // has been acquired.
  //
  // Allows C codes to work safely with Python objects.
  PyGILState_STATE prev_state = axis_py_gil_state_ensure_internal();

  PyObject *arglist = NULL;
  axis_py_error_t *py_error = NULL;

  if (rc) {
    arglist = Py_BuildValue("(O)", Py_None);
  } else {
    py_error = axis_py_error_wrap(&err);
    arglist = Py_BuildValue("(O)", py_error);
  }

  PyObject *result = PyObject_CallObject(ctx->py_cb_func, arglist);
  Py_XDECREF(result);  // Ensure cleanup if an error occurred.

  bool err_occurred = axis_py_check_and_clear_py_error();
  axis_ASSERT(!err_occurred, "Should not happen.");

  Py_XDECREF(arglist);
  Py_XDECREF(ctx->py_cb_func);

  if (py_error) {
    axis_py_error_invalidate(py_error);
  }

  axis_py_gil_state_release_internal(prev_state);

  axis_error_deinit(&err);

  axis_env_notify_init_property_async_ctx_destroy(ctx);
}

PyObject *axis_py_axis_env_init_property_from_json(PyObject *self,
                                                 PyObject *args) {
  axis_py_axis_env_t *py_axis_env = (axis_py_axis_env_t *)self;
  axis_ASSERT(py_axis_env && axis_py_axis_env_check_integrity(py_axis_env),
             "Invalid argument.");

  if (PyTuple_GET_SIZE(args) != 1) {
    return axis_py_raise_py_value_error_exception(
        "Invalid argument count when axis_env.init_property_from_json.");
  }

  const char *json_str = NULL;
  if (!PyArg_ParseTuple(args, "s", &json_str)) {
    return axis_py_raise_py_value_error_exception(
        "Failed to parse arguments when "
        "axis_env.init_property_from_json.");
  }

  axis_error_t err;
  axis_error_init(&err);

  axis_env_notify_init_property_ctx_t *ctx =
      axis_env_notify_init_property_ctx_create(json_str, strlen(json_str));

  if (!axis_env_proxy_notify(py_axis_env->c_axis_env_proxy,
                            axis_env_proxy_notify_init_property_from_json, ctx,
                            false, NULL)) {
    goto done;
  }

  PyThreadState *saved_py_thread_state = PyEval_SaveThread();
  axis_event_wait(ctx->completed, -1);
  PyEval_RestoreThread(saved_py_thread_state);

done:
  axis_env_notify_init_property_ctx_destroy(ctx);
  axis_error_deinit(&err);

  Py_RETURN_NONE;
}

PyObject *axis_py_axis_env_init_property_from_json_async(PyObject *self,
                                                       PyObject *args) {
  axis_py_axis_env_t *py_axis_env = (axis_py_axis_env_t *)self;
  axis_ASSERT(py_axis_env && axis_py_axis_env_check_integrity(py_axis_env),
             "Invalid argument.");

  if (PyTuple_GET_SIZE(args) != 2) {
    return axis_py_raise_py_value_error_exception(
        "Invalid argument count when axis_env.init_property_from_json_async.");
  }

  const char *json_str = NULL;
  PyObject *py_cb_func = NULL;

  if (!PyArg_ParseTuple(args, "sO", &json_str, &py_cb_func)) {
    return axis_py_raise_py_value_error_exception(
        "Failed to parse arguments when "
        "axis_env.init_property_from_json_async.");
  }

  // Check if cb_func is callable.
  if (!PyCallable_Check(py_cb_func)) {
    return axis_py_raise_py_value_error_exception(
        "Invalid callback function when "
        "axis_env.init_property_from_json_async.");
  }

  axis_error_t err;
  axis_error_init(&err);

  bool success = true;

  axis_env_notify_init_property_async_ctx_t *ctx =
      axis_env_notify_init_property_async_ctx_create(json_str, strlen(json_str),
                                                    py_cb_func);

  if (!axis_env_proxy_notify(py_axis_env->c_axis_env_proxy,
                            axis_env_proxy_notify_init_property_from_json_async,
                            ctx, false, &err)) {
    Py_XDECREF(py_cb_func);
    axis_env_notify_init_property_async_ctx_destroy(ctx);

    axis_py_raise_py_value_error_exception("Failed to init property from json");

    success = false;
  }

  axis_error_deinit(&err);

  if (!success) {
    return NULL;
  }

  Py_RETURN_NONE;
}
