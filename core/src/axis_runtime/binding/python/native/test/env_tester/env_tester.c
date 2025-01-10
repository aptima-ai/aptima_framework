//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/binding/python/test/env_tester.h"

#include <stdbool.h>

#include "include_internal/axis_runtime/binding/python/common/error.h"
#include "axis_runtime/binding/common.h"
#include "axis_utils/macro/check.h"

bool axis_py_axis_env_tester_check_integrity(axis_py_axis_env_tester_t *self) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_PY_axis_ENV_TESTER_SIGNATURE) {
    return false;
  }

  return true;
}

static void axis_py_axis_env_tester_c_part_destroyed(
    void *axis_env_tester_bridge_) {
  axis_py_axis_env_tester_t *axis_env_tester_bridge =
      (axis_py_axis_env_tester_t *)axis_env_tester_bridge_;

  axis_ASSERT(axis_env_tester_bridge_ &&
                 axis_py_axis_env_tester_check_integrity(axis_env_tester_bridge_),
             "Should not happen.");

  axis_env_tester_bridge->c_axis_env_tester = NULL;
  axis_py_axis_env_tester_invalidate(axis_env_tester_bridge);
}

static PyObject *create_actual_py_axis_env_tester_instance(
    axis_py_axis_env_tester_t *py_axis_env_tester) {
  // Import the Python module where TenEnvTester is defined.
  PyObject *module_name = PyUnicode_FromString("aptima.test");
  PyObject *module = PyImport_Import(module_name);
  Py_DECREF(module_name);

  if (!module) {
    PyErr_Print();
    return NULL;
  }

  // Get the TenEnvTester class from the module.
  PyObject *axis_env_tester_class =
      PyObject_GetAttrString(module, "TenEnvTester");
  Py_DECREF(module);

  if (!axis_env_tester_class || !PyCallable_Check(axis_env_tester_class)) {
    PyErr_Print();
    Py_XDECREF(axis_env_tester_class);
    return NULL;
  }

  // Create the argument tuple with the _TenEnvTester object
  PyObject *args = PyTuple_Pack(1, py_axis_env_tester);

  // Create an instance of the TenEnvTester class.
  PyObject *axis_env_tester_instance =
      PyObject_CallObject(axis_env_tester_class, args);
  Py_DECREF(axis_env_tester_class);
  Py_DECREF(args);

  if (!axis_env_tester_instance) {
    PyErr_Print();
    return NULL;
  }

  return axis_env_tester_instance;
}

axis_py_axis_env_tester_t *axis_py_axis_env_tester_wrap(
    axis_env_tester_t *axis_env_tester) {
  axis_ASSERT(axis_env_tester, "Invalid argument.");

  axis_py_axis_env_tester_t *py_axis_env_tester =
      axis_binding_handle_get_me_in_target_lang(
          (axis_binding_handle_t *)axis_env_tester);
  if (py_axis_env_tester) {
    // The `axis_env_tester` has already been wrapped, so we directly returns the
    // previously wrapped result.
    return py_axis_env_tester;
  }

  PyTypeObject *py_axis_env_tester_py_type = axis_py_axis_env_tester_type();

  // Create a new py_axis_env for wrapping.
  py_axis_env_tester =
      (axis_py_axis_env_tester_t *)py_axis_env_tester_py_type->tp_alloc(
          py_axis_env_tester_py_type, 0);
  axis_ASSERT(axis_env_tester, "Failed to allocate memory.");

  axis_signature_set(&py_axis_env_tester->signature,
                    axis_PY_axis_ENV_TESTER_SIGNATURE);
  py_axis_env_tester->c_axis_env_tester = axis_env_tester;

  py_axis_env_tester->actual_py_axis_env_tester =
      create_actual_py_axis_env_tester_instance(py_axis_env_tester);
  if (!py_axis_env_tester->actual_py_axis_env_tester) {
    axis_ASSERT(0, "Should not happen.");
    Py_DECREF(py_axis_env_tester);
    return NULL;
  }

  axis_binding_handle_set_me_in_target_lang(
      (axis_binding_handle_t *)axis_env_tester, py_axis_env_tester);

  axis_env_tester_set_destroy_handler_in_target_lang(
      axis_env_tester, axis_py_axis_env_tester_c_part_destroyed);

  return py_axis_env_tester;
}

void axis_py_axis_env_tester_invalidate(
    axis_py_axis_env_tester_t *py_axis_env_tester) {
  axis_ASSERT(py_axis_env_tester, "Should not happen.");

  if (py_axis_env_tester->actual_py_axis_env_tester) {
    Py_DECREF(py_axis_env_tester->actual_py_axis_env_tester);
    py_axis_env_tester->actual_py_axis_env_tester = NULL;
  }

  Py_DECREF(py_axis_env_tester);
}

static void axis_py_axis_env_tester_destroy(PyObject *self) {
  Py_TYPE(self)->tp_free(self);
}

PyTypeObject *axis_py_axis_env_tester_type(void) {
  static PyMethodDef axis_methods[] = {
      {"on_start_done", axis_py_axis_env_tester_on_start_done, METH_VARARGS,
       NULL},
      {"stop_test", axis_py_axis_env_tester_stop_test, METH_VARARGS, NULL},
      {"send_cmd", axis_py_axis_env_tester_send_cmd, METH_VARARGS, NULL},
      {"send_data", axis_py_axis_env_tester_send_data, METH_VARARGS, NULL},
      {"send_audio_frame", axis_py_axis_env_tester_send_audio_frame, METH_VARARGS,
       NULL},
      {"send_video_frame", axis_py_axis_env_tester_send_video_frame, METH_VARARGS,
       NULL},
      {"return_result", axis_py_axis_env_tester_return_result, METH_VARARGS,
       NULL},
      {NULL, NULL, 0, NULL},
  };

  static PyTypeObject ty = {
      PyVarObject_HEAD_INIT(NULL, 0).tp_name =
          "libaxis_runtime_python._TenEnvTester",
      .tp_doc = PyDoc_STR("_TenEnvTester"),
      .tp_basicsize = sizeof(axis_py_axis_env_tester_t),
      .tp_itemsize = 0,
      .tp_flags = Py_TPFLAGS_DEFAULT,

      // The metadata info will be created by the Python binding and not by the
      // user within the Python environment.
      .tp_new = NULL,

      .tp_init = NULL,
      .tp_dealloc = axis_py_axis_env_tester_destroy,
      .tp_getset = NULL,
      .tp_methods = axis_methods,
  };
  return &ty;
}

bool axis_py_axis_env_tester_init_for_module(PyObject *module) {
  PyTypeObject *py_type = axis_py_axis_env_tester_type();
  if (PyType_Ready(axis_py_axis_env_tester_type()) < 0) {
    axis_py_raise_py_system_error_exception(
        "Python TenEnvTester class is not ready.");
    axis_ASSERT(0, "Should not happen.");
    return false;
  }

  if (PyModule_AddObjectRef(module, "_TenEnvTester", (PyObject *)py_type) < 0) {
    axis_py_raise_py_import_error_exception(
        "Failed to add Python type to module.");
    return false;
  }

  return true;
}
