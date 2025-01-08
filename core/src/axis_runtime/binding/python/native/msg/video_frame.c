//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/binding/python/msg/video_frame.h"

#include "include_internal/axis_runtime/binding/python/common/error.h"
#include "include_internal/axis_runtime/binding/python/msg/msg.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "memoryobject.h"
#include "axis_runtime/msg/video_frame/video_frame.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"

static axis_py_video_frame_t *axis_py_video_frame_create_internal(
    PyTypeObject *py_type) {
  if (!py_type) {
    py_type = axis_py_video_frame_py_type();
  }

  axis_py_video_frame_t *py_video_frame =
      (axis_py_video_frame_t *)py_type->tp_alloc(py_type, 0);

  axis_signature_set(&py_video_frame->msg.signature, axis_PY_MSG_SIGNATURE);
  py_video_frame->msg.c_msg = NULL;

  return py_video_frame;
}

static axis_py_video_frame_t *axis_py_video_frame_init(
    axis_py_video_frame_t *py_video_frame, const char *name) {
  axis_ASSERT(py_video_frame &&
                 axis_py_msg_check_integrity((axis_py_msg_t *)py_video_frame),
             "Invalid argument.");

  py_video_frame->msg.c_msg = axis_video_frame_create(name, NULL);

  return py_video_frame;
}

PyObject *axis_py_video_frame_create(PyTypeObject *type, PyObject *args,
                                    axis_UNUSED PyObject *kwds) {
  const char *name = NULL;
  if (!PyArg_ParseTuple(args, "s", &name)) {
    return axis_py_raise_py_value_error_exception("Failed to parse arguments.");
  }

  axis_py_video_frame_t *py_video_frame =
      axis_py_video_frame_create_internal(type);
  return (PyObject *)axis_py_video_frame_init(py_video_frame, name);
}

void axis_py_video_frame_destroy(PyObject *self) {
  axis_py_video_frame_t *py_video_frame = (axis_py_video_frame_t *)self;
  axis_ASSERT(py_video_frame &&
                 axis_py_msg_check_integrity((axis_py_msg_t *)py_video_frame),
             "Invalid argument.");

  axis_py_msg_destroy_c_msg(&py_video_frame->msg);
  Py_TYPE(self)->tp_free(self);
}

axis_py_video_frame_t *axis_py_video_frame_wrap(axis_shared_ptr_t *video_frame) {
  axis_ASSERT(video_frame && axis_msg_check_integrity(video_frame),
             "Invalid argument.");

  axis_py_video_frame_t *py_video_frame =
      axis_py_video_frame_create_internal(NULL);
  py_video_frame->msg.c_msg = axis_shared_ptr_clone(video_frame);
  return py_video_frame;
}

void axis_py_video_frame_invalidate(axis_py_video_frame_t *self) {
  axis_ASSERT(self, "Invalid argument");
  Py_DECREF(self);
}

PyObject *axis_py_video_frame_alloc_buf(PyObject *self, PyObject *args) {
  axis_py_video_frame_t *py_video_frame = (axis_py_video_frame_t *)self;
  axis_ASSERT(py_video_frame &&
                 axis_py_msg_check_integrity((axis_py_msg_t *)py_video_frame),
             "Invalid argument.");

  int size = 0;
  if (!PyArg_ParseTuple(args, "i", &size)) {
    axis_ASSERT(0, "Should not happen.");
    return axis_py_raise_py_value_error_exception("Invalid video frame size.");
  }

  if (size <= 0) {
    axis_ASSERT(0, "Should not happen.");
    return axis_py_raise_py_value_error_exception("Invalid video frame size.");
  }

  axis_video_frame_alloc_data(py_video_frame->msg.c_msg, size);

  Py_RETURN_NONE;
}

PyObject *axis_py_video_frame_lock_buf(PyObject *self, PyObject *args) {
  axis_py_video_frame_t *py_video_frame = (axis_py_video_frame_t *)self;
  axis_ASSERT(py_video_frame &&
                 axis_py_msg_check_integrity((axis_py_msg_t *)py_video_frame),
             "Invalid argument.");

  axis_error_t err;
  axis_error_init(&err);

  axis_buf_t *data = axis_video_frame_peek_buf(py_video_frame->msg.c_msg);

  if (!axis_msg_add_locked_res_buf(py_video_frame->msg.c_msg, data->data,
                                  &err)) {
    axis_ASSERT(0, "Should not happen.");
    return axis_py_raise_py_system_error_exception(
        "Failed to lock buffer in video frame.");
  }

  return PyMemoryView_FromMemory((char *)data->data, (Py_ssize_t)data->size,
                                 PyBUF_WRITE);
}

PyObject *axis_py_video_frame_unlock_buf(PyObject *self, PyObject *args) {
  axis_py_video_frame_t *py_video_frame = (axis_py_video_frame_t *)self;
  axis_ASSERT(py_video_frame &&
                 axis_py_msg_check_integrity((axis_py_msg_t *)py_video_frame),
             "Invalid argument.");

  Py_buffer py_buf;
  if (!PyArg_ParseTuple(args, "y*", &py_buf)) {
    axis_ASSERT(0, "Should not happen.");
    return axis_py_raise_py_value_error_exception("Invalid buffer.");
  }

  Py_ssize_t size = 0;
  uint8_t *data = py_buf.buf;
  if (!data) {
    axis_ASSERT(0, "Should not happen.");
    return axis_py_raise_py_value_error_exception("Invalid buffer.");
  }

  size = py_buf.len;
  if (size <= 0) {
    axis_ASSERT(0, "Should not happen.");
    return axis_py_raise_py_value_error_exception("Invalid buffer size.");
  }

  axis_error_t err;
  axis_error_init(&err);

  if (!axis_msg_remove_locked_res_buf(py_video_frame->msg.c_msg, data, &err)) {
    axis_ASSERT(0, "Should not happen.");
    return axis_py_raise_py_system_error_exception(
        "Failed to unlock buffer in video frame.");
  }

  Py_RETURN_NONE;
}

PyObject *axis_py_video_frame_get_buf(PyObject *self, PyObject *args) {
  axis_py_video_frame_t *py_video_frame = (axis_py_video_frame_t *)self;
  axis_ASSERT(py_video_frame &&
                 axis_py_msg_check_integrity((axis_py_msg_t *)py_video_frame),
             "Invalid argument.");

  axis_error_t err;
  axis_error_init(&err);

  axis_buf_t *buf = axis_video_frame_peek_buf(py_video_frame->msg.c_msg);
  uint8_t *data = buf->data;
  size_t data_size = buf->size;
  if (!data) {
    axis_ASSERT(0, "Should not happen.");
    return axis_py_raise_py_system_error_exception("Failed to get buffer.");
  }

  return PyByteArray_FromStringAndSize((const char *)data,
                                       (Py_ssize_t)data_size);
}

PyObject *axis_py_video_frame_get_width(PyObject *self, PyObject *args) {
  axis_py_video_frame_t *py_video_frame = (axis_py_video_frame_t *)self;
  axis_ASSERT(py_video_frame &&
                 axis_py_msg_check_integrity((axis_py_msg_t *)py_video_frame),
             "Invalid argument.");

  return PyLong_FromLong(axis_video_frame_get_width(py_video_frame->msg.c_msg));
}

PyObject *axis_py_video_frame_set_width(PyObject *self, PyObject *args) {
  axis_py_video_frame_t *py_video_frame = (axis_py_video_frame_t *)self;
  axis_ASSERT(py_video_frame &&
                 axis_py_msg_check_integrity((axis_py_msg_t *)py_video_frame),
             "Invalid argument.");

  int width = 0;
  if (!PyArg_ParseTuple(args, "i", &width)) {
    return axis_py_raise_py_value_error_exception("Invalid video frame width.");
  }

  axis_video_frame_set_width(py_video_frame->msg.c_msg, width);

  Py_RETURN_NONE;
}

PyObject *axis_py_video_frame_get_height(PyObject *self, PyObject *args) {
  axis_py_video_frame_t *py_video_frame = (axis_py_video_frame_t *)self;
  axis_ASSERT(py_video_frame &&
                 axis_py_msg_check_integrity((axis_py_msg_t *)py_video_frame),
             "Invalid argument.");

  return PyLong_FromLong(axis_video_frame_get_height(py_video_frame->msg.c_msg));
}

PyObject *axis_py_video_frame_set_height(PyObject *self, PyObject *args) {
  axis_py_video_frame_t *py_video_frame = (axis_py_video_frame_t *)self;
  axis_ASSERT(py_video_frame &&
                 axis_py_msg_check_integrity((axis_py_msg_t *)py_video_frame),
             "Invalid argument.");

  int height = 0;
  if (!PyArg_ParseTuple(args, "i", &height)) {
    return axis_py_raise_py_value_error_exception("Invalid video frame height.");
  }

  axis_video_frame_set_height(py_video_frame->msg.c_msg, height);

  Py_RETURN_NONE;
}

PyObject *axis_py_video_frame_get_timestamp(PyObject *self, PyObject *args) {
  axis_py_video_frame_t *py_video_frame = (axis_py_video_frame_t *)self;
  axis_ASSERT(py_video_frame &&
                 axis_py_msg_check_integrity((axis_py_msg_t *)py_video_frame),
             "Invalid argument.");

  return PyLong_FromLong(
      axis_video_frame_get_timestamp(py_video_frame->msg.c_msg));
}

PyObject *axis_py_video_frame_set_timestamp(PyObject *self, PyObject *args) {
  axis_py_video_frame_t *py_video_frame = (axis_py_video_frame_t *)self;
  axis_ASSERT(py_video_frame &&
                 axis_py_msg_check_integrity((axis_py_msg_t *)py_video_frame),
             "Invalid argument.");

  int64_t timestamp = 0;
  if (!PyArg_ParseTuple(args, "L", &timestamp)) {
    return axis_py_raise_py_value_error_exception("Invalid timestamp.");
  }

  axis_video_frame_set_timestamp(py_video_frame->msg.c_msg, timestamp);

  Py_RETURN_NONE;
}

PyObject *axis_py_video_frame_is_eof(PyObject *self, PyObject *args) {
  axis_py_video_frame_t *py_video_frame = (axis_py_video_frame_t *)self;
  axis_ASSERT(py_video_frame &&
                 axis_py_msg_check_integrity((axis_py_msg_t *)py_video_frame),
             "Invalid argument.");

  return PyBool_FromLong(axis_video_frame_is_eof(py_video_frame->msg.c_msg));
}

PyObject *axis_py_video_frame_set_eof(PyObject *self, PyObject *args) {
  axis_py_video_frame_t *py_video_frame = (axis_py_video_frame_t *)self;
  axis_ASSERT(py_video_frame &&
                 axis_py_msg_check_integrity((axis_py_msg_t *)py_video_frame),
             "Invalid argument.");

  int is_eof = 0;
  if (!PyArg_ParseTuple(args, "p", &is_eof)) {
    return axis_py_raise_py_value_error_exception("Invalid is_eof.");
  }

  axis_video_frame_set_eof(py_video_frame->msg.c_msg, is_eof);

  Py_RETURN_NONE;
}

PyObject *axis_py_video_frame_get_pixel_fmt(PyObject *self, PyObject *args) {
  axis_py_video_frame_t *py_video_frame = (axis_py_video_frame_t *)self;
  axis_ASSERT(py_video_frame &&
                 axis_py_msg_check_integrity((axis_py_msg_t *)py_video_frame),
             "Invalid argument.");

  return PyLong_FromLong(
      axis_video_frame_get_pixel_fmt(py_video_frame->msg.c_msg));
}

PyObject *axis_py_video_frame_set_pixel_fmt(PyObject *self, PyObject *args) {
  axis_py_video_frame_t *py_video_frame = (axis_py_video_frame_t *)self;
  axis_ASSERT(py_video_frame &&
                 axis_py_msg_check_integrity((axis_py_msg_t *)py_video_frame),
             "Invalid argument.");

  int pixel_fmt = 0;
  if (!PyArg_ParseTuple(args, "i", &pixel_fmt)) {
    return axis_py_raise_py_value_error_exception("Invalid pixel format.");
  }

  axis_video_frame_set_pixel_fmt(py_video_frame->msg.c_msg, pixel_fmt);

  Py_RETURN_NONE;
}

bool axis_py_video_frame_init_for_module(PyObject *module) {
  PyTypeObject *py_type = axis_py_video_frame_py_type();
  if (PyType_Ready(py_type) < 0) {
    axis_py_raise_py_system_error_exception(
        "Python VideoFrame class is not ready.");

    axis_ASSERT(0, "Should not happen.");
    return false;
  }

  if (PyModule_AddObjectRef(module, "_VideoFrame", (PyObject *)py_type) < 0) {
    axis_py_raise_py_import_error_exception(
        "Failed to add Python type to module.");

    axis_ASSERT(0, "Should not happen.");
    return false;
  }
  return true;
}
