//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/binding/python/common/common.h"
#include "include_internal/axis_runtime/binding/python/common/error.h"
#include "include_internal/axis_runtime/binding/python/msg/data.h"
#include "include_internal/axis_runtime/binding/python/msg/msg.h"
#include "include_internal/axis_runtime/binding/python/axis_env/axis_env.h"
#include "axis_utils/macro/mark.h"
#include "axis_utils/macro/memory.h"

typedef struct axis_env_notify_send_data_ctx_t {
  axis_shared_ptr_t *c_data;
  PyObject *py_cb_func;
} axis_env_notify_send_data_ctx_t;

static axis_env_notify_send_data_ctx_t *axis_env_notify_send_data_ctx_create(
    axis_shared_ptr_t *c_data, PyObject *py_cb_func) {
  axis_ASSERT(c_data, "Invalid argument.");

  axis_env_notify_send_data_ctx_t *ctx =
      axis_MALLOC(sizeof(axis_env_notify_send_data_ctx_t));
  axis_ASSERT(ctx, "Failed to allocate memory.");

  ctx->c_data = c_data;
  ctx->py_cb_func = py_cb_func;

  if (py_cb_func != NULL) {
    Py_INCREF(py_cb_func);
  }

  return ctx;
}

static void axis_env_notify_send_data_ctx_destroy(
    axis_env_notify_send_data_ctx_t *ctx) {
  axis_ASSERT(ctx, "Invalid argument.");

  if (ctx->c_data) {
    axis_shared_ptr_destroy(ctx->c_data);
    ctx->c_data = NULL;
  }

  ctx->py_cb_func = NULL;

  axis_FREE(ctx);
}

static void proxy_send_data_callback(axis_env_t *axis_env,
                                     axis_UNUSED axis_shared_ptr_t *cmd_result,
                                     void *callback_info, axis_error_t *err) {
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

static void axis_env_proxy_notify_send_data(axis_env_t *axis_env,
                                           void *user_data) {
  axis_ASSERT(user_data, "Invalid argument.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_env_notify_send_data_ctx_t *notify_info = user_data;
  axis_ASSERT(notify_info, "Invalid argument.");

  axis_error_t err;
  axis_error_init(&err);

  bool res = false;
  if (notify_info->py_cb_func == NULL) {
    res = axis_env_send_data(axis_env, notify_info->c_data, NULL, NULL, &err);
  } else {
    res = axis_env_send_data(axis_env, notify_info->c_data,
                            proxy_send_data_callback, notify_info->py_cb_func,
                            &err);
    if (!res) {
      // About to call the Python function, so it's necessary to ensure that the
      // GIL
      // has been acquired.
      //
      // Allows C codes to work safely with Python objects.
      PyGILState_STATE prev_state = axis_py_gil_state_ensure_internal();

      axis_py_axis_env_t *py_axis_env = axis_py_axis_env_wrap(axis_env);
      axis_py_error_t *py_err = axis_py_error_wrap(&err);

      PyObject *arglist =
          Py_BuildValue("(OO)", py_axis_env->actual_py_axis_env, py_err);

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

  axis_env_notify_send_data_ctx_destroy(notify_info);
}

PyObject *axis_py_axis_env_send_data(PyObject *self, PyObject *args) {
  axis_py_axis_env_t *py_axis_env = (axis_py_axis_env_t *)self;
  axis_ASSERT(py_axis_env && axis_py_axis_env_check_integrity(py_axis_env),
             "Invalid argument.");

  bool success = true;

  axis_error_t err;
  axis_error_init(&err);

  axis_py_data_t *py_data = NULL;
  PyObject *cb_func = NULL;

  if (!PyArg_ParseTuple(args, "O!O", axis_py_data_py_type(), &py_data,
                        &cb_func)) {
    success = false;
    axis_py_raise_py_type_error_exception(
        "Invalid argument type when send data.");
    goto done;
  }

  // Check if cb_func is callable.
  if (!PyCallable_Check(cb_func)) {
    cb_func = NULL;
  }

  axis_shared_ptr_t *cloned_data = axis_shared_ptr_clone(py_data->msg.c_msg);
  axis_env_notify_send_data_ctx_t *notify_info =
      axis_env_notify_send_data_ctx_create(cloned_data, cb_func);

  if (!axis_env_proxy_notify(py_axis_env->c_axis_env_proxy,
                            axis_env_proxy_notify_send_data, notify_info, false,
                            &err)) {
    if (cb_func) {
      Py_XDECREF(cb_func);
    }

    axis_env_notify_send_data_ctx_destroy(notify_info);
    success = false;
    axis_py_raise_py_runtime_error_exception("Failed to send data.");
    goto done;
  } else {
    // Destroy the C message from the Python message as the ownership has been
    // transferred to the notify_info.
    axis_py_msg_destroy_c_msg(&py_data->msg);
  }

done:
  axis_error_deinit(&err);

  if (success) {
    Py_RETURN_NONE;
  } else {
    return NULL;
  }
}
