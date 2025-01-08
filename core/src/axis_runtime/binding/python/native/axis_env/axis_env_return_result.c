//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/binding/python/common/common.h"
#include "include_internal/axis_runtime/binding/python/common/error.h"
#include "include_internal/axis_runtime/binding/python/msg/cmd.h"
#include "include_internal/axis_runtime/binding/python/msg/cmd_result.h"
#include "include_internal/axis_runtime/binding/python/msg/msg.h"
#include "include_internal/axis_runtime/binding/python/axis_env/axis_env.h"
#include "axis_runtime/msg/cmd_result/cmd_result.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/macro/memory.h"

typedef struct axis_env_notify_return_result_ctx_t {
  axis_shared_ptr_t *c_cmd;
  axis_shared_ptr_t *c_target_cmd;
  PyObject *py_cb_func;
} axis_env_notify_return_result_ctx_t;

static axis_env_notify_return_result_ctx_t *
axis_env_notify_return_result_ctx_create(axis_shared_ptr_t *c_cmd,
                                        axis_shared_ptr_t *c_target_cmd,
                                        PyObject *py_cb_func) {
  axis_ASSERT(c_cmd, "Invalid argument.");

  axis_env_notify_return_result_ctx_t *ctx =
      axis_MALLOC(sizeof(axis_env_notify_return_result_ctx_t));
  axis_ASSERT(ctx, "Failed to allocate memory.");

  ctx->c_cmd = c_cmd;
  ctx->c_target_cmd = c_target_cmd;
  ctx->py_cb_func = py_cb_func;

  if (py_cb_func) {
    Py_INCREF(py_cb_func);
  }

  return ctx;
}

static void axis_env_notify_return_result_ctx_destroy(
    axis_env_notify_return_result_ctx_t *ctx) {
  axis_ASSERT(ctx, "Invalid argument.");

  if (ctx->c_cmd) {
    axis_shared_ptr_destroy(ctx->c_cmd);
    ctx->c_cmd = NULL;
  }

  if (ctx->c_target_cmd) {
    axis_shared_ptr_destroy(ctx->c_target_cmd);
    ctx->c_target_cmd = NULL;
  }

  ctx->py_cb_func = NULL;

  axis_FREE(ctx);
}

static void proxy_return_result_callback(axis_env_t *axis_env,
                                         void *callback_info,
                                         axis_error_t *err) {
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
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

  if (err) {
    py_error = axis_py_error_wrap(err);
    arglist = Py_BuildValue("(OO)", py_axis_env->actual_py_axis_env, py_error);
  } else {
    arglist = Py_BuildValue("(OO)", py_axis_env->actual_py_axis_env, Py_None);
  }

  PyObject *result = PyObject_CallObject(cb_func, arglist);
  Py_XDECREF(result);  // Ensure cleanup if an error occurred.

  bool err_occurred = axis_py_check_and_clear_py_error();
  axis_ASSERT(!err_occurred, "Should not happen.");

  Py_XDECREF(arglist);
  Py_XDECREF(cb_func);

  if (py_error) {
    axis_py_error_invalidate(py_error);
  }

  axis_py_gil_state_release_internal(prev_state);
}

static void axis_env_proxy_notify_return_result(axis_env_t *axis_env,
                                               void *user_data) {
  axis_ASSERT(user_data, "Invalid argument.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_env_notify_return_result_ctx_t *ctx = user_data;
  axis_ASSERT(ctx, "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  bool rc = false;
  if (ctx->py_cb_func == NULL) {
    if (ctx->c_target_cmd) {
      rc = axis_env_return_result(axis_env, ctx->c_cmd, ctx->c_target_cmd, NULL,
                                 NULL, &err);
    } else {
      rc =
          axis_env_return_result_directly(axis_env, ctx->c_cmd, NULL, NULL, &err);
    }
  } else {
    if (ctx->c_target_cmd) {
      rc = axis_env_return_result(axis_env, ctx->c_cmd, ctx->c_target_cmd,
                                 proxy_return_result_callback, ctx->py_cb_func,
                                 &err);
    } else {
      rc = axis_env_return_result_directly(axis_env, ctx->c_cmd,
                                          proxy_return_result_callback,
                                          ctx->py_cb_func, &err);
    }

    if (!rc) {
      // About to call the Python function, so it's necessary to ensure that the
      // GIL has been acquired.
      //
      // Allows C codes to work safely with Python objects.
      PyGILState_STATE prev_state = axis_py_gil_state_ensure_internal();

      axis_py_error_t *py_err = axis_py_error_wrap(&err);

      axis_py_axis_env_t *py_axis_env = axis_py_axis_env_wrap(axis_env);

      PyObject *arglist =
          Py_BuildValue("(OO)", py_axis_env->actual_py_axis_env, py_err);

      PyObject *result = PyObject_CallObject(ctx->py_cb_func, arglist);
      Py_XDECREF(result);  // Ensure cleanup if an error occurred.

      bool err_occurred = axis_py_check_and_clear_py_error();
      axis_ASSERT(!err_occurred, "Should not happen.");

      Py_XDECREF(arglist);

      if (py_err) {
        axis_py_error_invalidate(py_err);
      }

      axis_py_gil_state_release_internal(prev_state);
    }
  }

  axis_error_deinit(&err);

  axis_env_notify_return_result_ctx_destroy(ctx);
}

PyObject *axis_py_axis_env_return_result(PyObject *self, PyObject *args) {
  axis_py_axis_env_t *py_axis_env = (axis_py_axis_env_t *)self;
  axis_ASSERT(py_axis_env && axis_py_axis_env_check_integrity(py_axis_env),
             "Invalid argument.");

  bool success = true;

  axis_error_t err;
  axis_error_init(&err);

  axis_py_cmd_t *py_target_cmd = NULL;
  axis_py_cmd_result_t *py_cmd_result = NULL;
  PyObject *cb_func = NULL;

  if (!PyArg_ParseTuple(args, "O!O!O", axis_py_cmd_result_py_type(),
                        &py_cmd_result, axis_py_cmd_py_type(), &py_target_cmd,
                        &cb_func)) {
    success = false;
    axis_py_raise_py_type_error_exception(
        "Invalid argument type when return result.");
    goto done;
  }

  // Check if cb_func is callable.
  if (!PyCallable_Check(cb_func)) {
    cb_func = NULL;
  }

  axis_shared_ptr_t *c_target_cmd =
      axis_shared_ptr_clone(py_target_cmd->msg.c_msg);
  axis_shared_ptr_t *c_result_cmd =
      axis_shared_ptr_clone(py_cmd_result->msg.c_msg);

  axis_env_notify_return_result_ctx_t *notify_info =
      axis_env_notify_return_result_ctx_create(c_result_cmd, c_target_cmd,
                                              cb_func);

  bool rc = axis_env_proxy_notify(py_axis_env->c_axis_env_proxy,
                                 axis_env_proxy_notify_return_result,
                                 notify_info, false, &err);
  if (!rc) {
    axis_env_notify_return_result_ctx_destroy(notify_info);
    success = false;
    axis_py_raise_py_runtime_error_exception("Failed to return result.");
    goto done;
  } else {
    if (axis_cmd_result_is_final(py_cmd_result->msg.c_msg, &err)) {
      // Remove the C message from the python target message if it is the final
      // cmd result.
      axis_py_msg_destroy_c_msg(&py_target_cmd->msg);
    }

    // Destroy the C message from the Python message as the ownership has been
    // transferred to the notify_info.
    axis_py_msg_destroy_c_msg(&py_cmd_result->msg);
  }

done:
  axis_error_deinit(&err);

  if (success) {
    Py_RETURN_NONE;
  } else {
    return NULL;
  }
}

PyObject *axis_py_axis_env_return_result_directly(PyObject *self,
                                                PyObject *args) {
  axis_py_axis_env_t *py_axis_env = (axis_py_axis_env_t *)self;
  axis_ASSERT(py_axis_env && axis_py_axis_env_check_integrity(py_axis_env),
             "Invalid argument.");

  bool success = true;

  axis_error_t err;
  axis_error_init(&err);

  axis_py_cmd_result_t *py_cmd_result = NULL;
  PyObject *cb_func = NULL;

  if (!PyArg_ParseTuple(args, "O!O", axis_py_cmd_result_py_type(),
                        &py_cmd_result, &cb_func)) {
    success = false;
    axis_py_raise_py_type_error_exception(
        "Invalid argument type when return result directly.");
    goto done;
  }

  // Check if cb_func is callable.
  if (!PyCallable_Check(cb_func)) {
    cb_func = NULL;
  }

  axis_shared_ptr_t *c_result_cmd =
      axis_shared_ptr_clone(py_cmd_result->msg.c_msg);

  axis_env_notify_return_result_ctx_t *notify_info =
      axis_env_notify_return_result_ctx_create(c_result_cmd, NULL, cb_func);

  if (!axis_env_proxy_notify(py_axis_env->c_axis_env_proxy,
                            axis_env_proxy_notify_return_result, notify_info,
                            false, &err)) {
    axis_env_notify_return_result_ctx_destroy(notify_info);
    success = false;
    axis_py_raise_py_runtime_error_exception(
        "Failed to return result directly.");
    goto done;
  } else {
    // Destroy the C message from the Python message as the ownership has been
    // transferred to the notify_info.
    axis_py_msg_destroy_c_msg(&py_cmd_result->msg);
  }

done:
  axis_error_deinit(&err);

  if (success) {
    Py_RETURN_NONE;
  } else {
    return NULL;
  }
}
