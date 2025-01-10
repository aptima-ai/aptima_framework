//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <string.h>

#include "include_internal/axis_runtime/binding/python/common.h"
#include "include_internal/axis_runtime/binding/python/common/common.h"
#include "include_internal/axis_runtime/binding/python/common/error.h"
#include "include_internal/axis_runtime/binding/python/axis_env/axis_env.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "pystate.h"
#include "axis_runtime/axis_env/internal/on_xxx_done.h"
#include "axis_runtime/axis_env_proxy/axis_env_proxy.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"

static void axis_env_proxy_notify_on_deinit_done(axis_env_t *axis_env,
                                                void *user_data) {
  axis_ASSERT(
      axis_env &&
          axis_env_check_integrity(
              axis_env,
              axis_env->attach_to != axis_ENV_ATTACH_TO_ADDON ? true : false),
      "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  axis_py_axis_env_t *py_axis_env = user_data;
  axis_ASSERT(py_axis_env, "Should not happen.");

  // Notify the Python side to do the cleanup.
  //
  // About to call the Python function, so it's necessary to ensure that the
  // GIL has been acquired.
  PyGILState_STATE prev_state = axis_py_gil_state_ensure_internal();

  PyObject *py_res =
      PyObject_CallMethod(py_axis_env->actual_py_axis_env, "_on_release", NULL);
  Py_XDECREF(py_res);

  bool err_occurred = axis_py_check_and_clear_py_error();
  axis_ASSERT(!err_occurred, "Should not happen.");

  axis_py_gil_state_release_internal(prev_state);

  if (py_axis_env->c_axis_env_proxy) {
    axis_ASSERT(
        axis_env_proxy_get_thread_cnt(py_axis_env->c_axis_env_proxy, NULL) == 1,
        "Should not happen.");

    axis_env_proxy_t *axis_env_proxy = py_axis_env->c_axis_env_proxy;

    py_axis_env->c_axis_env_proxy = NULL;

    bool rc = axis_env_proxy_release(axis_env_proxy, &err);
    axis_ASSERT(rc, "Should not happen.");
  }

  bool rc = axis_env_on_deinit_done(axis_env, &err);
  axis_ASSERT(rc, "Should not happen.");

  axis_error_deinit(&err);

  if (py_axis_env->need_to_release_gil_state) {
    if (!axis_py_is_holding_gil()) {
      axis_ASSERT(py_axis_env->py_thread_state != NULL, "Should not happen.");

      // The gil is not held by the current thread, so we have to acquire the
      // gil before we can release the gil state.
      axis_py_eval_restore_thread(py_axis_env->py_thread_state);

      // Release the gil state and the gil.
      axis_py_gil_state_release_internal(PyGILState_UNLOCKED);
    } else {
      // Release the gil state but keep holding the gil.
      axis_py_gil_state_release_internal(PyGILState_LOCKED);
    }
  }
}

PyObject *axis_py_axis_env_on_deinit_done(PyObject *self,
                                        axis_UNUSED PyObject *args) {
  axis_py_axis_env_t *py_axis_env = (axis_py_axis_env_t *)self;
  axis_ASSERT(py_axis_env && axis_py_axis_env_check_integrity(py_axis_env),
             "Invalid argument.");

  axis_error_t err;
  axis_error_init(&err);

  bool rc = true;
  if (py_axis_env->c_axis_env->attach_to == axis_ENV_ATTACH_TO_ADDON) {
    rc = axis_env_on_deinit_done(py_axis_env->c_axis_env, &err);
  } else {
    rc = axis_env_proxy_notify(py_axis_env->c_axis_env_proxy,
                              axis_env_proxy_notify_on_deinit_done, py_axis_env,
                              false, &err);
  }

  axis_error_deinit(&err);

  axis_ASSERT(rc, "Should not happen.");

  Py_RETURN_NONE;
}
