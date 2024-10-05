//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/binding/python/common/error.h"
#include "include_internal/ten_runtime/binding/python/ten_env/ten_env.h"

PyObject *ten_py_ten_env_is_cmd_connected(PyObject *self, PyObject *args) {
  ten_py_ten_env_t *py_ten_env = (ten_py_ten_env_t *)self;
  TEN_ASSERT(py_ten_env && ten_py_ten_env_check_integrity(py_ten_env),
             "Invalid argument.");

  if (PyTuple_GET_SIZE(args) != 1) {
    return ten_py_raise_py_value_error_exception(
        "Invalid argument count when ten_env.is_cmd_connected.");
  }

  const char *cmd_name = NULL;
  if (!PyArg_ParseTuple(args, "s", &cmd_name)) {
    return ten_py_raise_py_value_error_exception(
        "Failed to parse argument when ten_env.is_cmd_connected.");
  }

  ten_error_t err;
  ten_error_init(&err);

  bool is_connected =
      ten_env_is_cmd_connected(py_ten_env->c_ten_env, cmd_name, &err);
  if (!ten_error_is_success(&err)) {
    ten_error_deinit(&err);

    return ten_py_raise_py_value_error_exception(
        "Failed to check if command is connected.");
  }

  ten_error_deinit(&err);
  return PyBool_FromLong(is_connected);
}
