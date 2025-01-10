//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_runtime/app/app.h"

#include "include_internal/axis_runtime/binding/python/app/app.h"
#include "include_internal/axis_runtime/binding/python/common.h"
#include "include_internal/axis_runtime/binding/python/common/common.h"
#include "include_internal/axis_runtime/binding/python/common/error.h"
#include "include_internal/axis_runtime/binding/python/axis_env/axis_env.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/metadata/metadata_info.h"
#include "axis_runtime/binding/common.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_runtime/axis_env_proxy/axis_env_proxy.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/macro/mark.h"

static bool axis_py_app_check_integrity(axis_py_app_t *self, bool check_thread) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_PY_APP_SIGNATURE) {
    return false;
  }

  return axis_app_check_integrity(self->c_app, check_thread);
}

static void proxy_on_configure(axis_app_t *app, axis_env_t *axis_env) {
  axis_ASSERT(app && axis_app_check_integrity(app, true), "Invalid argument.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Invalid argument.");

  axis_LOGI("proxy_on_configure");

  // About to call the Python function, so it's necessary to ensure that the GIL
  // has been acquired.
  //
  // The current state may be PyGILState_LOCKED or PyGILState_UNLOCKED which
  // depends on whether the app is running in the Python thread or native
  // thread.
  PyGILState_STATE prev_state = axis_py_gil_state_ensure_internal();

  axis_py_app_t *py_app =
      axis_binding_handle_get_me_in_target_lang((axis_binding_handle_t *)app);
  axis_ASSERT(py_app && axis_py_app_check_integrity(py_app, true),
             "Should not happen.");

  axis_py_axis_env_t *py_axis_env = axis_py_axis_env_wrap(axis_env);
  py_axis_env->c_axis_env_proxy = axis_env_proxy_create(axis_env, 1, NULL);

  // Call python function.
  PyObject *py_res = PyObject_CallMethod((PyObject *)py_app, "on_configure",
                                         "O", py_axis_env->actual_py_axis_env);
  Py_XDECREF(py_res);

  bool err_occurred = axis_py_check_and_clear_py_error();
  axis_ASSERT(!err_occurred, "Should not happen.");

  if (prev_state == PyGILState_UNLOCKED) {
    // Since the original environment did not hold the GIL, we release the gil
    // here. However, an optimization has been made to avoid releasing the
    // thread state, allowing it to be reused later.
    //
    // The effect of not calling PyGILState_Release here is that, since the
    // number of calls to PyGILState_Ensure and PyGILState_Release are not
    // equal, the Python thread state will not be released, only the gil will be
    // released. It is not until on_deinit_done is reached that the
    // corresponding PyGILState_Release for PyGILState_Ensure is called,
    // achieving numerical consistency between PyGILState_Ensure and
    // PyGILState_Release, and only then will the Python thread state be
    // released.
    py_axis_env->py_thread_state = axis_py_eval_save_thread();
  } else {
    // No need to release the GIL.
  }

  py_axis_env->need_to_release_gil_state = true;

  axis_LOGI("proxy_on_configure done");
}

static void proxy_on_init(axis_app_t *app, axis_env_t *axis_env) {
  axis_ASSERT(app && axis_app_check_integrity(app, true), "Invalid argument.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Invalid argument.");

  axis_LOGI("proxy_on_init");

  // About to call the Python function, so it's necessary to ensure that the GIL
  // has been acquired.
  PyGILState_STATE prev_state = axis_py_gil_state_ensure_internal();

  axis_py_app_t *py_app =
      axis_binding_handle_get_me_in_target_lang((axis_binding_handle_t *)app);
  axis_ASSERT(py_app && axis_py_app_check_integrity(py_app, true),
             "Should not happen.");

  axis_py_axis_env_t *py_axis_env = axis_py_axis_env_wrap(axis_env);

  PyObject *py_res = PyObject_CallMethod((PyObject *)py_app, "on_init", "O",
                                         py_axis_env->actual_py_axis_env);
  Py_XDECREF(py_res);

  bool err_occurred = axis_py_check_and_clear_py_error();
  axis_ASSERT(!err_occurred, "Should not happen.");

  axis_py_gil_state_release_internal(prev_state);

  axis_LOGI("proxy_on_init done");
}

static void proxy_on_deinit(axis_app_t *app, axis_env_t *axis_env) {
  axis_ASSERT(app && axis_app_check_integrity(app, true), "Invalid argument.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Invalid argument.");

  axis_LOGI("proxy_on_deinit");

  // About to call the Python function, so it's necessary to ensure that the GIL
  // has been acquired.
  PyGILState_STATE prev_state = axis_py_gil_state_ensure_internal();

  axis_py_app_t *py_app =
      axis_binding_handle_get_me_in_target_lang((axis_binding_handle_t *)app);
  axis_ASSERT(py_app && axis_py_app_check_integrity(py_app, true),
             "Should not happen.");

  axis_py_axis_env_t *py_axis_env = axis_py_axis_env_wrap(axis_env);
  axis_ASSERT(py_axis_env, "Should not happen.");

  PyObject *py_res = PyObject_CallMethod((PyObject *)py_app, "on_deinit", "O",
                                         py_axis_env->actual_py_axis_env);
  Py_XDECREF(py_res);

  bool err_occurred = axis_py_check_and_clear_py_error();
  axis_ASSERT(!err_occurred, "Should not happen.");

  axis_py_gil_state_release_internal(prev_state);

  axis_LOGI("proxy_on_deinit done");
}

static PyObject *axis_py_app_create(PyTypeObject *type, PyObject *args,
                                   axis_UNUSED PyObject *kwds) {
  axis_py_app_t *py_app = (axis_py_app_t *)type->tp_alloc(type, 0);
  if (!py_app) {
    axis_ASSERT(0, "Failed to allocate Python app.");

    return axis_py_raise_py_memory_error_exception(
        "Failed to allocate memory for axis_py_app_t");
  }

  axis_signature_set(&py_app->signature, axis_PY_APP_SIGNATURE);
  py_app->c_app = NULL;

  if (PyTuple_Size(args)) {
    axis_ASSERT(0, "Expect 0 argument.");

    Py_DECREF(py_app);
    return axis_py_raise_py_type_error_exception("Expect 0 argument.");
  }

  py_app->c_app =
      axis_app_create(proxy_on_configure, proxy_on_init, proxy_on_deinit, NULL);
  if (!py_app->c_app) {
    axis_ASSERT(0, "Failed to create APTIMA app.");

    Py_DECREF(py_app);
    return axis_py_raise_py_system_error_exception("Failed to create APTIMA app.");
  }

  axis_binding_handle_set_me_in_target_lang(
      (axis_binding_handle_t *)py_app->c_app, py_app);

  return (PyObject *)py_app;
}

static PyObject *axis_py_app_run(PyObject *self, PyObject *args) {
  axis_py_app_t *py_app = (axis_py_app_t *)self;

  axis_ASSERT(py_app && axis_py_app_check_integrity(py_app, true),
             "Invalid argument.");

  int run_in_background_flag = 1;
  if (!PyArg_ParseTuple(args, "i", &run_in_background_flag)) {
    return NULL;
  }

  axis_LOGI("axis_py_app_run: %d", run_in_background_flag);

  bool rc = false;
  if (run_in_background_flag == 0) {
    PyThreadState *saved_py_thread_state = PyEval_SaveThread();

    // Blocking operation.
    rc = axis_app_run(py_app->c_app, false, NULL);

    PyEval_RestoreThread(saved_py_thread_state);
  } else {
    rc = axis_app_run(py_app->c_app, true, NULL);
  }

  axis_LOGI("axis_py_app_run done: %d", rc);

  if (!rc) {
    return axis_py_raise_py_runtime_error_exception("Failed to run axis_app.");
  }

  bool err_occurred = axis_py_check_and_clear_py_error();
  axis_ASSERT(!err_occurred, "Should not happen.");

  Py_RETURN_NONE;
}

static PyObject *axis_py_app_close(PyObject *self, PyObject *args) {
  axis_py_app_t *py_app = (axis_py_app_t *)self;

  axis_ASSERT(self && axis_py_app_check_integrity(py_app, true),
             "Invalid argument.");

  if (PyTuple_Size(args)) {
    return axis_py_raise_py_type_error_exception("Expect 0 argument.");
  }

  bool rc = axis_app_close(py_app->c_app, NULL);
  if (!rc) {
    return axis_py_raise_py_runtime_error_exception("Failed to close APTIMA app.");
  }

  Py_RETURN_NONE;
}

static PyObject *axis_py_app_wait(PyObject *self, PyObject *args) {
  axis_py_app_t *py_app = (axis_py_app_t *)self;

  axis_ASSERT(py_app && axis_py_app_check_integrity(py_app, true),
             "Invalid argument.");

  axis_LOGI("axis_py_app_wait");

  if (PyTuple_Size(args)) {
    return axis_py_raise_py_type_error_exception("Expect 0 argument.");
  }

  PyThreadState *saved_py_thread_state = PyEval_SaveThread();

  // Blocking operation.
  bool rc = axis_app_wait(py_app->c_app, NULL);

  PyEval_RestoreThread(saved_py_thread_state);

  if (!rc) {
    return axis_py_raise_py_runtime_error_exception(
        "Failed to wait for APTIMA app.");
  }

  axis_LOGI("axis_py_app_wait done");

  Py_RETURN_NONE;
}

static void axis_py_app_destroy(PyObject *self) {
  axis_py_app_t *py_app = (axis_py_app_t *)self;

  axis_ASSERT(py_app && axis_py_app_check_integrity(py_app, true),
             "Invalid argument.");

  axis_app_close(py_app->c_app, NULL);

  axis_app_destroy(py_app->c_app);
  py_app->c_app = NULL;

  Py_TYPE(self)->tp_free(self);
}

// Defines a new Python class named App.
static PyTypeObject *axis_py_app_py_type(void) {
  // Currently, we don't have any properties defined in the Python App class.
  static PyGetSetDef py_app_type_properties[] = {
      {NULL, NULL, NULL, NULL, NULL}};

  static PyMethodDef py_app_type_methods[] = {
      {"run", axis_py_app_run, METH_VARARGS, NULL},
      {"close", axis_py_app_close, METH_VARARGS, NULL},
      {"wait", axis_py_app_wait, METH_VARARGS, NULL},
      {NULL, NULL, 0, NULL},
  };

  static PyTypeObject py_app_type = {
      PyVarObject_HEAD_INIT(NULL, 0).tp_name = "libaxis_runtime_python._App",
      .tp_doc = PyDoc_STR("_App"),
      .tp_basicsize = sizeof(axis_py_app_t),
      .tp_itemsize = 0,
      .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
      .tp_new = axis_py_app_create,
      .tp_init = NULL,
      .tp_dealloc = axis_py_app_destroy,
      .tp_getset = py_app_type_properties,
      .tp_methods = py_app_type_methods,
  };

  return &py_app_type;
}

bool axis_py_app_init_for_module(PyObject *module) {
  PyTypeObject *py_type = axis_py_app_py_type();
  if (PyType_Ready(py_type) < 0) {
    axis_py_raise_py_system_error_exception("Python App class is not ready.");
    axis_ASSERT(0, "Should not happen.");
    return false;
  }

  if (PyModule_AddObjectRef(module, "_App", (PyObject *)py_type) < 0) {
    axis_py_raise_py_import_error_exception(
        "Failed to add Python type to module.");
    return false;
  }

  return true;
}
