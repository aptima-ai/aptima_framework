//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <string.h>

#include "include_internal/ten_runtime/binding/python/common/error.h"
#include "include_internal/ten_runtime/binding/python/ten_env/ten_env.h"
#include "ten_runtime/ten_env_proxy/ten_env_proxy.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/memory.h"
#include "ten_utils/value/value.h"
#include "ten_utils/value/value_get.h"

typedef struct ten_env_notify_get_property_info_t {
  ten_string_t path;
  ten_value_t *c_value;
  ten_event_t *completed;
} ten_env_notify_get_property_info_t;

static ten_env_notify_get_property_info_t *
ten_env_notify_get_property_info_create(const void *path) {
  ten_env_notify_get_property_info_t *info =
      TEN_MALLOC(sizeof(ten_env_notify_get_property_info_t));
  TEN_ASSERT(info, "Failed to allocate memory.");

  ten_string_init_formatted(&info->path, "%s", path);
  info->c_value = NULL;
  info->completed = ten_event_create(0, 1);

  return info;
}

static void ten_env_notify_get_property_info_destroy(
    ten_env_notify_get_property_info_t *info) {
  TEN_ASSERT(info, "Invalid argument.");

  ten_string_deinit(&info->path);
  info->c_value = NULL;
  ten_event_destroy(info->completed);

  TEN_FREE(info);
}

static void ten_env_proxy_notify_get_property(ten_env_t *ten_env,
                                              void *user_data) {
  TEN_ASSERT(user_data, "Invalid argument.");
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_env_notify_get_property_info_t *info = user_data;
  TEN_ASSERT(info, "Should not happen.");

  ten_error_t err;
  ten_error_init(&err);

  ten_value_t *c_value =
      ten_env_peek_property(ten_env, ten_string_get_raw_str(&info->path), &err);

  // Because this value will be passed out of the TEN world and back into the
  // Python world, and these two worlds are in different threads, copy semantics
  // are used to avoid thread safety issues.
  info->c_value = c_value ? ten_value_clone(c_value) : NULL;

  ten_event_set(info->completed);

  ten_error_deinit(&err);
}

static ten_value_t *ten_py_ten_property_get_and_check_if_exists(
    ten_py_ten_env_t *self, const char *path) {
  TEN_ASSERT(self && ten_py_ten_env_check_integrity(self), "Invalid argument.");

  ten_value_t *c_value = NULL;

  ten_error_t err;
  ten_error_init(&err);

  ten_env_notify_get_property_info_t *info =
      ten_env_notify_get_property_info_create(path);

  if (!ten_env_proxy_notify(self->c_ten_env_proxy,
                            ten_env_proxy_notify_get_property, info, false,
                            &err)) {
    goto done;
  }

  PyThreadState *saved_py_thread_state = PyEval_SaveThread();
  ten_event_wait(info->completed, -1);
  PyEval_RestoreThread(saved_py_thread_state);

  c_value = info->c_value;

done:
  ten_error_deinit(&err);
  ten_env_notify_get_property_info_destroy(info);

  return c_value;
}

PyObject *ten_py_ten_env_get_property_to_json(PyObject *self, PyObject *args) {
  ten_py_ten_env_t *py_ten_env = (ten_py_ten_env_t *)self;
  TEN_ASSERT(py_ten_env && ten_py_ten_env_check_integrity(py_ten_env),
             "Invalid argument.");

  if (PyTuple_GET_SIZE(args) != 1) {
    return ten_py_raise_py_value_error_exception(
        "Invalid argument count when ten_env.get_property_to_json.");
  }

  const char *path = NULL;
  if (!PyArg_ParseTuple(args, "s", &path)) {
    return ten_py_raise_py_value_error_exception(
        "Failed to parse argument when ten_env.get_property_to_json.");
  }

  ten_value_t *value =
      ten_py_ten_property_get_and_check_if_exists(py_ten_env, path);
  if (!value) {
    return ten_py_raise_py_value_error_exception("Failed to get property.");
  }

  ten_json_t *json = ten_value_to_json(value);
  if (!json) {
    ten_value_destroy(value);

    return ten_py_raise_py_value_error_exception(
        "ten_py_ten_get_property_to_json: failed to convert value to json.");
  }

  bool must_free = false;
  const char *json_str = ten_json_to_string(json, NULL, &must_free);
  TEN_ASSERT(json_str, "Should not happen.");

  ten_value_destroy(value);
  ten_json_destroy(json);

  PyObject *result = PyUnicode_FromString(json_str);
  if (must_free) {
    TEN_FREE(json_str);
  }

  return result;
}

PyObject *ten_py_ten_env_get_property_int(PyObject *self, PyObject *args) {
  ten_py_ten_env_t *py_ten_env = (ten_py_ten_env_t *)self;
  TEN_ASSERT(py_ten_env && ten_py_ten_env_check_integrity(py_ten_env),
             "Invalid argument.");

  if (PyTuple_GET_SIZE(args) != 1) {
    return ten_py_raise_py_value_error_exception(
        "Invalid argument count when ten_env.get_property_int.");
  }

  const char *path = NULL;
  if (!PyArg_ParseTuple(args, "s", &path)) {
    return ten_py_raise_py_value_error_exception(
        "Failed to parse argument when ten_env.get_property_int.");
  }

  ten_value_t *value =
      ten_py_ten_property_get_and_check_if_exists(py_ten_env, path);
  if (!value) {
    return ten_py_raise_py_value_error_exception("Failed to get property.");
  }

  ten_error_t err;
  ten_error_init(&err);

  int64_t int_value = ten_value_get_int64(value, &err);
  if (!ten_error_is_success(&err)) {
    ten_value_destroy(value);
    ten_error_deinit(&err);

    return ten_py_raise_py_value_error_exception(
        "Failed to get int value from property.");
  }

  ten_value_destroy(value);
  ten_error_deinit(&err);
  return PyLong_FromLongLong(int_value);
}

PyObject *ten_py_ten_env_get_property_string(PyObject *self, PyObject *args) {
  ten_py_ten_env_t *py_ten_env = (ten_py_ten_env_t *)self;
  TEN_ASSERT(py_ten_env && ten_py_ten_env_check_integrity(py_ten_env),
             "Invalid argument.");

  if (PyTuple_GET_SIZE(args) != 1) {
    return ten_py_raise_py_value_error_exception(
        "Invalid argument count when ten_env.get_property_string.");
  }

  const char *path = NULL;
  if (!PyArg_ParseTuple(args, "s", &path)) {
    return ten_py_raise_py_value_error_exception(
        "Failed to parse argument when ten_env.get_property_string.");
  }

  ten_value_t *value =
      ten_py_ten_property_get_and_check_if_exists(py_ten_env, path);
  if (!value) {
    return ten_py_raise_py_value_error_exception("Property [%s] is not found.",
                                                 path);
  }

  const char *str_value = ten_value_peek_c_str(value);
  if (!str_value) {
    ten_value_destroy(value);

    return ten_py_raise_py_value_error_exception(
        "Property [%s] is not a string.", path);
  }

  // Note that `PyUnicode_FromString()` can not accept NULL.
  PyObject *result = PyUnicode_FromString(str_value);

  ten_value_destroy(value);
  return result;
}

PyObject *ten_py_ten_env_get_property_bool(PyObject *self, PyObject *args) {
  ten_py_ten_env_t *py_ten_env = (ten_py_ten_env_t *)self;
  TEN_ASSERT(py_ten_env && ten_py_ten_env_check_integrity(py_ten_env),
             "Invalid argument.");

  if (PyTuple_GET_SIZE(args) != 1) {
    return ten_py_raise_py_value_error_exception(
        "Invalid argument count when ten_env.get_property_bool.");
  }

  const char *path = NULL;
  if (!PyArg_ParseTuple(args, "s", &path)) {
    return ten_py_raise_py_value_error_exception(
        "Failed to parse argument when ten_env.get_property_bool.");
  }

  ten_value_t *value =
      ten_py_ten_property_get_and_check_if_exists(py_ten_env, path);
  if (!value) {
    return ten_py_raise_py_value_error_exception("Failed to get property.");
  }

  ten_error_t err;
  ten_error_init(&err);

  bool bool_value = ten_value_get_bool(value, &err);
  if (!ten_error_is_success(&err)) {
    ten_value_destroy(value);
    ten_error_deinit(&err);

    return ten_py_raise_py_value_error_exception(
        "Failed to get bool value from property.");
  }

  ten_value_destroy(value);
  ten_error_deinit(&err);
  return PyBool_FromLong(bool_value);
}

PyObject *ten_py_ten_env_get_property_float(PyObject *self, PyObject *args) {
  ten_py_ten_env_t *py_ten_env = (ten_py_ten_env_t *)self;
  TEN_ASSERT(py_ten_env && ten_py_ten_env_check_integrity(py_ten_env),
             "Invalid argument.");

  if (PyTuple_GET_SIZE(args) != 1) {
    return ten_py_raise_py_value_error_exception(
        "Invalid argument count when ten_env.get_property_float.");
  }

  const char *path = NULL;
  if (!PyArg_ParseTuple(args, "s", &path)) {
    return ten_py_raise_py_value_error_exception(
        "Failed to parse argument when ten_env.get_property_float.");
  }

  ten_value_t *value =
      ten_py_ten_property_get_and_check_if_exists(py_ten_env, path);
  if (!value) {
    return ten_py_raise_py_value_error_exception("Failed to get property.");
  }

  ten_error_t err;
  ten_error_init(&err);

  double float_value = ten_value_get_float64(value, &err);
  if (!ten_error_is_success(&err)) {
    ten_value_destroy(value);
    ten_error_deinit(&err);

    return ten_py_raise_py_value_error_exception(
        "Failed to get float value from property.");
  }

  ten_value_destroy(value);
  ten_error_deinit(&err);
  return PyFloat_FromDouble(float_value);
}

PyObject *ten_py_ten_env_is_property_exist(PyObject *self, PyObject *args) {
  ten_py_ten_env_t *py_ten_env = (ten_py_ten_env_t *)self;
  TEN_ASSERT(py_ten_env && ten_py_ten_env_check_integrity(py_ten_env),
             "Invalid argument.");

  if (PyTuple_GET_SIZE(args) != 1) {
    return ten_py_raise_py_value_error_exception(
        "Invalid argument count when ten_env.is_property_exist.");
  }

  const char *path = NULL;
  if (!PyArg_ParseTuple(args, "s", &path)) {
    return ten_py_raise_py_value_error_exception(
        "Failed to parse argument when ten_env.is_property_exist.");
  }

  ten_error_t err;
  ten_error_init(&err);

  bool is_exist = ten_env_is_property_exist(py_ten_env->c_ten_env, path, &err);
  if (!ten_error_is_success(&err)) {
    ten_error_deinit(&err);

    return ten_py_raise_py_value_error_exception(
        "Failed to check if property exists.");
  }

  ten_error_deinit(&err);
  return PyBool_FromLong(is_exist);
}
