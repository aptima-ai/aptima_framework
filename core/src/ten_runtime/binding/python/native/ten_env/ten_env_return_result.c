//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/binding/python/common/common.h"
#include "include_internal/ten_runtime/binding/python/common/error.h"
#include "include_internal/ten_runtime/binding/python/msg/cmd.h"
#include "include_internal/ten_runtime/binding/python/msg/cmd_result.h"
#include "include_internal/ten_runtime/binding/python/msg/msg.h"
#include "include_internal/ten_runtime/binding/python/ten_env/ten_env.h"
#include "ten_runtime/msg/cmd_result/cmd_result.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/log/log.h"
#include "ten_utils/macro/memory.h"

typedef struct ten_env_notify_return_result_info_t {
  ten_shared_ptr_t *c_cmd;
  ten_shared_ptr_t *c_target_cmd;
  PyObject *py_cb_func;
} ten_env_notify_return_result_info_t;

static ten_env_notify_return_result_info_t *
ten_env_notify_return_result_info_create(ten_shared_ptr_t *c_cmd,
                                         ten_shared_ptr_t *c_target_cmd,
                                         PyObject *py_cb_func) {
  TEN_ASSERT(c_cmd, "Invalid argument.");

  ten_env_notify_return_result_info_t *info =
      TEN_MALLOC(sizeof(ten_env_notify_return_result_info_t));
  TEN_ASSERT(info, "Failed to allocate memory.");

  info->c_cmd = c_cmd;
  info->c_target_cmd = c_target_cmd;
  info->py_cb_func = py_cb_func;

  if (py_cb_func) {
    Py_INCREF(py_cb_func);
  }

  return info;
}

static void ten_env_notify_return_result_info_destroy(
    ten_env_notify_return_result_info_t *info) {
  TEN_ASSERT(info, "Invalid argument.");

  if (info->c_cmd) {
    ten_shared_ptr_destroy(info->c_cmd);
    info->c_cmd = NULL;
  }

  if (info->c_target_cmd) {
    ten_shared_ptr_destroy(info->c_target_cmd);
    info->c_target_cmd = NULL;
  }

  info->py_cb_func = NULL;

  TEN_FREE(info);
}

static void ten_env_proxy_notify_return_result(ten_env_t *ten_env,
                                               void *user_data) {
  TEN_ASSERT(user_data, "Invalid argument.");
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_env_notify_return_result_info_t *info = user_data;
  TEN_ASSERT(info, "Should not happen.");

  ten_error_t err;
  ten_error_init(&err);

  bool rc = false;
  if (info->py_cb_func == NULL) {
    if (info->c_target_cmd) {
      rc = ten_env_return_result(ten_env, info->c_cmd, info->c_target_cmd, NULL,
                                 NULL, &err);
    } else {
      rc = ten_env_return_result_directly(ten_env, info->c_cmd, NULL, NULL,
                                          &err);
    }

    if (!rc) {
      TEN_LOGE(
          "Failed to return result, but no callback function is provided. "
          "errno: %s, err_msg: %s",
          ten_error_errno(&err), ten_error_errmsg(&err));
    }
  } else {
    // TODO(xilin) : Transform the return_xxx C function into an async API and
    // set the callback here. Wait for the PR 357 to be merged.
    if (info->c_target_cmd) {
      rc = ten_env_return_result(ten_env, info->c_cmd, info->c_target_cmd, NULL,
                                 NULL, &err);
    } else {
      rc = ten_env_return_result_directly(ten_env, info->c_cmd, NULL, NULL,
                                          &err);
    }

    ten_py_error_t *py_err = NULL;

    // About to call the Python function, so it's necessary to ensure that the
    // GIL has been acquired.
    //
    // Allows C codes to work safely with Python objects.
    PyGILState_STATE prev_state = ten_py_gil_state_ensure();

    if (!rc) {
      py_err = ten_py_error_wrap(&err);
    }
    ten_py_ten_env_t *py_ten_env = ten_py_ten_env_wrap(ten_env);

    PyObject *arglist = Py_BuildValue("(OO)", py_ten_env->actual_py_ten_env,
                                      py_err ? (PyObject *)py_err : Py_None);

    PyObject *result = PyObject_CallObject(info->py_cb_func, arglist);
    Py_XDECREF(result);  // Ensure cleanup if an error occurred.

    bool err_occurred = ten_py_check_and_clear_py_error();
    TEN_ASSERT(!err_occurred, "Should not happen.");

    Py_XDECREF(arglist);

    if (py_err) {
      ten_py_error_invalidate(py_err);
    }

    ten_py_gil_state_release(prev_state);
  }

  ten_error_deinit(&err);

  ten_env_notify_return_result_info_destroy(info);
}

PyObject *ten_py_ten_env_return_result(PyObject *self, PyObject *args) {
  ten_py_ten_env_t *py_ten_env = (ten_py_ten_env_t *)self;
  TEN_ASSERT(py_ten_env && ten_py_ten_env_check_integrity(py_ten_env),
             "Invalid argument.");

  bool success = true;

  ten_error_t err;
  ten_error_init(&err);

  ten_py_cmd_t *py_target_cmd = NULL;
  ten_py_cmd_result_t *py_cmd_result = NULL;
  PyObject *cb_func = NULL;

  if (!PyArg_ParseTuple(args, "O!O!O", ten_py_cmd_result_py_type(),
                        &py_cmd_result, ten_py_cmd_py_type(), &py_target_cmd,
                        &cb_func)) {
    success = false;
    ten_py_raise_py_type_error_exception(
        "Invalid argument type when return result.");
    goto done;
  }

  // Check if cb_func is callable.
  if (!PyCallable_Check(cb_func)) {
    cb_func = NULL;
  }

  ten_shared_ptr_t *c_target_cmd =
      ten_shared_ptr_clone(py_target_cmd->msg.c_msg);
  ten_shared_ptr_t *c_result_cmd =
      ten_shared_ptr_clone(py_cmd_result->msg.c_msg);

  ten_env_notify_return_result_info_t *notify_info =
      ten_env_notify_return_result_info_create(c_result_cmd, c_target_cmd,
                                               cb_func);

  bool rc = ten_env_proxy_notify(py_ten_env->c_ten_env_proxy,
                                 ten_env_proxy_notify_return_result,
                                 notify_info, false, &err);
  if (!rc) {
    ten_env_notify_return_result_info_destroy(notify_info);
    success = false;
    ten_py_raise_py_runtime_error_exception("Failed to return result.");
    goto done;
  } else {
    if (ten_cmd_result_is_final(py_cmd_result->msg.c_msg, &err)) {
      // Remove the C message from the python target message if it is the final
      // cmd result.
      ten_py_msg_destroy_c_msg(&py_target_cmd->msg);
    }

    // Destroy the C message from the Python message as the ownership has been
    // transferred to the notify_info.
    ten_py_msg_destroy_c_msg(&py_cmd_result->msg);
  }

done:
  ten_error_deinit(&err);

  if (success) {
    Py_RETURN_NONE;
  } else {
    return NULL;
  }
}

PyObject *ten_py_ten_env_return_result_directly(PyObject *self,
                                                PyObject *args) {
  ten_py_ten_env_t *py_ten_env = (ten_py_ten_env_t *)self;
  TEN_ASSERT(py_ten_env && ten_py_ten_env_check_integrity(py_ten_env),
             "Invalid argument.");

  bool success = true;

  ten_error_t err;
  ten_error_init(&err);

  ten_py_cmd_result_t *py_cmd_result = NULL;
  PyObject *cb_func = NULL;

  if (!PyArg_ParseTuple(args, "O!O", ten_py_cmd_result_py_type(),
                        &py_cmd_result, &cb_func)) {
    success = false;
    ten_py_raise_py_type_error_exception(
        "Invalid argument type when return result directly.");
    goto done;
  }

  // Check if cb_func is callable.
  if (!PyCallable_Check(cb_func)) {
    cb_func = NULL;
  }

  ten_shared_ptr_t *c_result_cmd =
      ten_shared_ptr_clone(py_cmd_result->msg.c_msg);

  ten_env_notify_return_result_info_t *notify_info =
      ten_env_notify_return_result_info_create(c_result_cmd, NULL, cb_func);

  if (!ten_env_proxy_notify(py_ten_env->c_ten_env_proxy,
                            ten_env_proxy_notify_return_result, notify_info,
                            false, &err)) {
    ten_env_notify_return_result_info_destroy(notify_info);
    success = false;
    ten_py_raise_py_runtime_error_exception(
        "Failed to return result directly.");
    goto done;
  } else {
    // Destroy the C message from the Python message as the ownership has been
    // transferred to the notify_info.
    ten_py_msg_destroy_c_msg(&py_cmd_result->msg);
  }

done:
  ten_error_deinit(&err);

  if (success) {
    Py_RETURN_NONE;
  } else {
    return NULL;
  }
}
