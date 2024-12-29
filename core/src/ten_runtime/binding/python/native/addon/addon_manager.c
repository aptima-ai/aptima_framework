//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/binding/python/addon/addon_manager.h"

#include "include_internal/ten_runtime/binding/python/addon/addon.h"
#include "include_internal/ten_runtime/binding/python/common/error.h"
#include "ten_runtime/addon/extension/extension.h"

PyObject *ten_py_addon_manager_register_addon_as_extension(PyObject *self,
                                                           PyObject *args) {
  const char *name = NULL;
  PyObject *base_dir = NULL;
  PyObject *py_addon_object = NULL;
  PyObject *py_register_ctx = NULL;

  if (!PyArg_ParseTuple(args, "sOOO", &name, &base_dir, &py_addon_object,
                        &py_register_ctx)) {
    return ten_py_raise_py_value_error_exception(
        "Failed to parse arguments when registering addon.");
  }

  if (base_dir != Py_None && !PyUnicode_Check(base_dir)) {
    return ten_py_raise_py_type_error_exception(
        "base_dir must be a string or None.");
  }

  if (!PyObject_TypeCheck(py_addon_object, ten_py_addon_py_type())) {
    return ten_py_raise_py_type_error_exception(
        "Object is not an instance of Python Addon.");
  }

  const char *base_dir_str = NULL;
  if (base_dir != Py_None) {
    base_dir_str = PyUnicode_AsUTF8(base_dir);
    if (!base_dir_str) {
      return ten_py_raise_py_value_error_exception(
          "Failed to convert base_dir to UTF-8 string.");
    }
  }

  ten_py_addon_t *py_addon = (ten_py_addon_t *)py_addon_object;

  ten_addon_host_t *c_addon_host = ten_addon_register_extension(
      name, base_dir_str, &py_addon->c_addon, /* register_ctx= */ NULL);
  if (!c_addon_host) {
    return ten_py_raise_py_value_error_exception(
        "Failed to register addon in ten_addon_register_extension.");
  }

  py_addon->c_addon_host = c_addon_host;

  Py_INCREF(py_addon_object);  // Ensure the object is kept alive.

  Py_RETURN_NONE;
}
