//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/extension/extension.h"

#include "include_internal/axis_runtime/binding/python/common.h"
#include "include_internal/axis_runtime/binding/python/common/common.h"
#include "include_internal/axis_runtime/binding/python/common/error.h"
#include "include_internal/axis_runtime/binding/python/extension/extension.h"
#include "include_internal/axis_runtime/binding/python/msg/audio_frame.h"
#include "include_internal/axis_runtime/binding/python/msg/cmd.h"
#include "include_internal/axis_runtime/binding/python/msg/data.h"
#include "include_internal/axis_runtime/binding/python/msg/video_frame.h"
#include "include_internal/axis_runtime/binding/python/axis_env/axis_env.h"
#include "include_internal/axis_runtime/metadata/metadata_info.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/axis_env_proxy/axis_env_proxy.h"
#include "axis_runtime/binding/common.h"
#include "axis_runtime/extension/extension.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_runtime/axis_env_proxy/axis_env_proxy.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/macro/mark.h"

static bool axis_py_extension_check_integrity(axis_py_extension_t *self,
                                             bool check_thread) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_PY_EXTENSION_SIGNATURE) {
    return false;
  }

  return axis_extension_check_integrity(self->c_extension, check_thread);
}

static PyObject *stub_on_callback(axis_UNUSED PyObject *self,
                                  axis_UNUSED PyObject *args) {
  Py_RETURN_NONE;
}

static void proxy_on_configure(axis_extension_t *extension, axis_env_t *axis_env) {
  axis_ASSERT(extension && axis_extension_check_integrity(extension, true),
             "Invalid argument.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Invalid argument.");

  // About to call the Python function, so it's necessary to ensure that the GIL
  // has been acquired.
  PyGILState_STATE prev_state = axis_py_gil_state_ensure_internal();
  // This function can only be called on the native thread not a Python
  // thread.
  axis_ASSERT(prev_state == PyGILState_UNLOCKED,
             "The GIL should not be help by the extension thread now.");

  axis_py_extension_t *py_extension =
      (axis_py_extension_t *)axis_binding_handle_get_me_in_target_lang(
          (axis_binding_handle_t *)extension);
  axis_ASSERT(
      py_extension && axis_py_extension_check_integrity(py_extension, true),
      "Invalid argument.");

  axis_py_axis_env_t *py_axis_env = axis_py_axis_env_wrap(axis_env);
  py_extension->py_axis_env = (PyObject *)py_axis_env;

  py_axis_env->c_axis_env_proxy = axis_env_proxy_create(axis_env, 1, NULL);
  axis_ASSERT(py_axis_env->c_axis_env_proxy &&
                 axis_env_proxy_check_integrity(py_axis_env->c_axis_env_proxy),
             "Invalid argument.");

  PyObject *py_res =
      PyObject_CallMethod((PyObject *)py_extension, "_proxy_on_configure", "O",
                          py_axis_env->actual_py_axis_env);
  Py_XDECREF(py_res);

  bool err_occurred = axis_py_check_and_clear_py_error();
  axis_ASSERT(!err_occurred, "Should not happen.");

  // We should release the GIL but not destroy the PyThreadState. The
  // PyThreadState will not be released until the last extension calls
  // 'on_deinit_done' in the group.
  py_axis_env->py_thread_state = axis_py_eval_save_thread();

  py_axis_env->need_to_release_gil_state = true;
}

static void proxy_on_init(axis_extension_t *extension, axis_env_t *axis_env) {
  axis_ASSERT(extension && axis_extension_check_integrity(extension, true),
             "Invalid argument.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Invalid argument.");

  // About to call the Python function, so it's necessary to ensure that the GIL
  // has been acquired.
  PyGILState_STATE prev_state = axis_py_gil_state_ensure_internal();
  // This function can only be called on the native thread not a Python
  // thread.
  axis_ASSERT(prev_state == PyGILState_UNLOCKED,
             "The GIL should not be help by the extension thread now.");

  axis_py_extension_t *py_extension =
      (axis_py_extension_t *)axis_binding_handle_get_me_in_target_lang(
          (axis_binding_handle_t *)extension);
  axis_ASSERT(
      py_extension && axis_py_extension_check_integrity(py_extension, true),
      "Invalid argument.");

  PyObject *py_axis_env = py_extension->py_axis_env;
  axis_ASSERT(py_axis_env, "Should not happen.");

  PyObject *py_res =
      PyObject_CallMethod((PyObject *)py_extension, "_proxy_on_init", "O",
                          ((axis_py_axis_env_t *)py_axis_env)->actual_py_axis_env);
  Py_XDECREF(py_res);

  bool err_occurred = axis_py_check_and_clear_py_error();
  axis_ASSERT(!err_occurred, "Should not happen.");

  axis_py_gil_state_release_internal(prev_state);
}

static void proxy_on_start(axis_extension_t *extension, axis_env_t *axis_env) {
  axis_ASSERT(extension && axis_extension_check_integrity(extension, true),
             "Invalid argument.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Invalid argument.");

  // About to call the Python function, so it's necessary to ensure that the GIL
  // has been acquired.
  PyGILState_STATE prev_state = axis_py_gil_state_ensure_internal();
  axis_ASSERT(prev_state == PyGILState_UNLOCKED,
             "The GIL should not be help by the extension thread now.");

  axis_py_extension_t *py_extension =
      (axis_py_extension_t *)axis_binding_handle_get_me_in_target_lang(
          (axis_binding_handle_t *)extension);
  axis_ASSERT(
      py_extension && axis_py_extension_check_integrity(py_extension, true),
      "Invalid argument.");

  PyObject *py_axis_env = py_extension->py_axis_env;
  axis_ASSERT(py_axis_env, "Should not happen.");

  PyObject *py_res =
      PyObject_CallMethod((PyObject *)py_extension, "_proxy_on_start", "O",
                          ((axis_py_axis_env_t *)py_axis_env)->actual_py_axis_env);
  Py_XDECREF(py_res);

  bool err_occurred = axis_py_check_and_clear_py_error();
  axis_ASSERT(!err_occurred, "Should not happen.");

  axis_py_gil_state_release_internal(prev_state);
}

static void proxy_on_stop(axis_extension_t *extension, axis_env_t *axis_env) {
  axis_ASSERT(extension && axis_extension_check_integrity(extension, true),
             "Invalid argument.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Invalid argument.");

  // About to call the Python function, so it's necessary to ensure that the GIL
  // has been acquired.
  PyGILState_STATE prev_state = axis_py_gil_state_ensure_internal();
  axis_ASSERT(prev_state == PyGILState_UNLOCKED,
             "The GIL should not be help by the extension thread now.");

  axis_py_extension_t *py_extension =
      (axis_py_extension_t *)axis_binding_handle_get_me_in_target_lang(
          (axis_binding_handle_t *)extension);
  axis_ASSERT(
      py_extension && axis_py_extension_check_integrity(py_extension, true),
      "Invalid argument.");

  PyObject *py_axis_env = py_extension->py_axis_env;
  axis_ASSERT(py_axis_env, "Should not happen.");

  PyObject *py_res =
      PyObject_CallMethod((PyObject *)py_extension, "_proxy_on_stop", "O",
                          ((axis_py_axis_env_t *)py_axis_env)->actual_py_axis_env);
  Py_XDECREF(py_res);

  bool err_occurred = axis_py_check_and_clear_py_error();
  axis_ASSERT(!err_occurred, "Should not happen.");

  axis_py_gil_state_release_internal(prev_state);
}

static void proxy_on_deinit(axis_extension_t *extension, axis_env_t *axis_env) {
  axis_ASSERT(extension && axis_extension_check_integrity(extension, true),
             "Invalid argument.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Invalid argument.");

  // About to call the Python function, so it's necessary to ensure that the GIL
  // has been acquired.
  PyGILState_STATE prev_state = axis_py_gil_state_ensure_internal();
  axis_ASSERT(prev_state == PyGILState_UNLOCKED,
             "The GIL should not be help by the extension thread now.");

  axis_py_extension_t *py_extension =
      (axis_py_extension_t *)axis_binding_handle_get_me_in_target_lang(
          (axis_binding_handle_t *)extension);
  axis_ASSERT(
      py_extension && axis_py_extension_check_integrity(py_extension, true),
      "Invalid argument.");

  PyObject *py_axis_env = py_extension->py_axis_env;
  axis_ASSERT(py_axis_env, "Should not happen.");

  PyObject *py_res =
      PyObject_CallMethod((PyObject *)py_extension, "_proxy_on_deinit", "O",
                          ((axis_py_axis_env_t *)py_axis_env)->actual_py_axis_env);
  Py_XDECREF(py_res);

  bool err_occurred = axis_py_check_and_clear_py_error();
  axis_ASSERT(!err_occurred, "Should not happen.");

  axis_py_gil_state_release_internal(prev_state);
}

static void proxy_on_cmd(axis_extension_t *extension, axis_env_t *axis_env,
                         axis_shared_ptr_t *cmd) {
  axis_ASSERT(extension && axis_extension_check_integrity(extension, true),
             "Invalid argument.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Invalid argument.");
  axis_ASSERT(cmd && axis_msg_check_integrity(cmd), "Invalid argument.");

  // About to call the Python function, so it's necessary to ensure that the GIL
  // has been acquired.
  PyGILState_STATE prev_state = axis_py_gil_state_ensure_internal();

  axis_py_extension_t *py_extension =
      (axis_py_extension_t *)axis_binding_handle_get_me_in_target_lang(
          (axis_binding_handle_t *)extension);
  axis_ASSERT(
      py_extension && axis_py_extension_check_integrity(py_extension, true),
      "Invalid argument.");

  PyObject *py_axis_env = py_extension->py_axis_env;
  axis_ASSERT(py_axis_env, "Should not happen.");

  axis_py_cmd_t *py_cmd = axis_py_cmd_wrap(cmd);

  PyObject *py_res = PyObject_CallMethod(
      (PyObject *)py_extension, "_proxy_on_cmd", "OO",
      ((axis_py_axis_env_t *)py_axis_env)->actual_py_axis_env, py_cmd);
  Py_XDECREF(py_res);

  bool err_occurred = axis_py_check_and_clear_py_error();
  axis_ASSERT(!err_occurred, "Should not happen.");

  axis_py_cmd_invalidate(py_cmd);

  axis_py_gil_state_release_internal(prev_state);
}

static void proxy_on_data(axis_extension_t *extension, axis_env_t *axis_env,
                          axis_shared_ptr_t *data) {
  axis_ASSERT(extension && axis_extension_check_integrity(extension, true),
             "Invalid argument.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Invalid argument.");
  axis_ASSERT(data && axis_msg_check_integrity(data), "Invalid argument.");

  // About to call the Python function, so it's necessary to ensure that the GIL
  // has been acquired.
  PyGILState_STATE prev_state = axis_py_gil_state_ensure_internal();

  axis_py_extension_t *py_extension =
      (axis_py_extension_t *)axis_binding_handle_get_me_in_target_lang(
          (axis_binding_handle_t *)extension);
  axis_ASSERT(
      py_extension && axis_py_extension_check_integrity(py_extension, true),
      "Invalid argument.");

  PyObject *py_axis_env = py_extension->py_axis_env;
  axis_ASSERT(py_axis_env, "Should not happen.");

  axis_py_data_t *py_data = axis_py_data_wrap(data);

  PyObject *py_res = PyObject_CallMethod(
      (PyObject *)py_extension, "_proxy_on_data", "OO",
      ((axis_py_axis_env_t *)py_axis_env)->actual_py_axis_env, py_data);
  Py_XDECREF(py_res);

  bool err_occurred = axis_py_check_and_clear_py_error();
  axis_ASSERT(!err_occurred, "Should not happen.");

  axis_py_data_invalidate(py_data);

  axis_py_gil_state_release_internal(prev_state);
}

static void proxy_on_audio_frame(axis_extension_t *extension, axis_env_t *axis_env,
                                 axis_shared_ptr_t *audio_frame) {
  axis_ASSERT(extension && axis_extension_check_integrity(extension, true),
             "Invalid argument.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Invalid argument.");
  axis_ASSERT(audio_frame && axis_msg_check_integrity(audio_frame),
             "Invalid argument.");

  // About to call the Python function, so it's necessary to ensure that the GIL
  // has been acquired.
  PyGILState_STATE prev_state = axis_py_gil_state_ensure_internal();

  axis_py_extension_t *py_extension =
      (axis_py_extension_t *)axis_binding_handle_get_me_in_target_lang(
          (axis_binding_handle_t *)extension);
  axis_ASSERT(
      py_extension && axis_py_extension_check_integrity(py_extension, true),
      "Invalid argument.");

  PyObject *py_axis_env = py_extension->py_axis_env;
  axis_ASSERT(py_axis_env, "Should not happen.");

  axis_py_audio_frame_t *py_audio_frame = axis_py_audio_frame_wrap(audio_frame);

  PyObject *py_res = PyObject_CallMethod(
      (PyObject *)py_extension, "_proxy_on_audio_frame", "OO",
      ((axis_py_axis_env_t *)py_axis_env)->actual_py_axis_env, py_audio_frame);
  Py_XDECREF(py_res);

  bool err_occurred = axis_py_check_and_clear_py_error();
  axis_ASSERT(!err_occurred, "Should not happen.");

  axis_py_audio_frame_invalidate(py_audio_frame);

  axis_py_gil_state_release_internal(prev_state);
}

static void proxy_on_video_frame(axis_extension_t *extension, axis_env_t *axis_env,
                                 axis_shared_ptr_t *video_frame) {
  axis_ASSERT(extension && axis_extension_check_integrity(extension, true),
             "Invalid argument.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Invalid argument.");
  axis_ASSERT(video_frame && axis_msg_check_integrity(video_frame),
             "Invalid argument.");

  // About to call the Python function, so it's necessary to ensure that the GIL
  // has been acquired.
  PyGILState_STATE prev_state = axis_py_gil_state_ensure_internal();

  PyObject *py_extension = (PyObject *)axis_binding_handle_get_me_in_target_lang(
      (axis_binding_handle_t *)extension);
  PyObject *py_axis_env = ((axis_py_extension_t *)py_extension)->py_axis_env;
  axis_py_video_frame_t *py_video_frame = axis_py_video_frame_wrap(video_frame);

  PyObject *py_res = PyObject_CallMethod(
      py_extension, "_proxy_on_video_frame", "OO",
      ((axis_py_axis_env_t *)py_axis_env)->actual_py_axis_env, py_video_frame);
  Py_XDECREF(py_res);

  bool err_occurred = axis_py_check_and_clear_py_error();
  axis_ASSERT(!err_occurred, "Should not happen.");

  axis_py_video_frame_invalidate(py_video_frame);

  axis_py_gil_state_release_internal(prev_state);
}

static PyObject *axis_py_extension_create(PyTypeObject *type, PyObject *py_name,
                                         axis_UNUSED PyObject *kwds) {
  axis_py_extension_t *py_extension =
      (axis_py_extension_t *)type->tp_alloc(type, 0);
  if (!py_extension) {
    axis_ASSERT(0, "Failed to allocate Python extension.");

    return axis_py_raise_py_memory_error_exception(
        "Failed to allocate memory for axis_py_extension_t");
  }

  const char *name = NULL;
  if (!PyArg_ParseTuple(py_name, "s", &name)) {
    return axis_py_raise_py_type_error_exception("Invalid argument.");
  }

  axis_signature_set(&py_extension->signature, axis_PY_EXTENSION_SIGNATURE);

  py_extension->c_extension = axis_extension_create(
      name, proxy_on_configure, proxy_on_init, proxy_on_start, proxy_on_stop,
      proxy_on_deinit, proxy_on_cmd, proxy_on_data, proxy_on_audio_frame,
      proxy_on_video_frame, NULL);
  axis_ASSERT(py_extension->c_extension, "Should not happen.");

  axis_binding_handle_set_me_in_target_lang(
      &py_extension->c_extension->binding_handle, py_extension);
  py_extension->py_axis_env = Py_None;

  return (PyObject *)py_extension;
}

static void axis_py_extension_destroy(PyObject *self) {
  axis_py_extension_t *py_extension = (axis_py_extension_t *)self;

  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: In APTIMA world, the destroy operations need to be performed in
  // any threads.
  axis_ASSERT(
      py_extension && axis_py_extension_check_integrity(py_extension, false),
      "Invalid argument.");

  axis_extension_destroy(py_extension->c_extension);
  py_extension->c_extension = NULL;

  Py_TYPE(self)->tp_free(self);
}

PyTypeObject *axis_py_extension_py_type(void) {
  static PyGetSetDef py_extension_type_properties[] = {
      {NULL, NULL, NULL, NULL, NULL}};

  static PyMethodDef py_extension_type_methods[] = {
      {"on_init", stub_on_callback, METH_VARARGS, NULL},
      {"on_start", stub_on_callback, METH_VARARGS, NULL},
      {"on_stop", stub_on_callback, METH_VARARGS, NULL},
      {"on_deinit", stub_on_callback, METH_VARARGS, NULL},
      {"on_cmd", stub_on_callback, METH_VARARGS, NULL},
      {"on_data", stub_on_callback, METH_VARARGS, NULL},
      {"on_audio_frame", stub_on_callback, METH_VARARGS, NULL},
      {"on_video_frame", stub_on_callback, METH_VARARGS, NULL},
      {NULL, NULL, 0, NULL},
  };

  static PyTypeObject py_extension_type = {
      PyVarObject_HEAD_INIT(NULL, 0).tp_name =
          "libaxis_runtime_python._Extension",
      .tp_doc = PyDoc_STR("Extension"),
      .tp_basicsize = sizeof(axis_py_extension_t),
      .tp_itemsize = 0,
      .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
      .tp_new = axis_py_extension_create,
      .tp_init = NULL,
      .tp_dealloc = axis_py_extension_destroy,
      .tp_getset = py_extension_type_properties,
      .tp_methods = py_extension_type_methods,
  };

  return &py_extension_type;
}

bool axis_py_extension_init_for_module(PyObject *module) {
  PyTypeObject *py_type = axis_py_extension_py_type();
  if (PyType_Ready(py_type) < 0) {
    axis_py_raise_py_system_error_exception(
        "Python Extension class is not ready.");

    axis_ASSERT(0, "Should not happen.");
    return false;
  }

  if (PyModule_AddObjectRef(module, "_Extension", (PyObject *)py_type) < 0) {
    axis_py_raise_py_import_error_exception(
        "Failed to add Python type to module.");

    axis_ASSERT(0, "Should not happen.");
    return false;
  }

  return true;
}
