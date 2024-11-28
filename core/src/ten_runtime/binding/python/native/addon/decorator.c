//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/binding/python/addon/decorator.h"

#include "include_internal/ten_runtime/binding/python/addon/addon.h"
#include "include_internal/ten_runtime/binding/python/common/error.h"
#include "ten_runtime/addon/extension/extension.h"
#include "ten_runtime/addon/extension_group/extension_group.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/macro/mark.h"

static PyObject *ten_py_decorator_register_addon_create(
    PyTypeObject *ty, PyObject *args, TEN_UNUSED PyObject *kwds) {
  ten_py_decorator_register_addon_t *py_decorator =
      (ten_py_decorator_register_addon_t *)ty->tp_alloc(ty, 0);
  if (!py_decorator) {
    PyObject *result = ten_py_raise_py_memory_error_exception(
        "Failed to allocate memory for addon decorator.");
    TEN_ASSERT(0, "Failed to allocate memory.");
    return result;
  }

  ten_string_init(&py_decorator->addon_name);
  ten_string_init(&py_decorator->base_dir);

  return (PyObject *)py_decorator;
}

static PyObject *ten_py_decorator_register_addon_create_v2(
    PyTypeObject *ty, PyObject *args, TEN_UNUSED PyObject *kwds) {
  ten_py_decorator_register_addon_v2_t *py_decorator =
      (ten_py_decorator_register_addon_v2_t *)ty->tp_alloc(ty, 0);
  if (!py_decorator) {
    PyObject *result = ten_py_raise_py_memory_error_exception(
        "Failed to allocate memory for addon decorator.");
    TEN_ASSERT(0, "Failed to allocate memory.");
    return result;
  }

  return (PyObject *)py_decorator;
}

static int ten_py_decorator_register_addon_init(PyObject *self, PyObject *args,
                                                TEN_UNUSED PyObject *kwds) {
  ten_py_decorator_register_addon_t *py_decorator =
      (ten_py_decorator_register_addon_t *)self;

  const char *name = NULL;
  const char *base_dir = NULL;

  if (!PyArg_ParseTuple(args, "s|z", &name, &base_dir)) {
    return -1;
  }

  ten_string_set_formatted(&py_decorator->addon_name, "%s", name);

  if (base_dir) {
    ten_string_set_formatted(&py_decorator->base_dir, "%s", base_dir);
  }

  return 0;
}

static int ten_py_decorator_register_addon_init_v2(PyObject *self,
                                                   PyObject *args,
                                                   TEN_UNUSED PyObject *kwds) {
  return 0;
}

static void ten_py_decorator_register_addon_destroy(PyObject *self) {
  ten_py_decorator_register_addon_t *py_decorator =
      (ten_py_decorator_register_addon_t *)self;

  TEN_ASSERT(py_decorator, "Invalid argument.");

  ten_string_deinit(&py_decorator->addon_name);
  ten_string_deinit(&py_decorator->base_dir);

  Py_TYPE(self)->tp_free(self);
}

static void ten_py_decorator_register_addon_destroy_v2(PyObject *self) {
  ten_py_decorator_register_addon_v2_t *py_decorator =
      (ten_py_decorator_register_addon_v2_t *)self;
  TEN_ASSERT(py_decorator, "Invalid argument.");

  Py_TYPE(self)->tp_free(self);
}

static PyObject *ten_py_decorator_register_addon_call(
    ten_py_decorator_register_addon_t *self, PyObject *args,
    ten_addon_register_extension_func_t register_addon_func) {
  PyTypeObject *py_addon_type_object = NULL;

  if (!PyArg_ParseTuple(args, "O", &py_addon_type_object)) {
    return ten_py_raise_py_value_error_exception(
        "Failed to parse argument when registering addon.");
  }

  PyObject *py_addon_object =
      PyObject_CallObject((PyObject *)py_addon_type_object, NULL);
  if (!py_addon_object || ten_py_check_and_clear_py_error()) {
    Py_XDECREF(py_addon_object);  // Ensure cleanup if an error occurred.
    return ten_py_raise_py_value_error_exception(
        "Failed to create Python Addon object.");
  }

  // Validate the returned object is an instance of the expected type.
  if (!PyObject_TypeCheck(py_addon_object, ten_py_addon_py_type())) {
    Py_DECREF(py_addon_object);
    return ten_py_raise_py_type_error_exception(
        "Object is not an instance of Python Addon.");
  }

  // The memory of this c_addon will persist until the Python VM terminates.
  // Since the main of the TEN Python APP is Python, and the TEN world is just a
  // C FFI extension of the Python world, structurally, the Python VM will not
  // terminate until the entire TEN world has concluded. Therefore, this c_addon
  // existing within the Python VM memory space can be safely used within the
  // TEN world.
  ten_py_addon_t *py_addon = (ten_py_addon_t *)py_addon_object;
  ten_addon_host_t *c_addon_host = register_addon_func(
      ten_string_get_raw_str(&self->addon_name),
      ten_string_get_raw_str(&self->base_dir), &py_addon->c_addon);
  TEN_ASSERT(c_addon_host, "Should not happen.");

  py_addon->c_addon_host = c_addon_host;

  return py_addon_object;
}

static PyObject *ten_py_decorator_register_addon_as_extension_call(
    PyObject *self, PyObject *args, TEN_UNUSED PyObject *kwds) {
  return ten_py_decorator_register_addon_call(
      (ten_py_decorator_register_addon_t *)self, args,
      ten_addon_register_extension);
}

static PyObject *ten_py_decorator_register_addon_call_v2(
    ten_py_decorator_register_addon_v2_t *self, PyObject *args,
    ten_addon_register_extension_v2_func_t register_addon_func) {
  const char *name = NULL;
  const char *base_dir = NULL;
  PyObject *py_addon_object = NULL;
  PyObject *py_register_ctx = NULL;

  if (!PyArg_ParseTuple(args, "ssOO", &name, &base_dir, &py_addon_object,
                        &py_register_ctx)) {
    return ten_py_raise_py_value_error_exception(
        "Failed to parse arguments when registering addon.");
  }

  if (!PyObject_TypeCheck(py_addon_object, ten_py_addon_py_type())) {
    return ten_py_raise_py_type_error_exception(
        "Object is not an instance of Python Addon.");
  }

  ten_py_addon_t *py_addon = (ten_py_addon_t *)py_addon_object;

  ten_addon_host_t *c_addon_host =
      register_addon_func(name, base_dir, &py_addon->c_addon, NULL);
  if (!c_addon_host) {
    return ten_py_raise_py_value_error_exception(
        "Failed to register addon in register_addon_func.");
  }

  py_addon->c_addon_host = c_addon_host;

  Py_INCREF(py_addon_object);  // Ensure the object is kept alive.

  return py_addon_object;
}

static PyObject *ten_py_decorator_register_addon_as_extension_call_v2(
    PyObject *self, PyObject *args, TEN_UNUSED PyObject *kwds) {
  return ten_py_decorator_register_addon_call_v2(
      (ten_py_decorator_register_addon_v2_t *)self, args,
      ten_addon_register_extension_v2);
}

static PyTypeObject *ten_py_decorator_register_addon_as_extension_py_type(
    void) {
  static PyMethodDef decorator_methods[] = {
      {NULL, NULL, 0, NULL},
  };

  static PyTypeObject py_type = {
      PyVarObject_HEAD_INIT(NULL, 0).tp_name =
          "libten_runtime_python.register_addon_as_extension",
      .tp_doc = PyDoc_STR("register_addon_as_extension"),
      .tp_basicsize = sizeof(ten_py_decorator_register_addon_t),
      .tp_itemsize = 0,
      .tp_flags = Py_TPFLAGS_DEFAULT,
      .tp_new = ten_py_decorator_register_addon_create,
      .tp_init = ten_py_decorator_register_addon_init,
      .tp_dealloc = ten_py_decorator_register_addon_destroy,
      .tp_call = ten_py_decorator_register_addon_as_extension_call,
      .tp_getset = NULL,
      .tp_methods = decorator_methods,
  };

  return &py_type;
}

static PyTypeObject *ten_py_decorator_register_addon_as_extension_py_type_v2(
    void) {
  static PyMethodDef decorator_methods[] = {
      {NULL, NULL, 0, NULL},
  };

  static PyTypeObject py_type = {
      PyVarObject_HEAD_INIT(NULL, 0).tp_name =
          "libten_runtime_python.register_addon_as_extension_v2",
      .tp_doc = PyDoc_STR("register_addon_as_extension_v2"),
      .tp_basicsize = sizeof(ten_py_decorator_register_addon_t),
      .tp_itemsize = 0,
      .tp_flags = Py_TPFLAGS_DEFAULT,
      .tp_new = ten_py_decorator_register_addon_create_v2,
      .tp_init = ten_py_decorator_register_addon_init_v2,
      .tp_dealloc = ten_py_decorator_register_addon_destroy_v2,
      .tp_call = ten_py_decorator_register_addon_as_extension_call_v2,
      .tp_getset = NULL,
      .tp_methods = decorator_methods,
  };

  return &py_type;
}

static bool ten_py_decorator_register_addon_module_init(PyObject *module,
                                                        PyTypeObject *py_type,
                                                        const char *name) {
  if (PyType_Ready(py_type) < 0) {
    ten_py_raise_py_system_error_exception(
        "Failed to ready Python type for decorator.");
    TEN_ASSERT(0, "Should not happen.");
    return false;
  }

  if (PyModule_AddObjectRef(module, name, (PyObject *)py_type) < 0) {
    ten_py_raise_py_import_error_exception(
        "Failed to add Python decorator type to module.");
    return false;
  }

  return true;
}

bool ten_py_decorator_register_addon_as_extension_init_for_module(
    PyObject *module) {
  return ten_py_decorator_register_addon_module_init(
      module, ten_py_decorator_register_addon_as_extension_py_type(),
      "_register_addon_as_extension");
}

bool ten_py_decorator_register_addon_as_extension_init_for_module_v2(
    PyObject *module) {
  return ten_py_decorator_register_addon_module_init(
      module, ten_py_decorator_register_addon_as_extension_py_type_v2(),
      "_register_addon_as_extension_v2");
}
