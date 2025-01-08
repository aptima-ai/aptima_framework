//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <string.h>

#include "include_internal/axis_runtime/binding/python/common/error.h"
#include "include_internal/axis_runtime/binding/python/axis_env/axis_env.h"
#include "axis_runtime/axis_env_proxy/axis_env_proxy.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/memory.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_get.h"

typedef struct axis_env_notify_get_property_ctx_t {
  axis_string_t path;
  axis_value_t *c_value;
  axis_event_t *completed;
} axis_env_notify_get_property_ctx_t;

static axis_env_notify_get_property_ctx_t *
axis_env_notify_get_property_ctx_create(const void *path) {
  axis_env_notify_get_property_ctx_t *ctx =
      axis_MALLOC(sizeof(axis_env_notify_get_property_ctx_t));
  axis_ASSERT(ctx, "Failed to allocate memory.");

  axis_string_init_formatted(&ctx->path, "%s", path);
  ctx->c_value = NULL;
  ctx->completed = axis_event_create(0, 1);

  return ctx;
}

static void axis_env_notify_get_property_ctx_destroy(
    axis_env_notify_get_property_ctx_t *ctx) {
  axis_ASSERT(ctx, "Invalid argument.");

  axis_string_deinit(&ctx->path);
  ctx->c_value = NULL;
  axis_event_destroy(ctx->completed);

  axis_FREE(ctx);
}

static void axis_env_proxy_notify_get_property(axis_env_t *axis_env,
                                              void *user_data) {
  axis_ASSERT(user_data, "Invalid argument.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_env_notify_get_property_ctx_t *ctx = user_data;
  axis_ASSERT(ctx, "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  axis_value_t *c_value =
      axis_env_peek_property(axis_env, axis_string_get_raw_str(&ctx->path), &err);

  // Because this value will be passed out of the TEN world and back into the
  // Python world, and these two worlds are in different threads, copy semantics
  // are used to avoid thread safety issues.
  ctx->c_value = c_value ? axis_value_clone(c_value) : NULL;

  axis_event_set(ctx->completed);

  axis_error_deinit(&err);
}

static axis_value_t *axis_py_axis_property_get_and_check_if_exists(
    axis_py_axis_env_t *self, const char *path) {
  axis_ASSERT(self && axis_py_axis_env_check_integrity(self), "Invalid argument.");

  axis_value_t *c_value = NULL;

  axis_error_t err;
  axis_error_init(&err);

  axis_env_notify_get_property_ctx_t *ctx =
      axis_env_notify_get_property_ctx_create(path);

  if (!axis_env_proxy_notify(self->c_axis_env_proxy,
                            axis_env_proxy_notify_get_property, ctx, false,
                            &err)) {
    goto done;
  }

  PyThreadState *saved_py_thread_state = PyEval_SaveThread();
  axis_event_wait(ctx->completed, -1);
  PyEval_RestoreThread(saved_py_thread_state);

  c_value = ctx->c_value;

done:
  axis_error_deinit(&err);
  axis_env_notify_get_property_ctx_destroy(ctx);

  return c_value;
}

PyObject *axis_py_axis_env_get_property_to_json(PyObject *self, PyObject *args) {
  axis_py_axis_env_t *py_axis_env = (axis_py_axis_env_t *)self;
  axis_ASSERT(py_axis_env && axis_py_axis_env_check_integrity(py_axis_env),
             "Invalid argument.");

  if (PyTuple_GET_SIZE(args) != 1) {
    return axis_py_raise_py_value_error_exception(
        "Invalid argument count when axis_env.get_property_to_json.");
  }

  const char *path = NULL;
  if (!PyArg_ParseTuple(args, "s", &path)) {
    return axis_py_raise_py_value_error_exception(
        "Failed to parse argument when axis_env.get_property_to_json.");
  }

  axis_value_t *value =
      axis_py_axis_property_get_and_check_if_exists(py_axis_env, path);
  if (!value) {
    return axis_py_raise_py_value_error_exception("Failed to get property.");
  }

  axis_json_t *json = axis_value_to_json(value);
  if (!json) {
    axis_value_destroy(value);

    return axis_py_raise_py_value_error_exception(
        "axis_py_axis_get_property_to_json: failed to convert value to json.");
  }

  bool must_free = false;
  const char *json_str = axis_json_to_string(json, NULL, &must_free);
  axis_ASSERT(json_str, "Should not happen.");

  axis_value_destroy(value);
  axis_json_destroy(json);

  PyObject *result = PyUnicode_FromString(json_str);
  if (must_free) {
    axis_FREE(json_str);
  }

  return result;
}

PyObject *axis_py_axis_env_get_property_int(PyObject *self, PyObject *args) {
  axis_py_axis_env_t *py_axis_env = (axis_py_axis_env_t *)self;
  axis_ASSERT(py_axis_env && axis_py_axis_env_check_integrity(py_axis_env),
             "Invalid argument.");

  if (PyTuple_GET_SIZE(args) != 1) {
    return axis_py_raise_py_value_error_exception(
        "Invalid argument count when axis_env.get_property_int.");
  }

  const char *path = NULL;
  if (!PyArg_ParseTuple(args, "s", &path)) {
    return axis_py_raise_py_value_error_exception(
        "Failed to parse argument when axis_env.get_property_int.");
  }

  axis_value_t *value =
      axis_py_axis_property_get_and_check_if_exists(py_axis_env, path);
  if (!value) {
    return axis_py_raise_py_value_error_exception("Failed to get property.");
  }

  axis_error_t err;
  axis_error_init(&err);

  int64_t int_value = axis_value_get_int64(value, &err);
  if (!axis_error_is_success(&err)) {
    axis_value_destroy(value);
    axis_error_deinit(&err);

    return axis_py_raise_py_value_error_exception(
        "Failed to get int value from property.");
  }

  axis_value_destroy(value);
  axis_error_deinit(&err);
  return PyLong_FromLongLong(int_value);
}

PyObject *axis_py_axis_env_get_property_string(PyObject *self, PyObject *args) {
  axis_py_axis_env_t *py_axis_env = (axis_py_axis_env_t *)self;
  axis_ASSERT(py_axis_env && axis_py_axis_env_check_integrity(py_axis_env),
             "Invalid argument.");

  if (PyTuple_GET_SIZE(args) != 1) {
    return axis_py_raise_py_value_error_exception(
        "Invalid argument count when axis_env.get_property_string.");
  }

  const char *path = NULL;
  if (!PyArg_ParseTuple(args, "s", &path)) {
    return axis_py_raise_py_value_error_exception(
        "Failed to parse argument when axis_env.get_property_string.");
  }

  axis_value_t *value =
      axis_py_axis_property_get_and_check_if_exists(py_axis_env, path);
  if (!value) {
    return axis_py_raise_py_value_error_exception("Property [%s] is not found.",
                                                 path);
  }

  const char *str_value = axis_value_peek_raw_str(value, NULL);
  if (!str_value) {
    axis_value_destroy(value);

    return axis_py_raise_py_value_error_exception(
        "Property [%s] is not a string.", path);
  }

  // Note that `PyUnicode_FromString()` can not accept NULL.
  PyObject *result = PyUnicode_FromString(str_value);

  axis_value_destroy(value);
  return result;
}

PyObject *axis_py_axis_env_get_property_bool(PyObject *self, PyObject *args) {
  axis_py_axis_env_t *py_axis_env = (axis_py_axis_env_t *)self;
  axis_ASSERT(py_axis_env && axis_py_axis_env_check_integrity(py_axis_env),
             "Invalid argument.");

  if (PyTuple_GET_SIZE(args) != 1) {
    return axis_py_raise_py_value_error_exception(
        "Invalid argument count when axis_env.get_property_bool.");
  }

  const char *path = NULL;
  if (!PyArg_ParseTuple(args, "s", &path)) {
    return axis_py_raise_py_value_error_exception(
        "Failed to parse argument when axis_env.get_property_bool.");
  }

  axis_value_t *value =
      axis_py_axis_property_get_and_check_if_exists(py_axis_env, path);
  if (!value) {
    return axis_py_raise_py_value_error_exception("Failed to get property.");
  }

  axis_error_t err;
  axis_error_init(&err);

  bool bool_value = axis_value_get_bool(value, &err);
  if (!axis_error_is_success(&err)) {
    axis_value_destroy(value);
    axis_error_deinit(&err);

    return axis_py_raise_py_value_error_exception(
        "Failed to get bool value from property.");
  }

  axis_value_destroy(value);
  axis_error_deinit(&err);
  return PyBool_FromLong(bool_value);
}

PyObject *axis_py_axis_env_get_property_float(PyObject *self, PyObject *args) {
  axis_py_axis_env_t *py_axis_env = (axis_py_axis_env_t *)self;
  axis_ASSERT(py_axis_env && axis_py_axis_env_check_integrity(py_axis_env),
             "Invalid argument.");

  if (PyTuple_GET_SIZE(args) != 1) {
    return axis_py_raise_py_value_error_exception(
        "Invalid argument count when axis_env.get_property_float.");
  }

  const char *path = NULL;
  if (!PyArg_ParseTuple(args, "s", &path)) {
    return axis_py_raise_py_value_error_exception(
        "Failed to parse argument when axis_env.get_property_float.");
  }

  axis_value_t *value =
      axis_py_axis_property_get_and_check_if_exists(py_axis_env, path);
  if (!value) {
    return axis_py_raise_py_value_error_exception("Failed to get property.");
  }

  axis_error_t err;
  axis_error_init(&err);

  double float_value = axis_value_get_float64(value, &err);
  if (!axis_error_is_success(&err)) {
    axis_value_destroy(value);
    axis_error_deinit(&err);

    return axis_py_raise_py_value_error_exception(
        "Failed to get float value from property.");
  }

  axis_value_destroy(value);
  axis_error_deinit(&err);
  return PyFloat_FromDouble(float_value);
}

PyObject *axis_py_axis_env_is_property_exist(PyObject *self, PyObject *args) {
  axis_py_axis_env_t *py_axis_env = (axis_py_axis_env_t *)self;
  axis_ASSERT(py_axis_env && axis_py_axis_env_check_integrity(py_axis_env),
             "Invalid argument.");

  if (PyTuple_GET_SIZE(args) != 1) {
    return axis_py_raise_py_value_error_exception(
        "Invalid argument count when axis_env.is_property_exist.");
  }

  const char *path = NULL;
  if (!PyArg_ParseTuple(args, "s", &path)) {
    return axis_py_raise_py_value_error_exception(
        "Failed to parse argument when axis_env.is_property_exist.");
  }

  axis_value_t *value =
      axis_py_axis_property_get_and_check_if_exists(py_axis_env, path);
  if (!value) {
    return PyBool_FromLong(false);
  }

  axis_value_destroy(value);
  return PyBool_FromLong(true);
}
