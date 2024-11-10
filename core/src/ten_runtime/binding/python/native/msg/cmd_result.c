//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/binding/python/msg/cmd_result.h"

#include "include_internal/ten_runtime/binding/python/common/error.h"
#include "include_internal/ten_runtime/binding/python/msg/msg.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd_result/cmd.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "ten_runtime/common/status_code.h"
#include "ten_runtime/msg/cmd_result/cmd_result.h"

static ten_py_cmd_result_t *ten_py_cmd_result_create_internal(
    PyTypeObject *py_type) {
  if (!py_type) {
    py_type = ten_py_cmd_result_py_type();
  }

  ten_py_cmd_result_t *py_cmd_result =
      (ten_py_cmd_result_t *)py_type->tp_alloc(py_type, 0);

  ten_signature_set(&py_cmd_result->msg.signature, TEN_PY_MSG_SIGNATURE);
  py_cmd_result->msg.c_msg = NULL;

  return py_cmd_result;
}

void ten_py_cmd_result_destroy(PyObject *self) {
  ten_py_cmd_result_t *py_cmd_result = (ten_py_cmd_result_t *)self;

  TEN_ASSERT(py_cmd_result &&
                 ten_py_msg_check_integrity((ten_py_msg_t *)py_cmd_result),
             "Invalid argument.");

  ten_py_msg_destroy_c_msg(&py_cmd_result->msg);
  Py_TYPE(self)->tp_free(self);
}

static ten_py_cmd_result_t *ten_py_cmd_result_init(
    ten_py_cmd_result_t *py_cmd_result, TEN_UNUSED PyObject *args,
    TEN_UNUSED PyObject *kw) {
  TEN_ASSERT(py_cmd_result &&
                 ten_py_msg_check_integrity((ten_py_msg_t *)py_cmd_result),
             "Invalid argument.");

  py_cmd_result->msg.c_msg =
      ten_msg_create_from_msg_type(TEN_MSG_TYPE_CMD_RESULT);

  return py_cmd_result;
}

PyObject *ten_py_cmd_result_create(PyTypeObject *type, PyObject *args,
                                   PyObject *kw) {
  ten_py_cmd_result_t *py_cmd_result = ten_py_cmd_result_create_internal(type);
  return (PyObject *)ten_py_cmd_result_init(py_cmd_result, args, kw);
}

ten_py_cmd_result_t *ten_py_cmd_result_wrap(ten_shared_ptr_t *cmd) {
  TEN_ASSERT(cmd && ten_msg_check_integrity(cmd), "Invalid argument.");

  ten_py_cmd_result_t *py_cmd_result = ten_py_cmd_result_create_internal(NULL);
  py_cmd_result->msg.c_msg = ten_shared_ptr_clone(cmd);
  return py_cmd_result;
}

void ten_py_cmd_result_invalidate(ten_py_cmd_result_t *self) {
  TEN_ASSERT(self, "Invalid argument");
  Py_DECREF(self);
}

PyObject *ten_py_cmd_result_get_status_code(PyObject *self, PyObject *args) {
  ten_py_cmd_result_t *py_cmd_result = (ten_py_cmd_result_t *)self;

  TEN_ASSERT(py_cmd_result &&
                 ten_py_msg_check_integrity((ten_py_msg_t *)py_cmd_result),
             "Invalid argument.");

  TEN_ASSERT(py_cmd_result->msg.c_msg &&
                 ten_msg_check_integrity(py_cmd_result->msg.c_msg),
             "Invalid argument.");

  TEN_ASSERT(
      ten_msg_get_type(py_cmd_result->msg.c_msg) == TEN_MSG_TYPE_CMD_RESULT,
      "Invalid argument.");

  TEN_STATUS_CODE status_code =
      ten_cmd_result_get_status_code(py_cmd_result->msg.c_msg);

  return PyLong_FromLong(status_code);
}

PyObject *ten_py_cmd_result_set_status_code(PyObject *self, PyObject *args) {
  ten_py_cmd_result_t *py_cmd_result = (ten_py_cmd_result_t *)self;

  TEN_ASSERT(py_cmd_result &&
                 ten_py_msg_check_integrity((ten_py_msg_t *)py_cmd_result),
             "Invalid argument.");

  TEN_ASSERT(py_cmd_result->msg.c_msg &&
                 ten_msg_check_integrity(py_cmd_result->msg.c_msg),
             "Invalid argument.");

  TEN_ASSERT(
      ten_msg_get_type(py_cmd_result->msg.c_msg) == TEN_MSG_TYPE_CMD_RESULT,
      "Invalid argument.");

  int status_code = 0;
  if (!PyArg_ParseTuple(args, "i", &status_code)) {
    return ten_py_raise_py_value_error_exception(
        "Failed to parse arguments when setting status code.");
  }

  ten_cmd_result_set_status_code(py_cmd_result->msg.c_msg,
                                 (TEN_STATUS_CODE)status_code);

  Py_RETURN_NONE;
}

PyObject *ten_py_cmd_result_set_final(PyObject *self, PyObject *args) {
  ten_py_cmd_result_t *py_cmd_result = (ten_py_cmd_result_t *)self;

  TEN_ASSERT(py_cmd_result &&
                 ten_py_msg_check_integrity((ten_py_msg_t *)py_cmd_result),
             "Invalid argument.");

  int is_final_flag = 1;
  if (!PyArg_ParseTuple(args, "i", &is_final_flag)) {
    return ten_py_raise_py_value_error_exception(
        "Failed to parse arguments when set_final.");
  }

  ten_error_t err;
  ten_error_init(&err);

  bool rc =
      ten_cmd_result_set_final(py_cmd_result->msg.c_msg, is_final_flag, &err);
  if (!rc) {
    ten_error_deinit(&err);
    return ten_py_raise_py_runtime_error_exception("Failed to set_final.");
  }

  bool err_occurred = ten_py_check_and_clear_py_error();
  TEN_ASSERT(!err_occurred, "Should not happen.");

  ten_error_deinit(&err);

  if (rc) {
    Py_RETURN_NONE;
  } else {
    return NULL;
  }
}

PyObject *ten_py_cmd_result_is_final(PyObject *self, PyObject *args) {
  ten_py_cmd_result_t *py_cmd_result = (ten_py_cmd_result_t *)self;

  TEN_ASSERT(py_cmd_result &&
                 ten_py_msg_check_integrity((ten_py_msg_t *)py_cmd_result),
             "Invalid argument.");

  ten_error_t err;
  ten_error_init(&err);

  bool is_final = ten_cmd_result_is_final(py_cmd_result->msg.c_msg, &err);

  if (!ten_error_is_success(&err)) {
    ten_error_deinit(&err);
    return ten_py_raise_py_runtime_error_exception("Failed to is_final.");
  }

  ten_error_deinit(&err);

  return PyBool_FromLong(is_final);
}

PyObject *ten_py_cmd_result_is_completed(PyObject *self, PyObject *args) {
  ten_py_cmd_result_t *py_cmd_result = (ten_py_cmd_result_t *)self;

  TEN_ASSERT(py_cmd_result &&
                 ten_py_msg_check_integrity((ten_py_msg_t *)py_cmd_result),
             "Invalid argument.");

  ten_error_t err;
  ten_error_init(&err);

  bool is_completed =
      ten_cmd_result_is_completed(py_cmd_result->msg.c_msg, &err);

  if (!ten_error_is_success(&err)) {
    ten_error_deinit(&err);
    return ten_py_raise_py_runtime_error_exception("Failed to is_completed.");
  }

  ten_error_deinit(&err);

  return PyBool_FromLong(is_completed);
}

bool ten_py_cmd_result_init_for_module(PyObject *module) {
  PyTypeObject *py_type = ten_py_cmd_result_py_type();
  if (PyType_Ready(py_type) < 0) {
    ten_py_raise_py_system_error_exception(
        "Python cmd_result class is not ready.");

    TEN_ASSERT(0, "Should not happen.");
    return false;
  }

  if (PyModule_AddObjectRef(module, "_CmdResult", (PyObject *)py_type) < 0) {
    ten_py_raise_py_import_error_exception(
        "Failed to add Python type to module.");

    TEN_ASSERT(0, "Should not happen.");
    return false;
  }
  return true;
}
