//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_runtime/addon/addon.h"

#include "include_internal/axis_runtime/addon/addon.h"
#include "include_internal/axis_runtime/addon/addon_host.h"
#include "include_internal/axis_runtime/binding/common.h"
#include "include_internal/axis_runtime/binding/python/addon/addon.h"
#include "include_internal/axis_runtime/binding/python/common/common.h"
#include "include_internal/axis_runtime/binding/python/common/error.h"
#include "include_internal/axis_runtime/binding/python/axis_env/axis_env.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension_group/extension_group.h"
#include "axis_runtime/addon/extension/extension.h"
#include "axis_runtime/binding/common.h"
#include "axis_runtime/axis_env/internal/on_xxx_done.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/mark.h"

static bool axis_py_addon_check_integrity(axis_py_addon_t *self) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_PY_ADDON_SIGNATURE) {
    return false;
  }

  return true;
}

static PyObject *not_implemented_base_on_init(axis_UNUSED PyObject *self,
                                              axis_UNUSED PyObject *args) {
  return axis_py_raise_py_not_implemented_error_exception(
      "The method 'on_init' must be implemented in the subclass of 'Addon'.");
}

static PyObject *not_implemented_base_on_deinit(axis_UNUSED PyObject *self,
                                                axis_UNUSED PyObject *args) {
  return axis_py_raise_py_not_implemented_error_exception(
      "The method 'on_deinit' must be implemented in the subclass of 'Addon'.");
}

static PyObject *not_implemented_base_on_create_instance(
    axis_UNUSED PyObject *self, axis_UNUSED PyObject *args) {
  return axis_py_raise_py_not_implemented_error_exception(
      "The method 'on_create_instance' must be implemented in the subclass of "
      "'Addon'.");
}

static void proxy_on_init(axis_addon_t *addon, axis_env_t *axis_env) {
  axis_ASSERT(addon, "Invalid argument.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Invalid argument.");

  // About to call the Python function, so it's necessary to ensure that the GIL
  // has been acquired.
  PyGILState_STATE prev_state = axis_py_gil_state_ensure_internal();

  axis_py_axis_env_t *py_axis_env = axis_py_axis_env_wrap(axis_env);
  if (!py_axis_env) {
    axis_ASSERT(0, "Failed to wrap ten.");
    goto done;
  }

  axis_py_addon_t *py_addon = addon->binding_handle.me_in_target_lang;
  if (!py_addon) {
    Py_DECREF(py_axis_env);

    axis_ASSERT(0, "Invalid addon in target language.");
    goto done;
  }
  axis_ASSERT(axis_py_addon_check_integrity(py_addon), "Should not happen.");

  PyObject *py_res = PyObject_CallMethod((PyObject *)py_addon, "on_init", "O",
                                         py_axis_env->actual_py_axis_env);
  if (!py_res) {
    axis_py_check_and_clear_py_error();

    Py_DECREF(py_axis_env);

    axis_ASSERT(0, "Python method on_init failed.");
    goto done;
  }

  Py_XDECREF(py_res);

done:
  axis_py_gil_state_release_internal(prev_state);
}

static void proxy_on_deinit(axis_addon_t *addon, axis_env_t *axis_env) {
  axis_ASSERT(addon, "Invalid argument.");
  // TODO(Wei): In the context of Python standalone tests, the Python addon is
  // registered into the TEN world within the extension tester thread (i.e., the
  // Python thread) but is unregistered in the test app thread. It should be
  // modified to also perform the Python addon registration within the test
  // app's `on_configure_done`. This change will allow the underlying thread
  // check to be set to `true`.
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, false),
             "Invalid argument.");

  // About to call the Python function, so it's necessary to ensure that the GIL
  // has been acquired.
  PyGILState_STATE prev_state = axis_py_gil_state_ensure_internal();

  axis_py_axis_env_t *py_axis_env = axis_py_axis_env_wrap(axis_env);
  if (!py_axis_env) {
    axis_ASSERT(0, "Failed to wrap ten.");
    goto done;
  }

  axis_py_addon_t *py_addon = addon->binding_handle.me_in_target_lang;
  if (!py_addon) {
    Py_DECREF(py_axis_env);

    axis_ASSERT(0, "Invalid addon in target language.");
    goto done;
  }
  axis_ASSERT(axis_py_addon_check_integrity(py_addon), "Should not happen.");

  PyObject *py_res = PyObject_CallMethod((PyObject *)py_addon, "on_deinit", "O",
                                         py_axis_env->actual_py_axis_env);
  if (!py_res) {
    axis_py_check_and_clear_py_error();

    Py_DECREF(py_axis_env);

    axis_ASSERT(0, "Python method on_deinit failed.");
    goto done;
  }

  Py_XDECREF(py_res);

done:
  axis_py_gil_state_release_internal(prev_state);
}

static void proxy_on_create_instance_async(axis_addon_t *addon,
                                           axis_env_t *axis_env, const char *name,
                                           void *context) {
  axis_ASSERT(addon, "Invalid argument.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, false),
             "Invalid argument.");
  axis_ASSERT(name && strlen(name), "Invalid argument.");

  axis_py_addon_t *py_addon = addon->binding_handle.me_in_target_lang;
  axis_ASSERT(py_addon && axis_py_addon_check_integrity(py_addon),
             "Should not happen.");

  // About to call the Python function, so it's necessary to ensure that the GIL
  // has been acquired.
  PyGILState_STATE prev_state = axis_py_gil_state_ensure_internal();

  if (!py_addon || !name || !strlen(name)) {
    axis_py_raise_py_value_error_exception(
        "Invalid argument when creating instance.");

    axis_ASSERT(0, "Should not happen.");
    goto done;
  }

  axis_py_axis_env_t *py_axis_env = axis_py_axis_env_wrap(axis_env);
  if (!py_axis_env) {
    axis_ASSERT(0, "Failed to wrap ten.");
    goto done;
  }

  PyObject *py_res =
      PyObject_CallMethod((PyObject *)py_addon, "on_create_instance", "Osl",
                          py_axis_env->actual_py_axis_env, name, context);
  if (!py_res) {
    axis_py_check_and_clear_py_error();

    Py_DECREF(py_axis_env);

    axis_ASSERT(0, "Python method on_create_instance failed.");
    goto done;
  }

  Py_XDECREF(py_res);

done:
  axis_py_gil_state_release_internal(prev_state);
}

static void proxy_on_destroy_instance_async(axis_addon_t *addon,
                                            axis_env_t *axis_env, void *instance,
                                            void *context) {
  axis_ASSERT(addon, "Invalid argument.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, false),
             "Invalid argument.");
  axis_ASSERT(instance, "Invalid argument.");

  PyObject *py_instance = NULL;
  axis_py_addon_t *py_addon =
      (axis_py_addon_t *)addon->binding_handle.me_in_target_lang;
  axis_ASSERT(py_addon && axis_py_addon_check_integrity(py_addon),
             "Should not happen.");

  // About to call the Python function, so it's necessary to ensure that the GIL
  // has been acquired.
  PyGILState_STATE prev_state = axis_py_gil_state_ensure_internal();

  switch (py_addon->type) {
    case axis_ADDON_TYPE_EXTENSION:
    case axis_ADDON_TYPE_EXTENSION_GROUP:
      py_instance = axis_binding_handle_get_me_in_target_lang(
          (axis_binding_handle_t *)instance);
      break;

    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }

  axis_ASSERT(py_instance, "Failed to get Python instance.");

  // Decrement the reference count of the Python extension and its associated
  // ten object, so that Python GC can reclaim them.
  Py_DECREF(py_instance);

  axis_py_gil_state_release_internal(prev_state);

  axis_env_on_destroy_instance_done(axis_env, context, NULL);
}

static PyObject *axis_py_addon_create(PyTypeObject *type,
                                     axis_UNUSED PyObject *args,
                                     axis_UNUSED PyObject *kwds) {
  axis_py_addon_t *py_addon = (axis_py_addon_t *)type->tp_alloc(type, 0);
  if (!py_addon) {
    return axis_py_raise_py_memory_error_exception(
        "Failed to allocate memory for axis_py_addon_t");
  }

  axis_signature_set(&py_addon->signature, axis_PY_ADDON_SIGNATURE);
  py_addon->type = axis_ADDON_TYPE_EXTENSION;  // Now we only support extension.
  py_addon->c_addon_host = NULL;

  axis_addon_init(&py_addon->c_addon, proxy_on_init, proxy_on_deinit,
                 proxy_on_create_instance_async,
                 proxy_on_destroy_instance_async, NULL);

  axis_binding_handle_set_me_in_target_lang(
      (axis_binding_handle_t *)&py_addon->c_addon, py_addon);

  return (PyObject *)py_addon;
}

static void axis_py_addon_destroy(PyObject *self) {
  axis_py_addon_t *py_addon = (axis_py_addon_t *)self;
  axis_ASSERT(py_addon && axis_py_addon_check_integrity(py_addon),
             "Invalid argument.");

  axis_LOGI("[%s] destroy addon host for python addon.",
           axis_string_get_raw_str(&py_addon->c_addon_host->name));

  Py_TYPE(self)->tp_free(self);
}

PyTypeObject *axis_py_addon_py_type(void) {
  static PyMethodDef addon_methods[] = {
      {"on_init", not_implemented_base_on_init, METH_VARARGS,
       "Override to initialize."},
      {"on_deinit", not_implemented_base_on_deinit, METH_VARARGS,
       "Override to de-initialize."},
      {"on_create_instance", not_implemented_base_on_create_instance,
       METH_VARARGS, "Override to create your own instance."},
      {NULL, NULL, 0, NULL},
  };

  static PyTypeObject py_type = {
      PyVarObject_HEAD_INIT(NULL, 0).tp_name = "libaxis_runtime_python._Addon",
      .tp_doc = PyDoc_STR("_Addon"),
      .tp_basicsize = sizeof(axis_py_addon_t),
      .tp_itemsize = 0,
      .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
      .tp_new = axis_py_addon_create,
      .tp_init = NULL,
      .tp_dealloc = axis_py_addon_destroy,
      .tp_getset = NULL,
      .tp_methods = addon_methods,
  };

  return &py_type;
}

bool axis_py_addon_init_for_module(PyObject *module) {
  PyTypeObject *py_type = axis_py_addon_py_type();
  if (PyType_Ready(py_type) < 0) {
    axis_py_raise_py_system_error_exception("Python Addon class is not ready.");
    axis_ASSERT(0, "Should not happen.");
    return false;
  }

  if (PyModule_AddObjectRef(module, "_Addon", (PyObject *)py_type) < 0) {
    axis_py_raise_py_import_error_exception(
        "Failed to add Python type to module.");
    return false;
  }

  return true;
}
