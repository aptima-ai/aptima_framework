//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/binding/python/msg/data.h"

#include "include_internal/axis_runtime/binding/python/common/error.h"
#include "include_internal/axis_runtime/binding/python/msg/msg.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "pyport.h"
#include "axis_runtime/msg/data/data.h"
#include "axis_runtime/msg/msg.h"
#include "axis_utils/lib/buf.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"

static axis_py_data_t *axis_py_data_create_internal(PyTypeObject *py_type) {
  if (!py_type) {
    py_type = axis_py_data_py_type();
  }

  axis_py_data_t *py_data = (axis_py_data_t *)py_type->tp_alloc(py_type, 0);

  axis_signature_set(&py_data->msg.signature, axis_PY_MSG_SIGNATURE);
  py_data->msg.c_msg = NULL;

  return py_data;
}

static axis_py_data_t *axis_py_data_init(axis_py_data_t *py_data,
                                       const char *name) {
  axis_ASSERT(py_data && axis_py_msg_check_integrity((axis_py_msg_t *)py_data),
             "Invalid argument.");

  py_data->msg.c_msg = axis_data_create(name, NULL);

  return py_data;
}

PyObject *axis_py_data_create(PyTypeObject *type, PyObject *args,
                             axis_UNUSED PyObject *kwds) {
  const char *name = NULL;
  if (!PyArg_ParseTuple(args, "s", &name)) {
    return axis_py_raise_py_value_error_exception("Failed to parse arguments.");
  }

  axis_py_data_t *py_data = axis_py_data_create_internal(type);
  return (PyObject *)axis_py_data_init(py_data, name);
}

void axis_py_data_destroy(PyObject *self) {
  axis_py_data_t *py_data = (axis_py_data_t *)self;
  axis_ASSERT(py_data && axis_py_msg_check_integrity((axis_py_msg_t *)py_data),
             "Invalid argument.");

  axis_py_msg_destroy_c_msg(&py_data->msg);
  Py_TYPE(self)->tp_free(self);
}

PyObject *axis_py_data_alloc_buf(PyObject *self, PyObject *args) {
  axis_py_data_t *py_data = (axis_py_data_t *)self;
  axis_ASSERT(py_data && axis_py_msg_check_integrity((axis_py_msg_t *)py_data),
             "Invalid argument.");

  int size = 0;
  if (!PyArg_ParseTuple(args, "i", &size)) {
    return axis_py_raise_py_value_error_exception("Invalid buffer size.");
  }

  if (size <= 0) {
    return axis_py_raise_py_value_error_exception("Invalid buffer size.");
  }

  axis_data_alloc_buf(py_data->msg.c_msg, size);

  Py_RETURN_NONE;
}

PyObject *axis_py_data_lock_buf(PyObject *self, PyObject *args) {
  axis_py_data_t *py_data = (axis_py_data_t *)self;
  axis_ASSERT(py_data && axis_py_msg_check_integrity((axis_py_msg_t *)py_data),
             "Invalid argument.");

  axis_error_t err;
  axis_error_init(&err);

  if (!axis_msg_add_locked_res_buf(py_data->msg.c_msg,
                                  axis_data_peek_buf(py_data->msg.c_msg)->data,
                                  &err)) {
    return axis_py_raise_py_system_error_exception(
        "Failed to lock buffer in data.");
  }

  axis_buf_t *result = axis_data_peek_buf(py_data->msg.c_msg);

  return PyMemoryView_FromMemory((char *)result->data, (Py_ssize_t)result->size,
                                 PyBUF_WRITE);
}

PyObject *axis_py_data_unlock_buf(PyObject *self, PyObject *args) {
  axis_py_data_t *py_data = (axis_py_data_t *)self;
  axis_ASSERT(py_data && axis_py_msg_check_integrity((axis_py_msg_t *)py_data),
             "Invalid argument.");

  Py_buffer py_buf;
  if (!PyArg_ParseTuple(args, "y*", &py_buf)) {
    return axis_py_raise_py_value_error_exception("Invalid buffer.");
  }

  Py_ssize_t size = 0;
  uint8_t *data = py_buf.buf;
  if (!data) {
    return axis_py_raise_py_value_error_exception("Invalid buffer.");
  }

  size = py_buf.len;
  if (size <= 0) {
    return axis_py_raise_py_value_error_exception("Invalid buffer size.");
  }

  axis_error_t err;
  axis_error_init(&err);

  if (!axis_msg_remove_locked_res_buf(py_data->msg.c_msg, data, &err)) {
    return axis_py_raise_py_system_error_exception(
        "Failed to unlock buffer in data.");
  }

  Py_RETURN_NONE;
}

PyObject *axis_py_data_get_buf(PyObject *self, PyObject *args) {
  axis_py_data_t *py_data = (axis_py_data_t *)self;
  axis_ASSERT(py_data && axis_py_msg_check_integrity((axis_py_msg_t *)py_data),
             "Invalid argument.");

  axis_error_t err;
  axis_error_init(&err);

  axis_buf_t *buf = axis_data_peek_buf(py_data->msg.c_msg);
  if (!buf) {
    return axis_py_raise_py_system_error_exception("Failed to get buffer.");
  }

  size_t data_size = buf->size;

  return PyByteArray_FromStringAndSize((const char *)buf->data, data_size);
}

axis_py_data_t *axis_py_data_wrap(axis_shared_ptr_t *data) {
  axis_ASSERT(data && axis_msg_check_integrity(data), "Invalid argument.");

  axis_py_data_t *py_data = axis_py_data_create_internal(NULL);
  py_data->msg.c_msg = axis_shared_ptr_clone(data);
  return py_data;
}

void axis_py_data_invalidate(axis_py_data_t *self) {
  axis_ASSERT(self, "Invalid argument");
  Py_DECREF(self);
}

bool axis_py_data_init_for_module(PyObject *module) {
  PyTypeObject *py_type = axis_py_data_py_type();
  if (PyType_Ready(py_type) < 0) {
    axis_py_raise_py_system_error_exception("Python Data class is not ready.");

    axis_ASSERT(0, "Should not happen.");
    return false;
  }

  if (PyModule_AddObjectRef(module, "_Data", (PyObject *)py_type) < 0) {
    axis_py_raise_py_import_error_exception(
        "Failed to add Python type to module.");

    axis_ASSERT(0, "Should not happen.");
    return false;
  }
  return true;
}
