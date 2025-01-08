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
#include "axis_runtime/axis_env_proxy/axis_env_proxy.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/memory.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_get.h"

typedef void (*axis_py_get_property_cb)(axis_env_t *axis_env, axis_value_t *value,
                                       axis_error_t *error,
                                       PyObject *py_cb_func);

typedef struct axis_env_notify_get_property_async_ctx_t {
  axis_string_t path;
  PyObject *py_cb_func;
  axis_py_get_property_cb cb;
} axis_env_notify_get_property_async_ctx_t;

static axis_env_notify_get_property_async_ctx_t *
axis_env_notify_get_property_async_ctx_create(const char *path,
                                             PyObject *py_cb_func,
                                             axis_py_get_property_cb cb) {
  axis_env_notify_get_property_async_ctx_t *ctx =
      axis_MALLOC(sizeof(axis_env_notify_get_property_async_ctx_t));
  axis_ASSERT(ctx, "Failed to allocate memory.");

  axis_string_init_formatted(&ctx->path, "%s", path);
  ctx->py_cb_func = py_cb_func;
  ctx->cb = cb;

  if (py_cb_func != NULL) {
    Py_INCREF(py_cb_func);
  }

  return ctx;
}

static void axis_env_notify_get_property_async_ctx_destroy(
    axis_env_notify_get_property_async_ctx_t *ctx) {
  axis_ASSERT(ctx, "Invalid argument.");

  axis_string_deinit(&ctx->path);

  ctx->py_cb_func = NULL;
  ctx->cb = NULL;

  axis_FREE(ctx);
}

static void axis_env_proxy_notify_get_property(axis_env_t *axis_env,
                                              void *user_data) {
  axis_ASSERT(user_data, "Invalid argument.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_env_notify_get_property_async_ctx_t *ctx = user_data;
  axis_ASSERT(ctx, "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  axis_value_t *c_value =
      axis_env_peek_property(axis_env, axis_string_get_raw_str(&ctx->path), &err);

  if (c_value) {
    ctx->cb(axis_env, c_value, NULL, ctx->py_cb_func);
  } else {
    ctx->cb(axis_env, NULL, &err, ctx->py_cb_func);
  }

  axis_error_deinit(&err);
  axis_env_notify_get_property_async_ctx_destroy(ctx);
}

static bool axis_py_get_property_async(axis_py_axis_env_t *self, const char *path,
                                      PyObject *py_cb_func,
                                      axis_py_get_property_cb cb,
                                      axis_error_t *error) {
  axis_ASSERT(self && axis_py_axis_env_check_integrity(self), "Invalid argument.");

  bool success = true;

  axis_env_notify_get_property_async_ctx_t *ctx =
      axis_env_notify_get_property_async_ctx_create(path, py_cb_func, cb);
  if (!axis_env_proxy_notify(self->c_axis_env_proxy,
                            axis_env_proxy_notify_get_property, ctx, false,
                            error)) {
    if (py_cb_func) {
      Py_XDECREF(py_cb_func);
    }

    axis_env_notify_get_property_async_ctx_destroy(ctx);
    success = false;
    axis_py_raise_py_runtime_error_exception("Failed to get property");
  }

  return success;
}

static void axis_py_get_property_to_json_cb(axis_env_t *axis_env,
                                           axis_value_t *value,
                                           axis_error_t *error,
                                           PyObject *py_cb_func) {
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  // About to call the Python function, so it's necessary to ensure that the GIL
  // has been acquired.
  //
  // Allows C codes to work safely with Python objects.
  PyGILState_STATE prev_state = axis_py_gil_state_ensure_internal();

  PyObject *arglist = NULL;
  axis_py_error_t *py_error = NULL;
  const char *json_str = "";

  if (error) {
    py_error = axis_py_error_wrap(error);

    arglist = Py_BuildValue("(sO)", json_str, py_error);
  } else {
    axis_json_t *json = axis_value_to_json(value);
    axis_ASSERT(json, "Should not happen.");

    bool must_free = false;
    json_str = axis_json_to_string(json, NULL, &must_free);
    axis_ASSERT(json_str, "Should not happen.");

    arglist = Py_BuildValue("(sO)", json_str, Py_None);

    axis_json_destroy(json);

    if (must_free) {
      axis_FREE(json_str);
    }
  }

  PyObject *result = PyObject_CallObject(py_cb_func, arglist);
  Py_XDECREF(result);  // Ensure cleanup if an error occurred.

  bool err_occurred = axis_py_check_and_clear_py_error();
  axis_ASSERT(!err_occurred, "Should not happen.");

  Py_XDECREF(arglist);
  Py_XDECREF(py_cb_func);

  if (py_error) {
    axis_py_error_invalidate(py_error);
  }

  axis_py_gil_state_release_internal(prev_state);
}

static void axis_py_get_property_int_cb(axis_env_t *axis_env, axis_value_t *value,
                                       axis_error_t *error,
                                       PyObject *py_cb_func) {
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  // About to call the Python function, so it's necessary to ensure that the GIL
  // has been acquired.
  //
  // Allows C codes to work safely with Python objects.
  PyGILState_STATE prev_state = axis_py_gil_state_ensure_internal();

  PyObject *arglist = NULL;
  axis_py_error_t *py_error = NULL;
  int64_t int_value = 0;

  if (error) {
    py_error = axis_py_error_wrap(error);

    arglist = Py_BuildValue("(iO)", int_value, py_error);
  } else {
    axis_error_t err;
    axis_error_init(&err);

    int_value = axis_value_get_int64(value, error);

    if (axis_error_is_success(&err)) {
      arglist = Py_BuildValue("(iO)", int_value, Py_None);
    } else {
      py_error = axis_py_error_wrap(&err);
      arglist = Py_BuildValue("(iO)", int_value, py_error);
    }

    axis_error_deinit(&err);
  }

  PyObject *result = PyObject_CallObject(py_cb_func, arglist);
  Py_XDECREF(result);  // Ensure cleanup if an error occurred.

  bool err_occurred = axis_py_check_and_clear_py_error();
  axis_ASSERT(!err_occurred, "Should not happen.");

  Py_XDECREF(arglist);
  Py_XDECREF(py_cb_func);

  if (py_error) {
    axis_py_error_invalidate(py_error);
  }

  axis_py_gil_state_release_internal(prev_state);
}

static void axis_py_get_property_string_cb(axis_env_t *axis_env,
                                          axis_value_t *value,
                                          axis_error_t *error,
                                          PyObject *py_cb_func) {
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  // About to call the Python function, so it's necessary to ensure that the GIL
  // has been acquired.
  //
  // Allows C codes to work safely with Python objects.
  PyGILState_STATE prev_state = axis_py_gil_state_ensure_internal();

  PyObject *arglist = NULL;
  axis_py_error_t *py_error = NULL;
  const char *str_value = "";

  if (error) {
    py_error = axis_py_error_wrap(error);

    arglist = Py_BuildValue("(sO)", str_value, py_error);
  } else {
    axis_error_t err;
    axis_error_init(&err);

    str_value = axis_value_peek_raw_str(value, &err);

    if (axis_error_is_success(&err)) {
      arglist = Py_BuildValue("(sO)", str_value, Py_None);
    } else {
      py_error = axis_py_error_wrap(&err);
      arglist = Py_BuildValue("(sO)", str_value, py_error);
    }

    axis_error_deinit(&err);
  }

  PyObject *result = PyObject_CallObject(py_cb_func, arglist);
  Py_XDECREF(result);  // Ensure cleanup if an error occurred.

  bool err_occurred = axis_py_check_and_clear_py_error();
  axis_ASSERT(!err_occurred, "Should not happen.");

  Py_XDECREF(arglist);
  Py_XDECREF(py_cb_func);

  if (py_error) {
    axis_py_error_invalidate(py_error);
  }

  axis_py_gil_state_release_internal(prev_state);
}

static void axis_py_get_property_bool_cb(axis_env_t *axis_env, axis_value_t *value,
                                        axis_error_t *error,
                                        PyObject *py_cb_func) {
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  // About to call the Python function, so it's necessary to ensure that the GIL
  // has been acquired.
  //
  // Allows C codes to work safely with Python objects.
  PyGILState_STATE prev_state = axis_py_gil_state_ensure_internal();

  PyObject *arglist = NULL;
  axis_py_error_t *py_error = NULL;
  bool bool_value = false;

  if (error) {
    py_error = axis_py_error_wrap(error);

    arglist = Py_BuildValue("(OO)", bool_value ? Py_True : Py_False, py_error);
  } else {
    axis_error_t err;
    axis_error_init(&err);

    bool_value = axis_value_get_bool(value, &err);

    if (axis_error_is_success(&err)) {
      arglist = Py_BuildValue("(OO)", bool_value ? Py_True : Py_False, Py_None);
    } else {
      py_error = axis_py_error_wrap(&err);
      arglist =
          Py_BuildValue("(OO)", bool_value ? Py_True : Py_False, py_error);
    }

    axis_error_deinit(&err);
  }

  PyObject *result = PyObject_CallObject(py_cb_func, arglist);
  Py_XDECREF(result);  // Ensure cleanup if an error occurred.

  bool err_occurred = axis_py_check_and_clear_py_error();
  axis_ASSERT(!err_occurred, "Should not happen.");

  Py_XDECREF(arglist);
  Py_XDECREF(py_cb_func);

  if (py_error) {
    axis_py_error_invalidate(py_error);
  }

  axis_py_gil_state_release_internal(prev_state);
}

static void axis_py_get_property_float_cb(axis_env_t *axis_env, axis_value_t *value,
                                         axis_error_t *error,
                                         PyObject *py_cb_func) {
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  // About to call the Python function, so it's necessary to ensure that the GIL
  // has been acquired.
  //
  // Allows C codes to work safely with Python objects.
  PyGILState_STATE prev_state = axis_py_gil_state_ensure_internal();

  PyObject *arglist = NULL;
  axis_py_error_t *py_error = NULL;
  double float_value = 0.0;

  if (error) {
    py_error = axis_py_error_wrap(error);

    arglist = Py_BuildValue("(dO)", float_value, py_error);
  } else {
    axis_error_t err;
    axis_error_init(&err);

    float_value = axis_value_get_float64(value, &err);

    if (axis_error_is_success(&err)) {
      arglist = Py_BuildValue("(dO)", float_value, Py_None);
    } else {
      py_error = axis_py_error_wrap(&err);
      arglist = Py_BuildValue("(dO)", float_value, py_error);
    }

    axis_error_deinit(&err);
  }

  PyObject *result = PyObject_CallObject(py_cb_func, arglist);
  Py_XDECREF(result);  // Ensure cleanup if an error occurred.

  bool err_occurred = axis_py_check_and_clear_py_error();
  axis_ASSERT(!err_occurred, "Should not happen.");

  Py_XDECREF(arglist);
  Py_XDECREF(py_cb_func);

  if (py_error) {
    axis_py_error_invalidate(py_error);
  }

  axis_py_gil_state_release_internal(prev_state);
}

static void axis_py_is_property_exist_cb(axis_env_t *axis_env, axis_value_t *value,
                                        axis_error_t *error,
                                        PyObject *py_cb_func) {
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  // About to call the Python function, so it's necessary to ensure that the GIL
  // has been acquired.
  //
  // Allows C codes to work safely with Python objects.
  PyGILState_STATE prev_state = axis_py_gil_state_ensure_internal();

  PyObject *arglist = NULL;
  bool is_exist = value != NULL;

  arglist = Py_BuildValue("(O)", is_exist ? Py_True : Py_False);

  PyObject *result = PyObject_CallObject(py_cb_func, arglist);
  Py_XDECREF(result);  // Ensure cleanup if an error occurred.

  bool err_occurred = axis_py_check_and_clear_py_error();
  axis_ASSERT(!err_occurred, "Should not happen.");

  Py_XDECREF(arglist);
  Py_XDECREF(py_cb_func);

  axis_py_gil_state_release_internal(prev_state);
}

static PyObject *axis_py_axis_env_get_property_async(PyObject *self,
                                                   PyObject *args,
                                                   axis_py_get_property_cb cb) {
  axis_py_axis_env_t *py_axis_env = (axis_py_axis_env_t *)self;
  axis_ASSERT(py_axis_env && axis_py_axis_env_check_integrity(py_axis_env),
             "Invalid argument.");

  if (PyTuple_GET_SIZE(args) != 2) {
    return axis_py_raise_py_value_error_exception(
        "Invalid argument count when axis_env.get_property_to_json_async.");
  }

  const char *path = NULL;
  PyObject *cb_func = NULL;
  if (!PyArg_ParseTuple(args, "sO", &path, &cb_func)) {
    return axis_py_raise_py_value_error_exception(
        "Failed to parse argument when axis_env.get_property_to_json_async.");
  }

  // Check if cb_func is callable.
  if (!PyCallable_Check(cb_func)) {
    return axis_py_raise_py_value_error_exception(
        "Invalid callback function when axis_env.get_property_to_json_async.");
  }

  axis_error_t err;
  axis_error_init(&err);

  bool rc = axis_py_get_property_async(py_axis_env, path, cb_func, cb, &err);

  axis_error_deinit(&err);

  if (!rc) {
    return NULL;
  }

  Py_RETURN_NONE;
}

PyObject *axis_py_axis_env_get_property_to_json_async(PyObject *self,
                                                    PyObject *args) {
  return axis_py_axis_env_get_property_async(self, args,
                                           axis_py_get_property_to_json_cb);
}

PyObject *axis_py_axis_env_get_property_int_async(PyObject *self,
                                                PyObject *args) {
  return axis_py_axis_env_get_property_async(self, args,
                                           axis_py_get_property_int_cb);
}

PyObject *axis_py_axis_env_get_property_string_async(PyObject *self,
                                                   PyObject *args) {
  return axis_py_axis_env_get_property_async(self, args,
                                           axis_py_get_property_string_cb);
}

PyObject *axis_py_axis_env_get_property_bool_async(PyObject *self,
                                                 PyObject *args) {
  return axis_py_axis_env_get_property_async(self, args,
                                           axis_py_get_property_bool_cb);
}

PyObject *axis_py_axis_env_get_property_float_async(PyObject *self,
                                                  PyObject *args) {
  return axis_py_axis_env_get_property_async(self, args,
                                           axis_py_get_property_float_cb);
}

PyObject *axis_py_axis_env_is_property_exist_async(PyObject *self,
                                                 PyObject *args) {
  return axis_py_axis_env_get_property_async(self, args,
                                           axis_py_is_property_exist_cb);
}
