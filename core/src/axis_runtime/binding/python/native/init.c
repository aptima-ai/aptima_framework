//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/binding/python/addon/addon.h"
#include "include_internal/axis_runtime/binding/python/addon/addon_manager.h"
#include "include_internal/axis_runtime/binding/python/app/app.h"
#include "include_internal/axis_runtime/binding/python/common/buf.h"
#include "include_internal/axis_runtime/binding/python/common/error.h"
#include "include_internal/axis_runtime/binding/python/extension/extension.h"
#include "include_internal/axis_runtime/binding/python/msg/audio_frame.h"
#include "include_internal/axis_runtime/binding/python/msg/cmd.h"
#include "include_internal/axis_runtime/binding/python/msg/cmd_result.h"
#include "include_internal/axis_runtime/binding/python/msg/data.h"
#include "include_internal/axis_runtime/binding/python/msg/msg.h"
#include "include_internal/axis_runtime/binding/python/msg/video_frame.h"
#include "include_internal/axis_runtime/binding/python/axis_env/axis_env.h"
#include "include_internal/axis_runtime/binding/python/test/env_tester.h"
#include "include_internal/axis_runtime/binding/python/test/extension_tester.h"

// Note on memory leaks of Python VM.
//
// The Python VM seems to consistently have memory leaks. In the official Python
// test suite, the ability to detect leaks is even turned off.
// https://github.com/python/buildmaster-config/blob/main/master/custom/factories.py#L240.
//
// Additionally, in Atheris: A Coverage-Guided, Native Python Fuzzer
// (https://github.com/google/atheris/blob/master/native_extension_fuzzing.md#leak-detection),
// it is also recommended to disable the leak detection when using Python.
//
// Therefore, we can only enable LSan in some simple test cases. Beyond that, we
// largely need to rely on the TEN's own memory tracking feature to ensure that
// the TEN itself has no memory leaks. This feature in the TEN can be seen in
// the TEN Memory Sanitizer.

static PyModuleDef *axis_py_runtime_module(void) {
  static struct PyMethodDef module_methods[] = {
      {"_register_addon_as_extension",
       axis_py_addon_manager_register_addon_as_extension, METH_VARARGS,
       "Register an addon as an extension"},
      {"_unregister_all_addons_and_cleanup",
       axis_py_unregister_all_addons_and_cleanup, METH_VARARGS,
       "Unregister all addons and cleanup"},
      {NULL, NULL, 0, NULL}};

  static struct PyModuleDef module_def = {
      PyModuleDef_HEAD_INIT,
      "libaxis_runtime_python",
      NULL,
      -1,
      module_methods,
      NULL,
      0,
      0,
      0,
  };

  return &module_def;
}

PyMODINIT_FUNC PyInit_libaxis_runtime_python(void) {
  PyObject *module = PyModule_Create(axis_py_runtime_module());
  if (!module) {
    return axis_py_raise_py_system_error_exception(
        "Failed create axis_runtime_python module.");
  }

  if (!axis_py_addon_init_for_module(module)) {
    Py_DECREF(module);
    return NULL;
  }

  if (!axis_py_app_init_for_module(module)) {
    Py_DECREF(module);
    return NULL;
  }

  if (!axis_py_axis_env_init_for_module(module)) {
    Py_DECREF(module);
    return NULL;
  }

  if (!axis_py_extension_init_for_module(module)) {
    Py_DECREF(module);
    return NULL;
  }

  if (!axis_py_msg_init_for_module(module)) {
    Py_DECREF(module);
    return NULL;
  }

  if (!axis_py_cmd_init_for_module(module)) {
    Py_DECREF(module);
    return NULL;
  }

  if (!axis_py_cmd_result_init_for_module(module)) {
    Py_DECREF(module);
    return NULL;
  }

  if (!axis_py_data_init_for_module(module)) {
    Py_DECREF(module);
    return NULL;
  }

  if (!axis_py_video_frame_init_for_module(module)) {
    Py_DECREF(module);
    return NULL;
  }

  if (!axis_py_audio_frame_init_for_module(module)) {
    Py_DECREF(module);
    return NULL;
  }

  if (!axis_py_buf_init_for_module(module)) {
    Py_DECREF(module);
    return NULL;
  }

  if (!axis_py_extension_tester_init_for_module(module)) {
    Py_DECREF(module);
    return NULL;
  }

  if (!axis_py_axis_env_tester_init_for_module(module)) {
    Py_DECREF(module);
    return NULL;
  }

  if (!axis_py_error_init_for_module(module)) {
    Py_DECREF(module);
    return NULL;
  }

  return module;
}
