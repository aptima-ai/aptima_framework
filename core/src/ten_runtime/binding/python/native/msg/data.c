//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "include_internal/ten_runtime/binding/python/msg/data.h"

#include "include_internal/ten_runtime/binding/python/common/error.h"
#include "include_internal/ten_runtime/binding/python/msg/msg.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "pyport.h"
#include "ten_runtime/msg/data/data.h"
#include "ten_runtime/msg/msg.h"
#include "ten_utils/lib/buf.h"
#include "ten_utils/macro/check.h"

static ten_py_data_t *ten_py_data_create_internal(PyTypeObject *py_type) {
  if (!py_type) {
    py_type = ten_py_data_py_type();
  }

  ten_py_data_t *py_data = (ten_py_data_t *)py_type->tp_alloc(py_type, 0);

  ten_signature_set(&py_data->msg.signature, TEN_PY_MSG_SIGNATURE);
  py_data->msg.c_msg = NULL;

  return py_data;
}

static ten_py_data_t *ten_py_data_init(ten_py_data_t *py_data,
                                       TEN_UNUSED PyObject *args,
                                       TEN_UNUSED PyObject *kw) {
  TEN_ASSERT(py_data && ten_py_msg_check_integrity((ten_py_msg_t *)py_data),
             "Invalid argument.");

  py_data->msg.c_msg = ten_msg_create_from_msg_type(TEN_MSG_TYPE_DATA);

  return py_data;
}

PyObject *ten_py_data_create(PyTypeObject *type, TEN_UNUSED PyObject *args,
                             TEN_UNUSED PyObject *kwds) {
  ten_py_data_t *py_data = ten_py_data_create_internal(type);
  return (PyObject *)ten_py_data_init(py_data, args, kwds);
}

void ten_py_data_destroy(PyObject *self) {
  ten_py_data_t *py_data = (ten_py_data_t *)self;
  TEN_ASSERT(py_data && ten_py_msg_check_integrity((ten_py_msg_t *)py_data),
             "Invalid argument.");

  ten_py_msg_destroy_c_msg(&py_data->msg);
  Py_TYPE(self)->tp_free(self);
}

PyObject *ten_py_data_alloc_buf(PyObject *self, PyObject *args) {
  ten_py_data_t *py_data = (ten_py_data_t *)self;
  TEN_ASSERT(py_data && ten_py_msg_check_integrity((ten_py_msg_t *)py_data),
             "Invalid argument.");

  int size = 0;
  if (!PyArg_ParseTuple(args, "i", &size)) {
    return ten_py_raise_py_value_error_exception("Invalid buffer size.");
  }

  if (size <= 0) {
    return ten_py_raise_py_value_error_exception("Invalid buffer size.");
  }

  ten_data_alloc_buf(py_data->msg.c_msg, size);

  Py_RETURN_NONE;
}

PyObject *ten_py_data_lock_buf(PyObject *self, PyObject *args) {
  ten_py_data_t *py_data = (ten_py_data_t *)self;
  TEN_ASSERT(py_data && ten_py_msg_check_integrity((ten_py_msg_t *)py_data),
             "Invalid argument.");

  ten_error_t err;
  ten_error_init(&err);

  if (!ten_msg_add_locked_res_buf(py_data->msg.c_msg,
                                  ten_data_peek_buf(py_data->msg.c_msg)->data,
                                  &err)) {
    return ten_py_raise_py_system_error_exception(
        "Failed to lock buffer in data.");
  }

  ten_buf_t *result = ten_data_peek_buf(py_data->msg.c_msg);

  return PyMemoryView_FromMemory((char *)result->data, (Py_ssize_t)result->size,
                                 PyBUF_WRITE);
}

PyObject *ten_py_data_unlock_buf(PyObject *self, PyObject *args) {
  ten_py_data_t *py_data = (ten_py_data_t *)self;
  TEN_ASSERT(py_data && ten_py_msg_check_integrity((ten_py_msg_t *)py_data),
             "Invalid argument.");

  Py_buffer py_buf;
  if (!PyArg_ParseTuple(args, "y*", &py_buf)) {
    return ten_py_raise_py_value_error_exception("Invalid buffer.");
  }

  Py_ssize_t size = 0;
  uint8_t *data = py_buf.buf;
  if (!data) {
    return ten_py_raise_py_value_error_exception("Invalid buffer.");
  }

  size = py_buf.len;
  if (size <= 0) {
    return ten_py_raise_py_value_error_exception("Invalid buffer size.");
  }

  ten_error_t err;
  ten_error_init(&err);

  if (!ten_msg_remove_locked_res_buf(py_data->msg.c_msg, data, &err)) {
    return ten_py_raise_py_system_error_exception(
        "Failed to unlock buffer in data.");
  }

  Py_RETURN_NONE;
}

PyObject *ten_py_data_get_buf(PyObject *self, PyObject *args) {
  ten_py_data_t *py_data = (ten_py_data_t *)self;
  TEN_ASSERT(py_data && ten_py_msg_check_integrity((ten_py_msg_t *)py_data),
             "Invalid argument.");

  ten_error_t err;
  ten_error_init(&err);

  ten_buf_t *buf = ten_data_peek_buf(py_data->msg.c_msg);
  if (!buf) {
    return ten_py_raise_py_system_error_exception("Failed to get buffer.");
  }

  size_t data_size = buf->size;

  return PyByteArray_FromStringAndSize((const char *)buf->data, data_size);
}

ten_py_data_t *ten_py_data_wrap(ten_shared_ptr_t *data) {
  TEN_ASSERT(data && ten_msg_check_integrity(data), "Invalid argument.");

  ten_py_data_t *py_data = ten_py_data_create_internal(NULL);
  py_data->msg.c_msg = ten_shared_ptr_clone(data);
  return py_data;
}

void ten_py_data_invalidate(ten_py_data_t *self) {
  TEN_ASSERT(self, "Invalid argument");
  Py_DECREF(self);
}

bool ten_py_data_init_for_module(PyObject *module) {
  PyTypeObject *py_type = ten_py_data_py_type();
  if (PyType_Ready(py_type) < 0) {
    ten_py_raise_py_system_error_exception("Python Data class is not ready.");

    TEN_ASSERT(0, "Should not happen.");
    return false;
  }

  if (PyModule_AddObjectRef(module, "_Data", (PyObject *)py_type) < 0) {
    ten_py_raise_py_import_error_exception(
        "Failed to add Python type to module.");

    TEN_ASSERT(0, "Should not happen.");
    return false;
  }
  return true;
}
