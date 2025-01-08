//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/binding/python/common/buf.h"

#include "include_internal/axis_runtime/binding/python/common/error.h"
#include "object.h"

axis_py_buf_t *axis_py_buf_wrap(axis_buf_t *buf) {
  if (!buf) {
    return NULL;
  }

  axis_py_buf_t *py_buf =
      (axis_py_buf_t *)axis_py_buf_py_type()->tp_alloc(axis_py_buf_py_type(), 0);
  if (!py_buf) {
    return NULL;
  }

  py_buf->c_buf = buf;

  return py_buf;
}

void axis_py_buf_destroy(PyObject *self) {
  axis_py_buf_t *py_buf = (axis_py_buf_t *)self;
  if (!py_buf) {
    return;
  }

  Py_TYPE(self)->tp_free(self);
}

int axis_py_buf_get_buffer(PyObject *self, Py_buffer *view, int flags) {
  axis_py_buf_t *py_buf = (axis_py_buf_t *)self;
  if (!py_buf || !py_buf->c_buf) {
    return -1;
  }

  axis_buf_t *c_buf = py_buf->c_buf;

  Py_INCREF(self);

  view->format = "B";
  view->buf = c_buf->data;
  view->obj = self;
  view->len = (Py_ssize_t)c_buf->size;
  view->readonly = false;
  view->itemsize = 1;
  view->ndim = 1;
  view->shape = &view->len;
  view->strides = &view->itemsize;

  return 0;
}

bool axis_py_buf_init_for_module(PyObject *module) {
  PyTypeObject *py_type = axis_py_buf_py_type();
  if (PyType_Ready(py_type) < 0) {
    axis_py_raise_py_system_error_exception(
        "Python AudioFrame class is not ready.");
    return false;
  }

  if (PyModule_AddObjectRef(module, "_Buf", (PyObject *)py_type) < 0) {
    axis_py_raise_py_import_error_exception(
        "Failed to add Python type to module.");
    return false;
  }
  return true;
}