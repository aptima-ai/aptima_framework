//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/binding/python/msg/cmd_result.h"

#include "include_internal/axis_runtime/binding/python/common/error.h"
#include "include_internal/axis_runtime/binding/python/msg/msg.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd_result/cmd.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "axis_runtime/common/status_code.h"
#include "axis_runtime/msg/cmd_result/cmd_result.h"

static axis_py_cmd_result_t *axis_py_cmd_result_create_internal(
    PyTypeObject *py_type) {
  if (!py_type) {
    py_type = axis_py_cmd_result_py_type();
  }

  axis_py_cmd_result_t *py_cmd_result =
      (axis_py_cmd_result_t *)py_type->tp_alloc(py_type, 0);

  axis_signature_set(&py_cmd_result->msg.signature, axis_PY_MSG_SIGNATURE);
  py_cmd_result->msg.c_msg = NULL;

  return py_cmd_result;
}

void axis_py_cmd_result_destroy(PyObject *self) {
  axis_py_cmd_result_t *py_cmd_result = (axis_py_cmd_result_t *)self;

  axis_ASSERT(py_cmd_result &&
                 axis_py_msg_check_integrity((axis_py_msg_t *)py_cmd_result),
             "Invalid argument.");

  axis_py_msg_destroy_c_msg(&py_cmd_result->msg);
  Py_TYPE(self)->tp_free(self);
}

static axis_py_cmd_result_t *axis_py_cmd_result_init(
    axis_py_cmd_result_t *py_cmd_result, axis_UNUSED PyObject *args,
    axis_UNUSED PyObject *kw) {
  axis_ASSERT(py_cmd_result &&
                 axis_py_msg_check_integrity((axis_py_msg_t *)py_cmd_result),
             "Invalid argument.");

  py_cmd_result->msg.c_msg =
      axis_msg_create_from_msg_type(axis_MSG_TYPE_CMD_RESULT);

  return py_cmd_result;
}

PyObject *axis_py_cmd_result_create(PyTypeObject *type, PyObject *args,
                                   PyObject *kw) {
  axis_py_cmd_result_t *py_cmd_result = axis_py_cmd_result_create_internal(type);
  return (PyObject *)axis_py_cmd_result_init(py_cmd_result, args, kw);
}

axis_py_cmd_result_t *axis_py_cmd_result_wrap(axis_shared_ptr_t *cmd) {
  axis_ASSERT(cmd && axis_msg_check_integrity(cmd), "Invalid argument.");

  axis_py_cmd_result_t *py_cmd_result = axis_py_cmd_result_create_internal(NULL);
  py_cmd_result->msg.c_msg = axis_shared_ptr_clone(cmd);
  return py_cmd_result;
}

void axis_py_cmd_result_invalidate(axis_py_cmd_result_t *self) {
  axis_ASSERT(self, "Invalid argument");
  Py_DECREF(self);
}

PyObject *axis_py_cmd_result_get_status_code(PyObject *self, PyObject *args) {
  axis_py_cmd_result_t *py_cmd_result = (axis_py_cmd_result_t *)self;

  axis_ASSERT(py_cmd_result &&
                 axis_py_msg_check_integrity((axis_py_msg_t *)py_cmd_result),
             "Invalid argument.");

  axis_ASSERT(py_cmd_result->msg.c_msg &&
                 axis_msg_check_integrity(py_cmd_result->msg.c_msg),
             "Invalid argument.");

  axis_ASSERT(
      axis_msg_get_type(py_cmd_result->msg.c_msg) == axis_MSG_TYPE_CMD_RESULT,
      "Invalid argument.");

  axis_STATUS_CODE status_code =
      axis_cmd_result_get_status_code(py_cmd_result->msg.c_msg);

  return PyLong_FromLong(status_code);
}

PyObject *axis_py_cmd_result_set_status_code(PyObject *self, PyObject *args) {
  axis_py_cmd_result_t *py_cmd_result = (axis_py_cmd_result_t *)self;

  axis_ASSERT(py_cmd_result &&
                 axis_py_msg_check_integrity((axis_py_msg_t *)py_cmd_result),
             "Invalid argument.");

  axis_ASSERT(py_cmd_result->msg.c_msg &&
                 axis_msg_check_integrity(py_cmd_result->msg.c_msg),
             "Invalid argument.");

  axis_ASSERT(
      axis_msg_get_type(py_cmd_result->msg.c_msg) == axis_MSG_TYPE_CMD_RESULT,
      "Invalid argument.");

  int status_code = 0;
  if (!PyArg_ParseTuple(args, "i", &status_code)) {
    return axis_py_raise_py_value_error_exception(
        "Failed to parse arguments when setting status code.");
  }

  axis_cmd_result_set_status_code(py_cmd_result->msg.c_msg,
                                 (axis_STATUS_CODE)status_code);

  Py_RETURN_NONE;
}

PyObject *axis_py_cmd_result_set_final(PyObject *self, PyObject *args) {
  axis_py_cmd_result_t *py_cmd_result = (axis_py_cmd_result_t *)self;

  axis_ASSERT(py_cmd_result &&
                 axis_py_msg_check_integrity((axis_py_msg_t *)py_cmd_result),
             "Invalid argument.");

  int is_final_flag = 1;
  if (!PyArg_ParseTuple(args, "i", &is_final_flag)) {
    return axis_py_raise_py_value_error_exception(
        "Failed to parse arguments when set_final.");
  }

  axis_error_t err;
  axis_error_init(&err);

  bool rc =
      axis_cmd_result_set_final(py_cmd_result->msg.c_msg, is_final_flag, &err);
  if (!rc) {
    axis_error_deinit(&err);
    return axis_py_raise_py_runtime_error_exception("Failed to set_final.");
  }

  bool err_occurred = axis_py_check_and_clear_py_error();
  axis_ASSERT(!err_occurred, "Should not happen.");

  axis_error_deinit(&err);

  if (rc) {
    Py_RETURN_NONE;
  } else {
    return NULL;
  }
}

PyObject *axis_py_cmd_result_is_final(PyObject *self, PyObject *args) {
  axis_py_cmd_result_t *py_cmd_result = (axis_py_cmd_result_t *)self;

  axis_ASSERT(py_cmd_result &&
                 axis_py_msg_check_integrity((axis_py_msg_t *)py_cmd_result),
             "Invalid argument.");

  axis_error_t err;
  axis_error_init(&err);

  bool is_final = axis_cmd_result_is_final(py_cmd_result->msg.c_msg, &err);

  if (!axis_error_is_success(&err)) {
    axis_error_deinit(&err);
    return axis_py_raise_py_runtime_error_exception("Failed to is_final.");
  }

  axis_error_deinit(&err);

  return PyBool_FromLong(is_final);
}

PyObject *axis_py_cmd_result_is_completed(PyObject *self, PyObject *args) {
  axis_py_cmd_result_t *py_cmd_result = (axis_py_cmd_result_t *)self;

  axis_ASSERT(py_cmd_result &&
                 axis_py_msg_check_integrity((axis_py_msg_t *)py_cmd_result),
             "Invalid argument.");

  axis_error_t err;
  axis_error_init(&err);

  bool is_completed =
      axis_cmd_result_is_completed(py_cmd_result->msg.c_msg, &err);

  if (!axis_error_is_success(&err)) {
    axis_error_deinit(&err);
    return axis_py_raise_py_runtime_error_exception("Failed to is_completed.");
  }

  axis_error_deinit(&err);

  return PyBool_FromLong(is_completed);
}

bool axis_py_cmd_result_init_for_module(PyObject *module) {
  PyTypeObject *py_type = axis_py_cmd_result_py_type();
  if (PyType_Ready(py_type) < 0) {
    axis_py_raise_py_system_error_exception(
        "Python cmd_result class is not ready.");

    axis_ASSERT(0, "Should not happen.");
    return false;
  }

  if (PyModule_AddObjectRef(module, "_CmdResult", (PyObject *)py_type) < 0) {
    axis_py_raise_py_import_error_exception(
        "Failed to add Python type to module.");

    axis_ASSERT(0, "Should not happen.");
    return false;
  }
  return true;
}
