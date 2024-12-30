//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/binding/python/common/common.h"
#include "include_internal/ten_runtime/binding/python/common/error.h"
#include "include_internal/ten_runtime/binding/python/msg/cmd.h"
#include "include_internal/ten_runtime/binding/python/msg/cmd_result.h"
#include "include_internal/ten_runtime/binding/python/msg/msg.h"
#include "include_internal/ten_runtime/binding/python/test/env_tester.h"
#include "ten_runtime/msg/cmd_result/cmd_result.h"
#include "ten_runtime/test/env_tester.h"
#include "ten_utils/macro/check.h"

static void proxy_send_xxx_callback(ten_env_tester_t *ten_env_tester,
                                    ten_shared_ptr_t *cmd_result,
                                    void *callback_info, ten_error_t *error) {
  TEN_ASSERT(ten_env_tester && ten_env_tester_check_integrity(ten_env_tester),
             "Should not happen.");
  TEN_ASSERT(callback_info, "Should not happen.");

  // About to call the Python function, so it's necessary to ensure that the GIL
  // has been acquired.
  //
  // Allows C codes to work safely with Python objects.
  PyGILState_STATE prev_state = ten_py_gil_state_ensure_internal();

  ten_py_ten_env_tester_t *py_ten_env_tester =
      ten_py_ten_env_tester_wrap(ten_env_tester);
  PyObject *cb_func = callback_info;

  PyObject *arglist = NULL;
  ten_py_cmd_result_t *cmd_result_bridge = NULL;
  ten_py_error_t *py_error = NULL;

  if (cmd_result) {
    cmd_result_bridge = ten_py_cmd_result_wrap(cmd_result);
    arglist =
        Py_BuildValue("(OOO)", py_ten_env_tester->actual_py_ten_env_tester,
                      cmd_result_bridge, Py_None);
  } else {
    TEN_ASSERT(error, "Should not happen.");

    py_error = ten_py_error_wrap(error);
    arglist =
        Py_BuildValue("(OOO)", py_ten_env_tester->actual_py_ten_env_tester,
                      Py_None, py_error);
  }

  PyObject *result = PyObject_CallObject(cb_func, arglist);
  Py_XDECREF(result);  // Ensure cleanup if an error occurred.

  bool err_occurred = ten_py_check_and_clear_py_error();
  TEN_ASSERT(!err_occurred, "Should not happen.");

  Py_XDECREF(arglist);

  bool is_completed = ten_cmd_result_is_completed(cmd_result, NULL);
  if (is_completed) {
    Py_XDECREF(cb_func);
  }

  if (py_error) {
    ten_py_error_invalidate(py_error);
  }

  if (cmd_result_bridge) {
    ten_py_cmd_result_invalidate(cmd_result_bridge);
  }

  ten_py_gil_state_release_internal(prev_state);
}

PyObject *ten_py_ten_env_tester_send_cmd(PyObject *self, PyObject *args) {
  ten_py_ten_env_tester_t *py_ten_env_tester = (ten_py_ten_env_tester_t *)self;
  TEN_ASSERT(py_ten_env_tester &&
                 ten_py_ten_env_tester_check_integrity(py_ten_env_tester),
             "Invalid argument.");

  if (PyTuple_GET_SIZE(args) != 2) {
    return ten_py_raise_py_value_error_exception(
        "Invalid argument count when ten_env_tester.send_cmd.");
  }

  bool success = true;

  ten_error_t err;
  ten_error_init(&err);

  ten_py_cmd_t *py_cmd = NULL;
  PyObject *cb_func = NULL;

  if (!PyArg_ParseTuple(args, "O!O", ten_py_cmd_py_type(), &py_cmd, &cb_func)) {
    success = false;
    ten_py_raise_py_type_error_exception(
        "Invalid argument type when send cmd.");
    goto done;
  }

  // Check if cb_func is callable.
  if (!PyCallable_Check(cb_func)) {
    cb_func = NULL;
  }

  if (cb_func) {
    // Increase the reference count of the callback function to ensure that it
    // will not be destroyed before the callback is called.
    Py_INCREF(cb_func);

    success = ten_env_tester_send_cmd(py_ten_env_tester->c_ten_env_tester,
                                      py_cmd->msg.c_msg,
                                      proxy_send_xxx_callback, cb_func, &err);
  } else {
    success = ten_env_tester_send_cmd(py_ten_env_tester->c_ten_env_tester,
                                      py_cmd->msg.c_msg, NULL, NULL, &err);
  }

  if (!success) {
    if (cb_func) {
      Py_XDECREF(cb_func);
    }

    ten_py_raise_py_runtime_error_exception("Failed to send cmd.");
    goto done;
  } else {
    // Destroy the C message from the Python message as the ownership has been
    // transferred to the notify_info.
    ten_py_msg_destroy_c_msg(&py_cmd->msg);
  }

done:
  ten_error_deinit(&err);

  if (success) {
    Py_RETURN_NONE;
  } else {
    return NULL;
  }
}
