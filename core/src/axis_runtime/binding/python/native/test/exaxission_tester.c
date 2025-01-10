//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/binding/python/test/extension_tester.h"

#include <stdbool.h>

#include "include_internal/axis_runtime/binding/python/common/common.h"
#include "include_internal/axis_runtime/binding/python/common/error.h"
#include "include_internal/axis_runtime/binding/python/msg/audio_frame.h"
#include "include_internal/axis_runtime/binding/python/msg/cmd.h"
#include "include_internal/axis_runtime/binding/python/msg/data.h"
#include "include_internal/axis_runtime/binding/python/msg/video_frame.h"
#include "include_internal/axis_runtime/binding/python/test/env_tester.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/test/extension_tester.h"
#include "axis_runtime/binding/common.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"

static bool axis_py_extension_tester_check_integrity(
    axis_py_extension_tester_t *self) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_PY_EXTENSION_TESTER_SIGNATURE) {
    return false;
  }

  return true;
}

static axis_py_extension_tester_t *axis_py_extension_tester_create_internal(
    PyTypeObject *py_type) {
  if (!py_type) {
    py_type = axis_py_extension_tester_py_type();
  }

  axis_py_extension_tester_t *py_extension_tester =
      (axis_py_extension_tester_t *)py_type->tp_alloc(py_type, 0);

  axis_signature_set(&py_extension_tester->signature,
                    axis_PY_EXTENSION_TESTER_SIGNATURE);
  py_extension_tester->c_extension_tester = NULL;

  return py_extension_tester;
}

static void proxy_on_start(axis_extension_tester_t *extension_tester,
                           axis_env_tester_t *axis_env_tester) {
  axis_ASSERT(extension_tester &&
                 axis_extension_tester_check_integrity(extension_tester, true),
             "Invalid argument.");
  axis_ASSERT(axis_env_tester && axis_env_tester_check_integrity(axis_env_tester),
             "Invalid argument.");

  // About to call the Python function, so it's necessary to ensure that the GIL
  // has been acquired.
  PyGILState_STATE prev_state = axis_py_gil_state_ensure_internal();

  axis_py_extension_tester_t *py_extension_tester =
      (axis_py_extension_tester_t *)axis_binding_handle_get_me_in_target_lang(
          (axis_binding_handle_t *)extension_tester);
  axis_ASSERT(py_extension_tester &&
                 axis_py_extension_tester_check_integrity(py_extension_tester),
             "Invalid argument.");

  axis_py_axis_env_tester_t *py_axis_env_tester =
      axis_py_axis_env_tester_wrap(axis_env_tester);
  py_extension_tester->py_axis_env_tester = (PyObject *)py_axis_env_tester;
  axis_ASSERT(py_axis_env_tester->actual_py_axis_env_tester, "Should not happen.");

  PyObject *py_res =
      PyObject_CallMethod((PyObject *)py_extension_tester, "on_start", "O",
                          py_axis_env_tester->actual_py_axis_env_tester);
  Py_XDECREF(py_res);

  bool err_occurred = axis_py_check_and_clear_py_error();
  axis_ASSERT(!err_occurred, "Should not happen.");

  axis_py_gil_state_release_internal(prev_state);
}

static void proxy_on_cmd(axis_extension_tester_t *extension_tester,
                         axis_env_tester_t *axis_env_tester,
                         axis_shared_ptr_t *cmd) {
  axis_ASSERT(extension_tester &&
                 axis_extension_tester_check_integrity(extension_tester, true),
             "Invalid argument.");
  axis_ASSERT(axis_env_tester && axis_env_tester_check_integrity(axis_env_tester),
             "Invalid argument.");
  axis_ASSERT(cmd && axis_msg_check_integrity(cmd), "Invalid argument.");

  // About to call the Python function, so it's necessary to ensure that the GIL
  // has been acquired.
  PyGILState_STATE prev_state = axis_py_gil_state_ensure_internal();

  axis_py_extension_tester_t *py_extension_tester =
      (axis_py_extension_tester_t *)axis_binding_handle_get_me_in_target_lang(
          (axis_binding_handle_t *)extension_tester);
  axis_ASSERT(py_extension_tester &&
                 axis_py_extension_tester_check_integrity(py_extension_tester),
             "Invalid argument.");

  PyObject *py_axis_env_tester = py_extension_tester->py_axis_env_tester;
  axis_ASSERT(py_axis_env_tester, "Should not happen.");
  axis_ASSERT(
      ((axis_py_axis_env_tester_t *)py_axis_env_tester)->actual_py_axis_env_tester,
      "Should not happen.");

  axis_py_cmd_t *py_cmd = axis_py_cmd_wrap(cmd);

  PyObject *py_res = PyObject_CallMethod(
      (PyObject *)py_extension_tester, "on_cmd", "OO",
      ((axis_py_axis_env_tester_t *)py_axis_env_tester)->actual_py_axis_env_tester,
      py_cmd);
  Py_XDECREF(py_res);

  bool err_occurred = axis_py_check_and_clear_py_error();
  axis_ASSERT(!err_occurred, "Should not happen.");

  axis_py_cmd_invalidate(py_cmd);

  axis_py_gil_state_release_internal(prev_state);
}

static void proxy_on_data(axis_extension_tester_t *extension_tester,
                          axis_env_tester_t *axis_env_tester,
                          axis_shared_ptr_t *data) {
  axis_ASSERT(extension_tester &&
                 axis_extension_tester_check_integrity(extension_tester, true),
             "Invalid argument.");
  axis_ASSERT(axis_env_tester && axis_env_tester_check_integrity(axis_env_tester),
             "Invalid argument.");
  axis_ASSERT(data && axis_msg_check_integrity(data), "Invalid argument.");

  // About to call the Python function, so it's necessary to ensure that the GIL
  // has been acquired.
  PyGILState_STATE prev_state = axis_py_gil_state_ensure_internal();

  axis_py_extension_tester_t *py_extension_tester =
      (axis_py_extension_tester_t *)axis_binding_handle_get_me_in_target_lang(
          (axis_binding_handle_t *)extension_tester);
  axis_ASSERT(py_extension_tester &&
                 axis_py_extension_tester_check_integrity(py_extension_tester),
             "Invalid argument.");

  PyObject *py_axis_env_tester = py_extension_tester->py_axis_env_tester;
  axis_ASSERT(py_axis_env_tester, "Should not happen.");
  axis_ASSERT(
      ((axis_py_axis_env_tester_t *)py_axis_env_tester)->actual_py_axis_env_tester,
      "Should not happen.");

  axis_py_data_t *py_data = axis_py_data_wrap(data);

  PyObject *py_res = PyObject_CallMethod(
      (PyObject *)py_extension_tester, "on_data", "OO",
      ((axis_py_axis_env_tester_t *)py_axis_env_tester)->actual_py_axis_env_tester,
      py_data);
  Py_XDECREF(py_res);

  bool err_occurred = axis_py_check_and_clear_py_error();
  axis_ASSERT(!err_occurred, "Should not happen.");

  axis_py_data_invalidate(py_data);

  axis_py_gil_state_release_internal(prev_state);
}

static void proxy_on_audio_frame(axis_extension_tester_t *extension_tester,
                                 axis_env_tester_t *axis_env_tester,
                                 axis_shared_ptr_t *audio_frame) {
  axis_ASSERT(extension_tester &&
                 axis_extension_tester_check_integrity(extension_tester, true),
             "Invalid argument.");
  axis_ASSERT(axis_env_tester && axis_env_tester_check_integrity(axis_env_tester),
             "Invalid argument.");
  axis_ASSERT(audio_frame && axis_msg_check_integrity(audio_frame),
             "Invalid argument.");

  // About to call the Python function, so it's necessary to ensure that the GIL
  // has been acquired.
  PyGILState_STATE prev_state = axis_py_gil_state_ensure_internal();

  axis_py_extension_tester_t *py_extension_tester =
      (axis_py_extension_tester_t *)axis_binding_handle_get_me_in_target_lang(
          (axis_binding_handle_t *)extension_tester);
  axis_ASSERT(py_extension_tester &&
                 axis_py_extension_tester_check_integrity(py_extension_tester),
             "Invalid argument.");

  PyObject *py_axis_env_tester = py_extension_tester->py_axis_env_tester;
  axis_ASSERT(py_axis_env_tester, "Should not happen.");
  axis_ASSERT(
      ((axis_py_axis_env_tester_t *)py_axis_env_tester)->actual_py_axis_env_tester,
      "Should not happen.");

  axis_py_audio_frame_t *py_audio_frame = axis_py_audio_frame_wrap(audio_frame);

  PyObject *py_res = PyObject_CallMethod(
      (PyObject *)py_extension_tester, "on_audio_frame", "OO",
      ((axis_py_axis_env_tester_t *)py_axis_env_tester)->actual_py_axis_env_tester,
      py_audio_frame);
  Py_XDECREF(py_res);

  bool err_occurred = axis_py_check_and_clear_py_error();
  axis_ASSERT(!err_occurred, "Should not happen.");

  axis_py_audio_frame_invalidate(py_audio_frame);

  axis_py_gil_state_release_internal(prev_state);
}

static void proxy_on_video_frame(axis_extension_tester_t *extension_tester,
                                 axis_env_tester_t *axis_env_tester,
                                 axis_shared_ptr_t *video_frame) {
  axis_ASSERT(extension_tester &&
                 axis_extension_tester_check_integrity(extension_tester, true),
             "Invalid argument.");
  axis_ASSERT(axis_env_tester && axis_env_tester_check_integrity(axis_env_tester),
             "Invalid argument.");
  axis_ASSERT(video_frame && axis_msg_check_integrity(video_frame),
             "Invalid argument.");

  // About to call the Python function, so it's necessary to ensure that the GIL
  // has been acquired.
  PyGILState_STATE prev_state = axis_py_gil_state_ensure_internal();

  axis_py_extension_tester_t *py_extension_tester =
      (axis_py_extension_tester_t *)axis_binding_handle_get_me_in_target_lang(
          (axis_binding_handle_t *)extension_tester);
  axis_ASSERT(py_extension_tester &&
                 axis_py_extension_tester_check_integrity(py_extension_tester),
             "Invalid argument.");

  PyObject *py_axis_env_tester = py_extension_tester->py_axis_env_tester;
  axis_ASSERT(py_axis_env_tester, "Should not happen.");
  axis_ASSERT(
      ((axis_py_axis_env_tester_t *)py_axis_env_tester)->actual_py_axis_env_tester,
      "Should not happen.");

  axis_py_video_frame_t *py_video_frame = axis_py_video_frame_wrap(video_frame);

  PyObject *py_res = PyObject_CallMethod(
      (PyObject *)py_extension_tester, "on_video_frame", "OO",
      ((axis_py_axis_env_tester_t *)py_axis_env_tester)->actual_py_axis_env_tester,
      py_video_frame);
  Py_XDECREF(py_res);

  bool err_occurred = axis_py_check_and_clear_py_error();
  axis_ASSERT(!err_occurred, "Should not happen.");

  axis_py_video_frame_invalidate(py_video_frame);

  axis_py_gil_state_release_internal(prev_state);
}

static axis_py_extension_tester_t *axis_py_extension_tester_init(
    axis_py_extension_tester_t *py_extension_tester, axis_UNUSED PyObject *args,
    axis_UNUSED PyObject *kw) {
  axis_ASSERT(py_extension_tester &&
                 axis_py_extension_tester_check_integrity(
                     (axis_py_extension_tester_t *)py_extension_tester),
             "Invalid argument.");

  py_extension_tester->c_extension_tester =
      axis_extension_tester_create(proxy_on_start, proxy_on_cmd, proxy_on_data,
                                  proxy_on_audio_frame, proxy_on_video_frame);

  axis_binding_handle_set_me_in_target_lang(
      &py_extension_tester->c_extension_tester->binding_handle,
      py_extension_tester);
  py_extension_tester->py_axis_env_tester = Py_None;

  return py_extension_tester;
}

PyObject *axis_py_extension_tester_create(PyTypeObject *type,
                                         axis_UNUSED PyObject *args,
                                         axis_UNUSED PyObject *kwds) {
  axis_py_extension_tester_t *py_extension_tester =
      axis_py_extension_tester_create_internal(type);
  return (PyObject *)axis_py_extension_tester_init(py_extension_tester, args,
                                                  kwds);
}

void axis_py_extension_tester_destroy(PyObject *self) {
  axis_py_extension_tester_t *py_extension_tester =
      (axis_py_extension_tester_t *)self;
  axis_ASSERT(py_extension_tester &&
                 axis_py_extension_tester_check_integrity(
                     (axis_py_extension_tester_t *)py_extension_tester),
             "Invalid argument.");

  axis_extension_tester_destroy(py_extension_tester->c_extension_tester);
  Py_TYPE(self)->tp_free(self);
}

static PyObject *axis_py_extension_tester_set_test_mode_single(PyObject *self,
                                                              PyObject *args) {
  axis_py_extension_tester_t *py_extension_tester =
      (axis_py_extension_tester_t *)self;
  axis_ASSERT(py_extension_tester &&
                 axis_py_extension_tester_check_integrity(py_extension_tester),
             "Invalid argument.");

  if (PyTuple_GET_SIZE(args) != 2) {
    return axis_py_raise_py_value_error_exception(
        "Invalid argument count when extension_tester.set_test_mode_single.");
  }

  const char *addon_name = NULL;
  const char *property_json_str = NULL;
  if (!PyArg_ParseTuple(args, "sz", &addon_name, &property_json_str)) {
    return axis_py_raise_py_value_error_exception(
        "Failed to parse arguments when "
        "extension_tester.set_test_mode_single.");
  }

  axis_extension_tester_set_test_mode_single(
      py_extension_tester->c_extension_tester, addon_name, property_json_str);

  Py_RETURN_NONE;
}

static PyObject *axis_py_extension_tester_run(PyObject *self, PyObject *args) {
  axis_py_extension_tester_t *py_extension_tester =
      (axis_py_extension_tester_t *)self;

  axis_ASSERT(py_extension_tester &&
                 axis_py_extension_tester_check_integrity(py_extension_tester),
             "Invalid argument.");

  axis_LOGI("axis_py_extension_tester_run");

  PyThreadState *saved_py_thread_state = PyEval_SaveThread();

  // Blocking operation.
  bool rc = axis_extension_tester_run(py_extension_tester->c_extension_tester);

  PyEval_RestoreThread(saved_py_thread_state);

  axis_LOGI("axis_py_extension_tester_run done: %d", rc);

  if (!rc) {
    return axis_py_raise_py_runtime_error_exception(
        "Failed to run axis_extension_tester.");
  }

  bool err_occurred = axis_py_check_and_clear_py_error();
  axis_ASSERT(!err_occurred, "Should not happen.");

  Py_RETURN_NONE;
}

PyTypeObject *axis_py_extension_tester_py_type(void) {
  static PyMethodDef py_methods[] = {
      {"set_test_mode_single", axis_py_extension_tester_set_test_mode_single,
       METH_VARARGS, NULL},
      {"run", axis_py_extension_tester_run, METH_VARARGS, NULL},
      {NULL, NULL, 0, NULL},
  };

  static PyTypeObject py_type = {
      PyVarObject_HEAD_INIT(NULL, 0).tp_name =
          "libaxis_runtime_python._ExtensionTester",
      .tp_doc = PyDoc_STR("_ExtensionTester"),
      .tp_basicsize = sizeof(axis_py_extension_tester_t),
      .tp_itemsize = 0,
      .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
      .tp_new = axis_py_extension_tester_create,
      .tp_init = NULL,
      .tp_dealloc = axis_py_extension_tester_destroy,
      .tp_getset = NULL,
      .tp_methods = py_methods,
  };

  return &py_type;
}

bool axis_py_extension_tester_init_for_module(PyObject *module) {
  PyTypeObject *py_type = axis_py_extension_tester_py_type();
  if (PyType_Ready(py_type) < 0) {
    axis_py_raise_py_system_error_exception(
        "Python ExtensionTester class is not ready.");

    axis_ASSERT(0, "Should not happen.");
    return false;
  }

  if (PyModule_AddObjectRef(module, "_ExtensionTester", (PyObject *)py_type) <
      0) {
    axis_py_raise_py_import_error_exception(
        "Failed to add Python type to module.");

    axis_ASSERT(0, "Should not happen.");
    return false;
  }
  return true;
}
