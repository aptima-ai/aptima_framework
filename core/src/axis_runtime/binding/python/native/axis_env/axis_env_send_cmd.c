//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/binding/python/common/common.h"
#include "include_internal/axis_runtime/binding/python/common/error.h"
#include "include_internal/axis_runtime/binding/python/msg/cmd.h"
#include "include_internal/axis_runtime/binding/python/msg/cmd_result.h"
#include "include_internal/axis_runtime/binding/python/msg/msg.h"
#include "include_internal/axis_runtime/binding/python/axis_env/axis_env.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd_base.h"
#include "axis_runtime/extension/extension.h"
#include "axis_runtime/msg/cmd_result/cmd_result.h"
#include "axis_runtime/axis_env_proxy/axis_env_proxy.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/memory.h"

typedef struct axis_env_notify_send_cmd_ctx_t {
  axis_shared_ptr_t *c_cmd;
  PyObject *py_cb_func;
  bool is_ex;
} axis_env_notify_send_cmd_ctx_t;

static axis_env_notify_send_cmd_ctx_t *axis_env_notify_send_cmd_ctx_create(
    axis_shared_ptr_t *c_cmd, PyObject *py_cb_func, bool is_ex) {
  axis_ASSERT(c_cmd, "Invalid argument.");

  axis_env_notify_send_cmd_ctx_t *ctx =
      axis_MALLOC(sizeof(axis_env_notify_send_cmd_ctx_t));
  axis_ASSERT(ctx, "Failed to allocate memory.");

  ctx->c_cmd = c_cmd;
  ctx->py_cb_func = py_cb_func;
  ctx->is_ex = is_ex;

  if (py_cb_func != NULL) {
    Py_INCREF(py_cb_func);
  }

  return ctx;
}

static void axis_env_notify_send_cmd_ctx_destroy(
    axis_env_notify_send_cmd_ctx_t *ctx) {
  axis_ASSERT(ctx, "Invalid argument.");

  if (ctx->c_cmd) {
    axis_shared_ptr_destroy(ctx->c_cmd);
    ctx->c_cmd = NULL;
  }

  ctx->py_cb_func = NULL;

  axis_FREE(ctx);
}

static void proxy_send_xxx_callback(axis_env_t *axis_env,
                                    axis_shared_ptr_t *cmd_result,
                                    void *callback_info, axis_error_t *err) {
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");
  axis_ASSERT(cmd_result && axis_cmd_base_check_integrity(cmd_result),
             "Should not happen.");
  axis_ASSERT(callback_info, "Should not happen.");

  // About to call the Python function, so it's necessary to ensure that the GIL
  // has been acquired.
  //
  // Allows C codes to work safely with Python objects.
  PyGILState_STATE prev_state = axis_py_gil_state_ensure_internal();

  axis_py_axis_env_t *py_axis_env = axis_py_axis_env_wrap(axis_env);
  PyObject *cb_func = callback_info;

  PyObject *arglist = NULL;
  axis_py_error_t *py_error = NULL;
  axis_py_cmd_result_t *cmd_result_bridge = NULL;

  if (err) {
    py_error = axis_py_error_wrap(err);

    arglist = Py_BuildValue("(OOO)", py_axis_env->actual_py_axis_env, Py_None,
                            py_error);
  } else {
    cmd_result_bridge = axis_py_cmd_result_wrap(cmd_result);

    arglist = Py_BuildValue("(OOO)", py_axis_env->actual_py_axis_env,
                            cmd_result_bridge, Py_None);
  }

  PyObject *result = PyObject_CallObject(cb_func, arglist);
  Py_XDECREF(result);  // Ensure cleanup if an error occurred.

  bool err_occurred = axis_py_check_and_clear_py_error();
  axis_ASSERT(!err_occurred, "Should not happen.");

  Py_XDECREF(arglist);

  bool is_completed = axis_cmd_result_is_completed(cmd_result, NULL);
  if (is_completed) {
    Py_XDECREF(cb_func);
  }

  if (py_error) {
    axis_py_error_invalidate(py_error);
  }

  if (cmd_result_bridge) {
    axis_py_cmd_result_invalidate(cmd_result_bridge);
  }

  axis_py_gil_state_release_internal(prev_state);
}

static void axis_env_proxy_notify_send_cmd(axis_env_t *axis_env, void *user_data) {
  axis_ASSERT(user_data, "Invalid argument.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_env_notify_send_cmd_ctx_t *notify_info = user_data;

  axis_error_t err;
  axis_error_init(&err);

  axis_env_send_cmd_func_t send_cmd_func = NULL;
  if (notify_info->is_ex) {
    send_cmd_func = axis_env_send_cmd_ex;
  } else {
    send_cmd_func = axis_env_send_cmd;
  }

  bool res = false;
  if (notify_info->py_cb_func == NULL) {
    res = send_cmd_func(axis_env, notify_info->c_cmd, NULL, NULL, &err);
  } else {
    res = send_cmd_func(axis_env, notify_info->c_cmd, proxy_send_xxx_callback,
                        notify_info->py_cb_func, &err);
    if (!res) {
      // About to call the Python function, so it's necessary to ensure that the
      // GIL has been acquired.
      //
      // Allows C codes to work safely with Python objects.
      PyGILState_STATE prev_state = axis_py_gil_state_ensure_internal();

      axis_py_axis_env_t *py_axis_env = axis_py_axis_env_wrap(axis_env);
      axis_py_error_t *py_err = axis_py_error_wrap(&err);

      PyObject *arglist = Py_BuildValue("(OOO)", py_axis_env->actual_py_axis_env,
                                        Py_None, py_err);

      PyObject *result = PyObject_CallObject(notify_info->py_cb_func, arglist);
      Py_XDECREF(result);  // Ensure cleanup if an error occurred.

      bool err_occurred = axis_py_check_and_clear_py_error();
      axis_ASSERT(!err_occurred, "Should not happen.");

      Py_XDECREF(arglist);
      Py_XDECREF(notify_info->py_cb_func);

      axis_py_error_invalidate(py_err);

      axis_py_gil_state_release_internal(prev_state);
    }
  }

  axis_error_deinit(&err);

  axis_env_notify_send_cmd_ctx_destroy(notify_info);
}

PyObject *axis_py_axis_env_send_cmd(PyObject *self, PyObject *args) {
  axis_py_axis_env_t *py_axis_env = (axis_py_axis_env_t *)self;
  axis_ASSERT(py_axis_env && axis_py_axis_env_check_integrity(py_axis_env),
             "Invalid argument.");

  if (PyTuple_GET_SIZE(args) != 3) {
    return axis_py_raise_py_value_error_exception(
        "Invalid argument count when axis_env.send_cmd.");
  }

  bool success = true;

  axis_error_t err;
  axis_error_init(&err);

  axis_py_cmd_t *py_cmd = NULL;
  PyObject *cb_func = NULL;
  int is_ex = false;

  if (!PyArg_ParseTuple(args, "O!Op", axis_py_cmd_py_type(), &py_cmd, &cb_func,
                        &is_ex)) {
    success = false;
    axis_py_raise_py_type_error_exception(
        "Invalid argument type when send cmd.");
    goto done;
  }

  // Check if cb_func is callable.
  if (!PyCallable_Check(cb_func)) {
    cb_func = NULL;
  }

  axis_shared_ptr_t *cloned_cmd = axis_shared_ptr_clone(py_cmd->msg.c_msg);
  axis_env_notify_send_cmd_ctx_t *notify_info =
      axis_env_notify_send_cmd_ctx_create(cloned_cmd, cb_func, is_ex);

  if (!axis_env_proxy_notify(py_axis_env->c_axis_env_proxy,
                            axis_env_proxy_notify_send_cmd, notify_info, false,
                            &err)) {
    if (cb_func) {
      Py_XDECREF(cb_func);
    }

    axis_env_notify_send_cmd_ctx_destroy(notify_info);
    success = false;
    axis_py_raise_py_runtime_error_exception("Failed to send cmd.");
    goto done;
  } else {
    // Destroy the C message from the Python message as the ownership has been
    // transferred to the notify_info.
    axis_py_msg_destroy_c_msg(&py_cmd->msg);
  }

done:
  axis_error_deinit(&err);

  if (success) {
    Py_RETURN_NONE;
  } else {
    return NULL;
  }
}
