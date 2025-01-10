//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <string.h>

#include "include_internal/axis_runtime/binding/python/common/common.h"
#include "include_internal/axis_runtime/binding/python/common/error.h"
#include "include_internal/axis_runtime/binding/python/axis_env/axis_env.h"
#include "axis_runtime/axis_env/internal/metadata.h"
#include "axis_runtime/axis_env_proxy/axis_env_proxy.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/macro/memory.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_json.h"

typedef struct axis_env_notify_set_property_async_ctx_t {
  axis_string_t path;
  axis_value_t *c_value;
  PyObject *py_cb_func;
} axis_env_notify_set_property_async_ctx_t;

static axis_env_notify_set_property_async_ctx_t *
axis_env_notify_set_property_async_ctx_create(const void *path,
                                             axis_value_t *value,
                                             PyObject *py_cb_func) {
  axis_env_notify_set_property_async_ctx_t *ctx =
      axis_MALLOC(sizeof(axis_env_notify_set_property_async_ctx_t));
  axis_ASSERT(ctx, "Failed to allocate memory.");

  axis_string_init_formatted(&ctx->path, "%s", path);
  ctx->c_value = value;
  ctx->py_cb_func = py_cb_func;

  if (py_cb_func != NULL) {
    Py_INCREF(py_cb_func);
  }

  return ctx;
}

static void axis_env_notify_set_property_async_ctx_destroy(
    axis_env_notify_set_property_async_ctx_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_string_deinit(&self->path);
  self->c_value = NULL;
  self->py_cb_func = NULL;

  axis_FREE(self);
}

static void axis_env_proxy_notify_set_property_async(axis_env_t *axis_env,
                                                    void *user_data) {
  axis_ASSERT(user_data, "Invalid argument.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_env_notify_set_property_async_ctx_t *ctx = user_data;
  axis_ASSERT(ctx, "Should not happen.");

  axis_ASSERT(ctx->py_cb_func, "Invalid argument.");

  axis_error_t err;
  axis_error_init(&err);

  bool rc = axis_env_set_property(axis_env, axis_string_get_raw_str(&ctx->path),
                                 ctx->c_value, &err);

  // About to call the Python function, so it's necessary to ensure that the GIL
  // has been acquired.
  //
  // Allows C codes to work safely with Python objects.
  PyGILState_STATE prev_state = axis_py_gil_state_ensure_internal();

  PyObject *arglist = NULL;
  axis_py_error_t *py_error = NULL;

  if (!rc) {
    py_error = axis_py_error_wrap(&err);

    arglist = Py_BuildValue("(O)", py_error);
  } else {
    arglist = Py_BuildValue("(O)", Py_None);
  }

  axis_error_deinit(&err);

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

  axis_env_notify_set_property_async_ctx_destroy(ctx);
}

static bool axis_py_axis_env_set_property_async(axis_py_axis_env_t *self,
                                              const void *path,
                                              axis_value_t *value,
                                              PyObject *py_cb_func,
                                              axis_error_t *err) {
  axis_ASSERT(self && axis_py_axis_env_check_integrity(self), "Invalid argument.");
  axis_ASSERT(value && axis_value_check_integrity(value), "Invalid argument.");
  axis_ASSERT(py_cb_func && PyCallable_Check(py_cb_func), "Invalid argument.");

  bool success = true;

  axis_env_notify_set_property_async_ctx_t *ctx =
      axis_env_notify_set_property_async_ctx_create(path, value, py_cb_func);
  if (!axis_env_proxy_notify(self->c_axis_env_proxy,
                            axis_env_proxy_notify_set_property_async, ctx, false,
                            err)) {
    Py_XDECREF(py_cb_func);

    axis_env_notify_set_property_async_ctx_destroy(ctx);
    success = false;
    axis_py_raise_py_runtime_error_exception("Failed to set property");
  }

  return success;
}

PyObject *axis_py_axis_env_set_property_from_json_async(PyObject *self,
                                                      PyObject *args) {
  axis_py_axis_env_t *py_axis_env = (axis_py_axis_env_t *)self;
  axis_ASSERT(py_axis_env && axis_py_axis_env_check_integrity(py_axis_env),
             "Invalid argument.");

  if (PyTuple_GET_SIZE(args) != 3) {
    return axis_py_raise_py_value_error_exception(
        "Invalid argument count when axis_env.set_property_from_json_async.");
  }

  const char *path = NULL;
  const char *json_str = NULL;
  PyObject *py_cb_func = NULL;
  if (!PyArg_ParseTuple(args, "ssO", &path, &json_str, &py_cb_func)) {
    return axis_py_raise_py_value_error_exception(
        "Failed to parse argument when axis_env.set_property_from_json_async.");
  }

  axis_json_t *json = axis_json_from_string(json_str, NULL);
  if (!json) {
    return axis_py_raise_py_value_error_exception(
        "Failed to parse json when axis_env.set_property_from_json_async.");
  }

  axis_value_t *value = axis_value_from_json(json);
  if (!value) {
    return axis_py_raise_py_value_error_exception(
        "Failed to convert json to value when "
        "axis_env.set_property_from_json_async.");
  }

  axis_json_destroy(json);

  axis_error_t err;
  axis_error_init(&err);

  bool rc = axis_py_axis_env_set_property_async(py_axis_env, path, value,
                                              py_cb_func, &err);

  if (!rc) {
    axis_value_destroy(value);
  }

  axis_error_deinit(&err);

  if (!rc) {
    return NULL;
  }

  Py_RETURN_NONE;
}

PyObject *axis_py_axis_env_set_property_string_async(PyObject *self,
                                                   PyObject *args) {
  axis_py_axis_env_t *py_axis_env = (axis_py_axis_env_t *)self;
  axis_ASSERT(py_axis_env && axis_py_axis_env_check_integrity(py_axis_env),
             "Invalid argument.");

  if (PyTuple_GET_SIZE(args) != 3) {
    return axis_py_raise_py_value_error_exception(
        "Invalid argument count when axis_env.set_property_string_async.");
  }

  const char *path = NULL;
  const char *value = NULL;
  PyObject *py_cb_func = NULL;
  if (!PyArg_ParseTuple(args, "ssO", &path, &value, &py_cb_func)) {
    return axis_py_raise_py_value_error_exception(
        "Failed to parse argument when axis_env.set_property_string_async.");
  }

  axis_value_t *c_value = axis_value_create_string(value);
  if (!c_value) {
    return axis_py_raise_py_value_error_exception(
        "Failed to create value when axis_env.set_property_string_async.");
  }

  axis_error_t err;
  axis_error_init(&err);

  bool rc = axis_py_axis_env_set_property_async(py_axis_env, path, c_value,
                                              py_cb_func, &err);

  if (!rc) {
    axis_value_destroy(c_value);
  }

  axis_error_deinit(&err);

  if (!rc) {
    return NULL;
  }

  Py_RETURN_NONE;
}

PyObject *axis_py_axis_env_set_property_int_async(PyObject *self,
                                                PyObject *args) {
  axis_py_axis_env_t *py_axis_env = (axis_py_axis_env_t *)self;
  axis_ASSERT(py_axis_env && axis_py_axis_env_check_integrity(py_axis_env),
             "Invalid argument.");

  if (PyTuple_GET_SIZE(args) != 3) {
    return axis_py_raise_py_value_error_exception(
        "Invalid argument count when axis_env.set_property_int_async.");
  }

  const char *path = NULL;
  int value = 0;
  PyObject *py_cb_func = NULL;
  if (!PyArg_ParseTuple(args, "siO", &path, &value, &py_cb_func)) {
    return axis_py_raise_py_value_error_exception(
        "Failed to parse argument when axis_env.set_property_int_async.");
  }

  axis_value_t *c_value = axis_value_create_int64(value);
  if (!c_value) {
    return axis_py_raise_py_value_error_exception(
        "Failed to create value when axis_env.set_property_int_async.");
  }

  axis_error_t err;
  axis_error_init(&err);

  bool rc = axis_py_axis_env_set_property_async(py_axis_env, path, c_value,
                                              py_cb_func, &err);

  if (!rc) {
    axis_value_destroy(c_value);
  }

  axis_error_deinit(&err);

  if (!rc) {
    return NULL;
  }

  Py_RETURN_NONE;
}

PyObject *axis_py_axis_env_set_property_bool_async(PyObject *self,
                                                 PyObject *args) {
  axis_py_axis_env_t *py_axis_env = (axis_py_axis_env_t *)self;
  axis_ASSERT(py_axis_env && axis_py_axis_env_check_integrity(py_axis_env),
             "Invalid argument.");

  if (PyTuple_GET_SIZE(args) != 3) {
    return axis_py_raise_py_value_error_exception(
        "Invalid argument count when axis_env.set_property_bool_async.");
  }

  const char *path = NULL;
  int value = 0;
  PyObject *py_cb_func = NULL;
  if (!PyArg_ParseTuple(args, "siO", &path, &value, &py_cb_func)) {
    return axis_py_raise_py_value_error_exception(
        "Failed to parse argument when axis_env.set_property_bool_async.");
  }

  axis_value_t *c_value = axis_value_create_bool(value);
  if (!c_value) {
    return axis_py_raise_py_value_error_exception(
        "Failed to create value when axis_env.set_property_bool_async.");
  }

  axis_error_t err;
  axis_error_init(&err);

  bool rc = axis_py_axis_env_set_property_async(py_axis_env, path, c_value,
                                              py_cb_func, &err);

  if (!rc) {
    axis_value_destroy(c_value);
  }

  axis_error_deinit(&err);

  if (!rc) {
    return NULL;
  }

  Py_RETURN_NONE;
}

PyObject *axis_py_axis_env_set_property_float_async(PyObject *self,
                                                  PyObject *args) {
  axis_py_axis_env_t *py_axis_env = (axis_py_axis_env_t *)self;
  axis_ASSERT(py_axis_env && axis_py_axis_env_check_integrity(py_axis_env),
             "Invalid argument.");

  if (PyTuple_GET_SIZE(args) != 3) {
    return axis_py_raise_py_value_error_exception(
        "Invalid argument count when axis_env.set_property_float_async.");
  }

  const char *path = NULL;
  double value = 0.0;
  PyObject *py_cb_func = NULL;
  if (!PyArg_ParseTuple(args, "sdO", &path, &value, &py_cb_func)) {
    return axis_py_raise_py_value_error_exception(
        "Failed to parse argument when axis_env.set_property_float_async.");
  }

  axis_value_t *c_value = axis_value_create_float64(value);
  if (!c_value) {
    return axis_py_raise_py_value_error_exception(
        "Failed to create value when axis_env.set_property_float_async.");
  }

  axis_error_t err;
  axis_error_init(&err);

  bool rc = axis_py_axis_env_set_property_async(py_axis_env, path, c_value,
                                              py_cb_func, &err);

  if (!rc) {
    axis_value_destroy(c_value);
  }

  axis_error_deinit(&err);

  if (!rc) {
    return NULL;
  }

  Py_RETURN_NONE;
}