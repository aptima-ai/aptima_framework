//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/axis_env/axis_env.h"

#include <string.h>

#include "include_internal/axis_runtime/binding/python/common/error.h"
#include "include_internal/axis_runtime/binding/python/axis_env/axis_env.h"
#include "object.h"
#include "axis_runtime/binding/common.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/macro/check.h"

bool axis_py_axis_env_check_integrity(axis_py_axis_env_t *self) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_PY_axis_ENV_SIGNATURE) {
    return false;
  }

  return true;
}

static void axis_py_axis_env_c_part_destroyed(void *axis_env_bridge_) {
  axis_py_axis_env_t *axis_env_bridge = (axis_py_axis_env_t *)axis_env_bridge_;

  axis_ASSERT(axis_env_bridge && axis_py_axis_env_check_integrity(axis_env_bridge),
             "Should not happen.");

  axis_env_bridge->c_axis_env = NULL;
  axis_py_axis_env_invalidate(axis_env_bridge);
}

static PyObject *create_actual_py_axis_env_instance(
    axis_py_axis_env_t *py_axis_env) {
  // Import the Python module where TenEnv is defined.
  PyObject *module_name = PyUnicode_FromString("aptima.axis_env");
  PyObject *module = PyImport_Import(module_name);
  Py_DECREF(module_name);

  if (!module) {
    PyErr_Print();
    return NULL;
  }

  // Get the TenEnv class from the module.
  PyObject *axis_env_class = PyObject_GetAttrString(module, "TenEnv");
  Py_DECREF(module);

  if (!axis_env_class || !PyCallable_Check(axis_env_class)) {
    PyErr_Print();
    Py_XDECREF(axis_env_class);
    return NULL;
  }

  // Create the argument tuple with the _TenEnv object
  PyObject *args = PyTuple_Pack(1, py_axis_env);

  // Create an instance of the TenEnv class.
  PyObject *axis_env_instance = PyObject_CallObject(axis_env_class, args);
  Py_DECREF(axis_env_class);
  Py_DECREF(args);

  if (!axis_env_instance) {
    PyErr_Print();
    return NULL;
  }

  return axis_env_instance;
}

axis_py_axis_env_t *axis_py_axis_env_wrap(axis_env_t *axis_env) {
  axis_ASSERT(axis_env, "Invalid argument.");

  axis_py_axis_env_t *py_axis_env =
      axis_binding_handle_get_me_in_target_lang((axis_binding_handle_t *)axis_env);
  if (py_axis_env) {
    return py_axis_env;
  }

  PyTypeObject *py_axis_env_py_type = axis_py_axis_env_type();

  // Create a new py_axis_env.
  py_axis_env =
      (axis_py_axis_env_t *)py_axis_env_py_type->tp_alloc(py_axis_env_py_type, 0);
  axis_ASSERT(py_axis_env, "Failed to allocate memory.");

  axis_signature_set(&py_axis_env->signature, axis_PY_axis_ENV_SIGNATURE);
  py_axis_env->c_axis_env = axis_env;
  py_axis_env->c_axis_env_proxy = NULL;
  py_axis_env->need_to_release_gil_state = false;
  py_axis_env->py_thread_state = NULL;

  py_axis_env->actual_py_axis_env = create_actual_py_axis_env_instance(py_axis_env);
  if (!py_axis_env->actual_py_axis_env) {
    axis_ASSERT(0, "Should not happen.");
    Py_DECREF(py_axis_env);
    return NULL;
  }

  axis_binding_handle_set_me_in_target_lang((axis_binding_handle_t *)axis_env,
                                           py_axis_env);
  axis_env_set_destroy_handler_in_target_lang(axis_env,
                                             axis_py_axis_env_c_part_destroyed);

  return py_axis_env;
}

static PyObject *axis_py_axis_env_get_attach_to(axis_py_axis_env_t *self,
                                              void *closure) {
  return PyLong_FromLong(self->c_axis_env->attach_to);
}

void axis_py_axis_env_invalidate(axis_py_axis_env_t *py_axis_env) {
  axis_ASSERT(py_axis_env, "Should not happen.");

  if (py_axis_env->actual_py_axis_env) {
    Py_DECREF(py_axis_env->actual_py_axis_env);
    py_axis_env->actual_py_axis_env = NULL;
  }

  Py_DECREF(py_axis_env);
}

static void axis_py_axis_env_destroy(PyObject *self) {
  Py_TYPE(self)->tp_free(self);
}

PyTypeObject *axis_py_axis_env_type(void) {
  static PyGetSetDef axis_getset[] = {
      {"_attach_to", (getter)axis_py_axis_env_get_attach_to, NULL, NULL, NULL},
      {NULL, NULL, NULL, NULL, NULL}};

  static PyMethodDef axis_methods[] = {
      {"on_configure_done", axis_py_axis_env_on_configure_done, METH_VARARGS,
       NULL},
      {"on_init_done", axis_py_axis_env_on_init_done, METH_VARARGS, NULL},
      {"on_start_done", axis_py_axis_env_on_start_done, METH_VARARGS, NULL},
      {"on_stop_done", axis_py_axis_env_on_stop_done, METH_VARARGS, NULL},
      {"on_deinit_done", axis_py_axis_env_on_deinit_done, METH_VARARGS, NULL},
      {"on_create_instance_done", axis_py_axis_env_on_create_instance_done,
       METH_VARARGS, NULL},
      {"send_cmd", axis_py_axis_env_send_cmd, METH_VARARGS, NULL},
      {"send_data", axis_py_axis_env_send_data, METH_VARARGS, NULL},
      {"send_video_frame", axis_py_axis_env_send_video_frame, METH_VARARGS, NULL},
      {"send_audio_frame", axis_py_axis_env_send_audio_frame, METH_VARARGS, NULL},
      {"get_property_to_json", axis_py_axis_env_get_property_to_json,
       METH_VARARGS, NULL},
      {"set_property_from_json", axis_py_axis_env_set_property_from_json,
       METH_VARARGS, NULL},
      {"return_result", axis_py_axis_env_return_result, METH_VARARGS, NULL},
      {"return_result_directly", axis_py_axis_env_return_result_directly,
       METH_VARARGS, NULL},
      {"get_property_int", axis_py_axis_env_get_property_int, METH_VARARGS, NULL},
      {"set_property_int", axis_py_axis_env_set_property_int, METH_VARARGS, NULL},
      {"get_property_string", axis_py_axis_env_get_property_string, METH_VARARGS,
       NULL},
      {"set_property_string", axis_py_axis_env_set_property_string, METH_VARARGS,
       NULL},
      {"get_property_bool", axis_py_axis_env_get_property_bool, METH_VARARGS,
       NULL},
      {"set_property_bool", axis_py_axis_env_set_property_bool, METH_VARARGS,
       NULL},
      {"get_property_float", axis_py_axis_env_get_property_float, METH_VARARGS,
       NULL},
      {"set_property_float", axis_py_axis_env_set_property_float, METH_VARARGS,
       NULL},
      {"is_property_exist", axis_py_axis_env_is_property_exist, METH_VARARGS,
       NULL},
      {"init_property_from_json", axis_py_axis_env_init_property_from_json,
       METH_VARARGS, NULL},
      {"get_property_to_json_async", axis_py_axis_env_get_property_to_json_async,
       METH_VARARGS, NULL},
      {"set_property_from_json_async",
       axis_py_axis_env_set_property_from_json_async, METH_VARARGS, NULL},
      {"get_property_int_async", axis_py_axis_env_get_property_int_async,
       METH_VARARGS, NULL},
      {"set_property_int_async", axis_py_axis_env_set_property_int_async,
       METH_VARARGS, NULL},
      {"get_property_string_async", axis_py_axis_env_get_property_string_async,
       METH_VARARGS, NULL},
      {"set_property_string_async", axis_py_axis_env_set_property_string_async,
       METH_VARARGS, NULL},
      {"get_property_bool_async", axis_py_axis_env_get_property_bool_async,
       METH_VARARGS, NULL},
      {"set_property_bool_async", axis_py_axis_env_set_property_bool_async,
       METH_VARARGS, NULL},
      {"get_property_float_async", axis_py_axis_env_get_property_float_async,
       METH_VARARGS, NULL},
      {"set_property_float_async", axis_py_axis_env_set_property_float_async,
       METH_VARARGS, NULL},
      {"is_property_exist_async", axis_py_axis_env_is_property_exist_async,
       METH_VARARGS, NULL},
      {"init_property_from_json_async",
       axis_py_axis_env_init_property_from_json_async, METH_VARARGS, NULL},
      {"log", axis_py_axis_env_log, METH_VARARGS, NULL},
      {NULL, NULL, 0, NULL},
  };

  static PyTypeObject ty = {
      PyVarObject_HEAD_INIT(NULL, 0).tp_name = "libaxis_runtime_python._TenEnv",
      .tp_doc = PyDoc_STR("_TenEnv"),
      .tp_basicsize = sizeof(axis_py_axis_env_t),
      .tp_itemsize = 0,
      .tp_flags = Py_TPFLAGS_DEFAULT,

      // The metadata info will be created by the Python binding and not by the
      // user within the Python environment.
      .tp_new = NULL,

      .tp_init = NULL,
      .tp_dealloc = axis_py_axis_env_destroy,
      .tp_getset = axis_getset,
      .tp_methods = axis_methods,
  };
  return &ty;
}

bool axis_py_axis_env_init_for_module(PyObject *module) {
  PyTypeObject *py_type = axis_py_axis_env_type();
  if (PyType_Ready(axis_py_axis_env_type()) < 0) {
    axis_py_raise_py_system_error_exception("Python TenEnv class is not ready.");
    axis_ASSERT(0, "Should not happen.");
    return false;
  }

  if (PyModule_AddObjectRef(module, "_TenEnv", (PyObject *)py_type) < 0) {
    axis_py_raise_py_import_error_exception(
        "Failed to add Python type to module.");
    return false;
  }

  return true;
}
