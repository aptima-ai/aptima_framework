//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <string.h>

#include "include_internal/axis_runtime/binding/python/common/error.h"
#include "include_internal/axis_runtime/binding/python/axis_env/axis_env.h"
#include "axis_runtime/axis_env/internal/metadata.h"
#include "axis_runtime/axis_env_proxy/axis_env_proxy.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/macro/memory.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_json.h"

typedef struct axis_env_notify_set_property_ctx_t {
  bool result;
  axis_string_t path;
  axis_value_t *c_value;
  axis_event_t *completed;
} axis_env_notify_set_property_ctx_t;

static axis_env_notify_set_property_ctx_t *
axis_env_notify_set_property_ctx_create(const void *path, axis_value_t *value) {
  axis_env_notify_set_property_ctx_t *ctx =
      axis_MALLOC(sizeof(axis_env_notify_set_property_ctx_t));
  axis_ASSERT(ctx, "Failed to allocate memory.");

  ctx->result = true;
  axis_string_init_formatted(&ctx->path, "%s", path);
  ctx->c_value = value;
  ctx->completed = axis_event_create(0, 1);

  return ctx;
}

static void axis_env_notify_set_property_ctx_destroy(
    axis_env_notify_set_property_ctx_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_string_deinit(&self->path);
  self->c_value = NULL;
  axis_event_destroy(self->completed);

  axis_FREE(self);
}

static void axis_env_proxy_notify_set_property(axis_env_t *axis_env,
                                              void *user_data) {
  axis_ASSERT(user_data, "Invalid argument.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_env_notify_set_property_ctx_t *ctx = user_data;
  axis_ASSERT(ctx, "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  ctx->result = axis_env_set_property(
      axis_env, axis_string_get_raw_str(&ctx->path), ctx->c_value, &err);
  axis_ASSERT(ctx->result, "Should not happen.");

  axis_event_set(ctx->completed);

  axis_error_deinit(&err);
}

static void axis_py_axis_env_set_property(axis_py_axis_env_t *self,
                                        const void *path, axis_value_t *value) {
  axis_ASSERT(self && axis_py_axis_env_check_integrity(self), "Invalid argument.");
  axis_ASSERT(value && axis_value_check_integrity(value), "Invalid argument.");

  axis_error_t err;
  axis_error_init(&err);

  axis_env_notify_set_property_ctx_t *ctx =
      axis_env_notify_set_property_ctx_create(path, value);

  if (!axis_env_proxy_notify(self->c_axis_env_proxy,
                            axis_env_proxy_notify_set_property, ctx, false,
                            &err)) {
    goto done;
  }

  PyThreadState *saved_py_thread_state = PyEval_SaveThread();
  axis_event_wait(ctx->completed, -1);
  PyEval_RestoreThread(saved_py_thread_state);

done:
  axis_env_notify_set_property_ctx_destroy(ctx);
  axis_error_deinit(&err);
}

PyObject *axis_py_axis_env_set_property_from_json(PyObject *self,
                                                PyObject *args) {
  axis_py_axis_env_t *py_axis_env = (axis_py_axis_env_t *)self;
  axis_ASSERT(py_axis_env && axis_py_axis_env_check_integrity(py_axis_env),
             "Invalid argument.");

  if (PyTuple_GET_SIZE(args) != 2) {
    return axis_py_raise_py_value_error_exception(
        "Invalid argument count when axis_env.set_property_from_json.");
  }

  const char *path = NULL;
  const char *json_str = NULL;
  if (!PyArg_ParseTuple(args, "ss", &path, &json_str)) {
    return axis_py_raise_py_value_error_exception(
        "Failed to parse arguments when axis_env.set_property_from_json.");
  }

  axis_json_t *json = axis_json_from_string(json_str, NULL);
  if (!json) {
    return axis_py_raise_py_value_error_exception(
        "Failed to parse json when axis_env.set_property_from_json.");
  }

  axis_value_t *value = axis_value_from_json(json);
  if (!value) {
    return axis_py_raise_py_value_error_exception(
        "Failed to convert json to value when axis_env.set_property_from_json.");
  }

  axis_json_destroy(json);

  axis_py_axis_env_set_property(py_axis_env, path, value);

  Py_RETURN_NONE;
}

PyObject *axis_py_axis_env_set_property_int(PyObject *self, PyObject *args) {
  axis_py_axis_env_t *py_axis_env = (axis_py_axis_env_t *)self;
  axis_ASSERT(py_axis_env && axis_py_axis_env_check_integrity(py_axis_env),
             "Invalid argument.");

  if (PyTuple_GET_SIZE(args) != 2) {
    return axis_py_raise_py_value_error_exception(
        "Invalid argument count when axis_env.set_property_int.");
  }

  const char *path = NULL;
  int value = 0;
  if (!PyArg_ParseTuple(args, "si", &path, &value)) {
    return axis_py_raise_py_value_error_exception(
        "Failed to parse arguments when axis_env.set_property_int.");
  }

  axis_value_t *c_value = axis_value_create_int64(value);
  if (!c_value) {
    return axis_py_raise_py_value_error_exception(
        "Failed to create value when axis_env.set_property_int.");
  }

  axis_py_axis_env_set_property(py_axis_env, path, c_value);

  Py_RETURN_NONE;
}

PyObject *axis_py_axis_env_set_property_string(PyObject *self, PyObject *args) {
  axis_py_axis_env_t *py_axis_env = (axis_py_axis_env_t *)self;
  axis_ASSERT(py_axis_env && axis_py_axis_env_check_integrity(py_axis_env),
             "Invalid argument.");

  if (PyTuple_GET_SIZE(args) != 2) {
    return axis_py_raise_py_value_error_exception(
        "Invalid argument count when axis_env.set_property_string.");
  }

  const char *path = NULL;
  const char *value = NULL;
  if (!PyArg_ParseTuple(args, "ss", &path, &value)) {
    return axis_py_raise_py_value_error_exception(
        "Failed to parse arguments when axis_env.set_property_string.");
  }

  axis_value_t *c_value = axis_value_create_string(value);
  if (!c_value) {
    return axis_py_raise_py_value_error_exception(
        "Failed to create value when axis_env.set_property_string.");
  }

  axis_py_axis_env_set_property(py_axis_env, path, c_value);

  Py_RETURN_NONE;
}

PyObject *axis_py_axis_env_set_property_bool(PyObject *self, PyObject *args) {
  axis_py_axis_env_t *py_axis_env = (axis_py_axis_env_t *)self;
  axis_ASSERT(py_axis_env && axis_py_axis_env_check_integrity(py_axis_env),
             "Invalid argument.");

  if (PyTuple_GET_SIZE(args) != 2) {
    return axis_py_raise_py_value_error_exception(
        "Invalid argument count when axis_env.set_property_bool.");
  }

  const char *path = NULL;
  int value = 0;
  if (!PyArg_ParseTuple(args, "si", &path, &value)) {
    return axis_py_raise_py_value_error_exception(
        "Failed to parse arguments when axis_env.set_property_bool.");
  }

  axis_value_t *c_value = axis_value_create_bool(value > 0);
  if (!c_value) {
    return axis_py_raise_py_value_error_exception(
        "Failed to create value when axis_env.set_property_bool.");
  }

  axis_py_axis_env_set_property(py_axis_env, path, c_value);

  Py_RETURN_NONE;
}

PyObject *axis_py_axis_env_set_property_float(PyObject *self, PyObject *args) {
  axis_py_axis_env_t *py_axis_env = (axis_py_axis_env_t *)self;
  axis_ASSERT(py_axis_env && axis_py_axis_env_check_integrity(py_axis_env),
             "Invalid argument.");

  if (PyTuple_GET_SIZE(args) != 2) {
    return axis_py_raise_py_value_error_exception(
        "Invalid argument count when axis_env.set_property_float.");
  }

  const char *path = NULL;
  double value = 0.0;
  if (!PyArg_ParseTuple(args, "sd", &path, &value)) {
    return axis_py_raise_py_value_error_exception(
        "Failed to parse arguments when axis_env.set_property_float.");
  }

  axis_value_t *c_value = axis_value_create_float64(value);
  if (!c_value) {
    return axis_py_raise_py_value_error_exception(
        "Failed to create value when axis_env.set_property_float.");
  }

  axis_py_axis_env_set_property(py_axis_env, path, c_value);

  Py_RETURN_NONE;
}
