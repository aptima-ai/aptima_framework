//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/binding/python/msg/msg.h"

#include "include_internal/axis_runtime/binding/python/common/error.h"
#include "axis_runtime/common/errno.h"
#include "axis_runtime/msg/msg.h"
#include "axis_utils/lib/buf.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/macro/mark.h"
#include "axis_utils/macro/memory.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_get.h"
#include "axis_utils/value/value_is.h"
#include "axis_utils/value/value_json.h"

bool axis_py_msg_check_integrity(axis_py_msg_t *self) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_PY_MSG_SIGNATURE) {
    return false;
  }

  return true;
}

void axis_py_msg_destroy_c_msg(axis_py_msg_t *self) {
  if (self->c_msg) {
    axis_shared_ptr_destroy(self->c_msg);
    self->c_msg = NULL;
  }
}

axis_shared_ptr_t *axis_py_msg_move_c_msg(axis_py_msg_t *self) {
  axis_ASSERT(self, "Should not happen.");

  axis_shared_ptr_t *c_msg = self->c_msg;
  self->c_msg = NULL;

  return c_msg;
}

PyObject *axis_py_msg_get_name(PyObject *self, axis_UNUSED PyObject *args) {
  axis_py_msg_t *py_msg = (axis_py_msg_t *)self;

  axis_ASSERT(py_msg && axis_py_msg_check_integrity(py_msg), "Invalid argument.");

  axis_shared_ptr_t *c_msg = py_msg->c_msg;
  if (!c_msg) {
    axis_ASSERT(0, "Should not happen.");
    return axis_py_raise_py_value_error_exception("Msg is invalidated.");
  }

  const char *name = axis_msg_get_name(c_msg);

  PyObject *res = Py_BuildValue("s", name);

  return res;
}

PyObject *axis_py_msg_set_name(PyObject *self, axis_UNUSED PyObject *args) {
  axis_py_msg_t *py_msg = (axis_py_msg_t *)self;

  axis_ASSERT(py_msg && axis_py_msg_check_integrity(py_msg), "Invalid argument.");

  axis_shared_ptr_t *c_msg = py_msg->c_msg;
  if (!c_msg) {
    axis_ASSERT(0, "Should not happen.");
    return axis_py_raise_py_value_error_exception("Msg is invalidated.");
  }

  const char *name = NULL;
  if (!PyArg_ParseTuple(args, "s", &name)) {
    return axis_py_raise_py_value_error_exception("Failed to parse arguments.");
  }

  axis_error_t err;
  axis_error_init(&err);

  bool rc = axis_msg_set_name(c_msg, name, &err);

  if (!rc) {
    axis_py_raise_py_value_error_exception(axis_error_errmsg(&err));
  }

  axis_error_deinit(&err);

  if (rc) {
    Py_RETURN_NONE;
  } else {
    return NULL;
  }
}

PyObject *axis_py_msg_set_dest(PyObject *self, axis_UNUSED PyObject *args) {
  axis_py_msg_t *py_msg = (axis_py_msg_t *)self;

  axis_ASSERT(py_msg && axis_py_msg_check_integrity(py_msg), "Invalid argument.");

  if (PyTuple_GET_SIZE(args) != 4) {
    return axis_py_raise_py_value_error_exception(
        "Invalid argument count when set_dest.");
  }

  axis_shared_ptr_t *c_msg = py_msg->c_msg;
  if (!c_msg) {
    axis_ASSERT(0, "Should not happen.");
    return axis_py_raise_py_value_error_exception("Msg is invalidated.");
  }

  const char *app_uri = NULL;
  const char *graph_id = NULL;
  const char *extension_group_name = NULL;
  const char *extension_name = NULL;
  if (!PyArg_ParseTuple(args, "zzzz", &app_uri, &graph_id,
                        &extension_group_name, &extension_name)) {
    return axis_py_raise_py_value_error_exception("Failed to parse arguments.");
  }

  axis_error_t err;
  axis_error_init(&err);

  bool rc = axis_msg_clear_and_set_dest(
      c_msg, app_uri, graph_id, extension_group_name, extension_name, &err);

  if (!rc) {
    axis_py_raise_py_value_error_exception(axis_error_errmsg(&err));
  }

  axis_error_deinit(&err);

  if (rc) {
    Py_RETURN_NONE;
  } else {
    return NULL;
  }
}

PyObject *axis_py_msg_set_property_string(PyObject *self, PyObject *args) {
  axis_py_msg_t *py_msg = (axis_py_msg_t *)self;

  axis_ASSERT(py_msg && axis_py_msg_check_integrity(py_msg), "Invalid argument.");

  axis_shared_ptr_t *c_msg = py_msg->c_msg;
  if (!c_msg) {
    axis_ASSERT(0, "Should not happen.");
    return axis_py_raise_py_value_error_exception("Msg is invalidated.");
  }

  const char *path = NULL;
  const char *value = NULL;

  if (!PyArg_ParseTuple(args, "ss", &path, &value)) {
    return axis_py_raise_py_value_error_exception("Failed to parse arguments.");
  }

  axis_error_t err;
  axis_error_init(&err);

  if (!path || !value) {
    axis_error_set(&err, axis_ERRNO_INVALID_ARGUMENT, "Invalid argument.");
    axis_py_raise_py_value_error_exception(axis_error_errmsg(&err));
    axis_error_deinit(&err);
    return NULL;
  }

  axis_value_t *c_value = axis_value_create_string(value);

  bool rc = axis_msg_set_property(c_msg, path, c_value, &err);
  if (!rc) {
    axis_py_raise_py_value_error_exception(axis_error_errmsg(&err));
  }

  axis_error_deinit(&err);

  if (rc) {
    Py_RETURN_NONE;
  } else {
    return NULL;
  }
}

PyObject *axis_py_msg_get_property_string(PyObject *self, PyObject *args) {
  axis_py_msg_t *py_msg = (axis_py_msg_t *)self;

  axis_ASSERT(py_msg && axis_py_msg_check_integrity(py_msg), "Invalid argument.");

  axis_shared_ptr_t *c_msg = py_msg->c_msg;
  if (!c_msg) {
    axis_ASSERT(0, "Should not happen.");
    return axis_py_raise_py_value_error_exception("Msg is invalidated.");
  }

  const char *path = NULL;

  if (!PyArg_ParseTuple(args, "s", &path)) {
    return axis_py_raise_py_value_error_exception("Failed to parse arguments.");
  }

  axis_error_t err;
  axis_error_init(&err);

  if (!path) {
    axis_error_set(&err, axis_ERRNO_INVALID_ARGUMENT, "Invalid argument.");
    axis_py_raise_py_value_error_exception(axis_error_errmsg(&err));
    axis_error_deinit(&err);
    return NULL;
  }

  axis_value_t *c_value = axis_msg_peek_property(c_msg, path, &err);
  if (!c_value) {
    axis_py_raise_py_value_error_exception(axis_error_errmsg(&err));
    axis_error_deinit(&err);
    return NULL;
  }

  if (!axis_value_is_string(c_value)) {
    axis_error_set(&err, axis_ERRNO_INVALID_ARGUMENT, "Value is not string.");
    axis_py_raise_py_value_error_exception(axis_error_errmsg(&err));
    axis_error_deinit(&err);
    return NULL;
  }

  const char *value = axis_value_peek_raw_str(c_value, &err);

  PyObject *res = Py_BuildValue("s", value);

  return res;
}

PyObject *axis_py_msg_set_property_from_json(PyObject *self, PyObject *args) {
  axis_py_msg_t *py_msg = (axis_py_msg_t *)self;

  axis_ASSERT(py_msg && axis_py_msg_check_integrity(py_msg), "Invalid argument.");

  axis_shared_ptr_t *c_msg = py_msg->c_msg;
  if (!c_msg) {
    axis_ASSERT(0, "Should not happen.");
    return axis_py_raise_py_value_error_exception("Msg is invalidated.");
  }

  const char *path = NULL;
  const char *json_str = NULL;

  if (!PyArg_ParseTuple(args, "zs", &path, &json_str)) {
    return axis_py_raise_py_value_error_exception("Failed to parse arguments.");
  }

  axis_error_t err;
  axis_error_init(&err);

  axis_json_t *c_json = axis_json_from_string(json_str, &err);
  if (!c_json) {
    axis_py_raise_py_value_error_exception(axis_error_errmsg(&err));
    axis_error_deinit(&err);
    return NULL;
  }

  axis_value_t *value = axis_value_from_json(c_json);

  bool rc = axis_msg_set_property(c_msg, path, value, &err);
  if (!rc) {
    axis_py_raise_py_value_error_exception(axis_error_errmsg(&err));
    axis_value_destroy(value);
  }

  axis_json_destroy(c_json);

  axis_error_deinit(&err);

  if (rc) {
    Py_RETURN_NONE;
  } else {
    return NULL;
  }
}

PyObject *axis_py_msg_get_property_to_json(PyObject *self, PyObject *args) {
  axis_py_msg_t *py_msg = (axis_py_msg_t *)self;

  axis_ASSERT(py_msg && axis_py_msg_check_integrity(py_msg), "Invalid argument.");

  axis_shared_ptr_t *c_msg = py_msg->c_msg;
  if (!c_msg) {
    axis_ASSERT(0, "Should not happen.");
    return axis_py_raise_py_value_error_exception("Msg is invalidated.");
  }

  const char *path = NULL;
  if (PyTuple_GET_SIZE(args) == 1) {
    if (!PyArg_ParseTuple(args, "z", &path)) {
      return axis_py_raise_py_value_error_exception(
          "Failed to parse arguments.");
    }
  } else if (PyTuple_GET_SIZE(args) != 0) {
    return axis_py_raise_py_value_error_exception(
        "Invalid argument count when msg.get_property_to_json.");
  }

  axis_error_t err;
  axis_error_init(&err);

  axis_value_t *c_value = axis_msg_peek_property(c_msg, path, &err);
  if (!c_value) {
    axis_py_raise_py_value_error_exception(axis_error_errmsg(&err));
    axis_error_deinit(&err);
    return NULL;
  }

  axis_json_t *c_json = axis_value_to_json(c_value);

  bool must_free = true;
  const char *json_string = axis_json_to_string(c_json, NULL, &must_free);
  PyObject *res = Py_BuildValue("s", json_string);
  if (must_free) {
    axis_FREE(json_string);
  }

  axis_json_destroy(c_json);

  return res;
}

PyObject *axis_py_msg_get_property_int(PyObject *self, PyObject *args) {
  axis_py_msg_t *py_msg = (axis_py_msg_t *)self;

  axis_ASSERT(py_msg && axis_py_msg_check_integrity(py_msg), "Invalid argument.");

  axis_shared_ptr_t *c_msg = py_msg->c_msg;
  if (!c_msg) {
    axis_ASSERT(0, "Should not happen.");
    return axis_py_raise_py_value_error_exception("Msg is invalidated.");
  }

  const char *path = NULL;

  if (!PyArg_ParseTuple(args, "s", &path)) {
    return axis_py_raise_py_value_error_exception("Failed to parse arguments.");
  }

  axis_error_t err;
  axis_error_init(&err);

  if (!path) {
    axis_error_set(&err, axis_ERRNO_INVALID_ARGUMENT, "Invalid argument.");
    axis_py_raise_py_value_error_exception(axis_error_errmsg(&err));
    axis_error_deinit(&err);
    return NULL;
  }

  axis_value_t *c_value = axis_msg_peek_property(c_msg, path, &err);
  if (!c_value) {
    axis_py_raise_py_value_error_exception(axis_error_errmsg(&err));
    axis_error_deinit(&err);
    return NULL;
  }

  int64_t value = axis_value_get_int64(c_value, &err);
  if (!axis_error_is_success(&err)) {
    axis_py_raise_py_value_error_exception(axis_error_errmsg(&err));
    axis_error_deinit(&err);
    return NULL;
  }

  PyObject *res = Py_BuildValue("l", value);

  return res;
}

PyObject *axis_py_msg_set_property_int(PyObject *self, PyObject *args) {
  axis_py_msg_t *py_msg = (axis_py_msg_t *)self;

  axis_ASSERT(py_msg && axis_py_msg_check_integrity(py_msg), "Invalid argument.");

  axis_shared_ptr_t *c_msg = py_msg->c_msg;
  if (!c_msg) {
    axis_ASSERT(0, "Should not happen.");
    return axis_py_raise_py_value_error_exception("Msg is invalidated.");
  }

  const char *path = NULL;
  int64_t value = 0;

  if (!PyArg_ParseTuple(args, "sl", &path, &value)) {
    return axis_py_raise_py_value_error_exception("Failed to parse arguments.");
  }

  axis_error_t err;
  axis_error_init(&err);

  if (!path) {
    axis_error_set(&err, axis_ERRNO_INVALID_ARGUMENT, "Invalid argument.");
    axis_py_raise_py_value_error_exception(axis_error_errmsg(&err));
    axis_error_deinit(&err);
    return NULL;
  }

  axis_value_t *c_value = axis_value_create_int64(value);

  bool rc = axis_msg_set_property(c_msg, path, c_value, &err);
  if (!rc) {
    axis_py_raise_py_value_error_exception(axis_error_errmsg(&err));
  }

  axis_error_deinit(&err);

  if (rc) {
    Py_RETURN_NONE;
  } else {
    return NULL;
  }
}

PyObject *axis_py_msg_get_property_bool(PyObject *self, PyObject *args) {
  axis_py_msg_t *py_msg = (axis_py_msg_t *)self;

  axis_ASSERT(py_msg && axis_py_msg_check_integrity(py_msg), "Invalid argument.");

  axis_shared_ptr_t *c_msg = py_msg->c_msg;
  if (!c_msg) {
    axis_ASSERT(0, "Should not happen.");
    return axis_py_raise_py_value_error_exception("Msg is invalidated.");
  }

  const char *path = NULL;

  if (!PyArg_ParseTuple(args, "s", &path)) {
    return axis_py_raise_py_value_error_exception("Failed to parse arguments.");
  }

  axis_error_t err;
  axis_error_init(&err);

  if (!path) {
    axis_error_set(&err, axis_ERRNO_INVALID_ARGUMENT, "Invalid argument.");
    axis_py_raise_py_value_error_exception(axis_error_errmsg(&err));
    axis_error_deinit(&err);
    return NULL;
  }

  axis_value_t *c_value = axis_msg_peek_property(c_msg, path, &err);
  if (!c_value) {
    axis_py_raise_py_value_error_exception(axis_error_errmsg(&err));
    axis_error_deinit(&err);
    return NULL;
  }

  bool value = axis_value_get_bool(c_value, &err);
  if (!axis_error_is_success(&err)) {
    axis_py_raise_py_value_error_exception(axis_error_errmsg(&err));
    axis_error_deinit(&err);
    return NULL;
  }

  return PyBool_FromLong(value);
}

PyObject *axis_py_msg_set_property_bool(PyObject *self, PyObject *args) {
  axis_py_msg_t *py_msg = (axis_py_msg_t *)self;

  axis_ASSERT(py_msg && axis_py_msg_check_integrity(py_msg), "Invalid argument.");

  axis_shared_ptr_t *c_msg = py_msg->c_msg;
  if (!c_msg) {
    axis_ASSERT(0, "Should not happen.");
    return axis_py_raise_py_value_error_exception("Msg is invalidated.");
  }

  const char *path = NULL;
  int value = 0;

  if (!PyArg_ParseTuple(args, "si", &path, &value)) {
    return axis_py_raise_py_value_error_exception("Failed to parse arguments.");
  }

  axis_error_t err;
  axis_error_init(&err);

  if (!path) {
    axis_error_set(&err, axis_ERRNO_INVALID_ARGUMENT, "Invalid argument.");
    axis_py_raise_py_value_error_exception(axis_error_errmsg(&err));
    axis_error_deinit(&err);
    return NULL;
  }

  axis_value_t *c_value = axis_value_create_bool(value > 0);

  bool rc = axis_msg_set_property(c_msg, path, c_value, &err);
  if (!rc) {
    axis_py_raise_py_value_error_exception(axis_error_errmsg(&err));
  }

  axis_error_deinit(&err);

  if (rc) {
    Py_RETURN_NONE;
  } else {
    return NULL;
  }
}

PyObject *axis_py_msg_get_property_float(PyObject *self, PyObject *args) {
  axis_py_msg_t *py_msg = (axis_py_msg_t *)self;

  axis_ASSERT(py_msg && axis_py_msg_check_integrity(py_msg), "Invalid argument.");

  axis_shared_ptr_t *c_msg = py_msg->c_msg;
  if (!c_msg) {
    axis_ASSERT(0, "Should not happen.");
    return axis_py_raise_py_value_error_exception("Msg is invalidated.");
  }

  const char *path = NULL;

  if (!PyArg_ParseTuple(args, "s", &path)) {
    return axis_py_raise_py_value_error_exception("Failed to parse arguments.");
  }

  axis_error_t err;
  axis_error_init(&err);

  if (!path) {
    axis_error_set(&err, axis_ERRNO_INVALID_ARGUMENT, "Invalid argument.");
    axis_py_raise_py_value_error_exception(axis_error_errmsg(&err));
    axis_error_deinit(&err);
    return NULL;
  }

  axis_value_t *c_value = axis_msg_peek_property(c_msg, path, &err);
  if (!c_value) {
    axis_py_raise_py_value_error_exception(axis_error_errmsg(&err));
    axis_error_deinit(&err);
    return NULL;
  }

  double value = axis_value_get_float64(c_value, &err);
  if (!axis_error_is_success(&err)) {
    axis_py_raise_py_value_error_exception(axis_error_errmsg(&err));
    axis_error_deinit(&err);
    return NULL;
  }

  PyObject *res = Py_BuildValue("d", value);

  return res;
}

PyObject *axis_py_msg_set_property_float(PyObject *self, PyObject *args) {
  axis_py_msg_t *py_msg = (axis_py_msg_t *)self;

  axis_ASSERT(py_msg && axis_py_msg_check_integrity(py_msg), "Invalid argument.");

  axis_shared_ptr_t *c_msg = py_msg->c_msg;
  if (!c_msg) {
    axis_ASSERT(0, "Should not happen.");
    return axis_py_raise_py_value_error_exception("Msg is invalidated.");
  }

  const char *path = NULL;
  double value = 0.0;

  if (!PyArg_ParseTuple(args, "sd", &path, &value)) {
    return axis_py_raise_py_value_error_exception("Failed to parse arguments.");
  }

  axis_error_t err;
  axis_error_init(&err);

  if (!path) {
    axis_error_set(&err, axis_ERRNO_INVALID_ARGUMENT, "Invalid argument.");
    axis_py_raise_py_value_error_exception(axis_error_errmsg(&err));
    axis_error_deinit(&err);
    return NULL;
  }

  axis_value_t *c_value = axis_value_create_float64(value);

  bool rc = axis_msg_set_property(c_msg, path, c_value, &err);
  if (!rc) {
    axis_py_raise_py_value_error_exception(axis_error_errmsg(&err));
  }

  axis_error_deinit(&err);

  if (rc) {
    Py_RETURN_NONE;
  } else {
    return NULL;
  }
}

PyObject *axis_py_msg_get_property_buf(PyObject *self, PyObject *args) {
  axis_py_msg_t *py_msg = (axis_py_msg_t *)self;
  axis_ASSERT(py_msg && axis_py_msg_check_integrity(py_msg), "Invalid argument.");

  axis_shared_ptr_t *c_msg = py_msg->c_msg;
  if (!c_msg) {
    axis_ASSERT(0, "Should not happen.");
    return axis_py_raise_py_value_error_exception("Msg is invalidated.");
  }

  const char *path = NULL;

  if (!PyArg_ParseTuple(args, "s", &path)) {
    return axis_py_raise_py_value_error_exception("Failed to parse arguments.");
  }

  axis_error_t err;
  axis_error_init(&err);

  if (!path) {
    axis_error_set(&err, axis_ERRNO_INVALID_ARGUMENT, "Invalid argument.");
    axis_py_raise_py_value_error_exception(axis_error_errmsg(&err));
    axis_error_deinit(&err);
    return NULL;
  }

  axis_value_t *c_value = axis_msg_peek_property(c_msg, path, &err);
  if (!c_value) {
    axis_py_raise_py_value_error_exception(axis_error_errmsg(&err));
    axis_error_deinit(&err);
    return NULL;
  }

  if (!axis_value_is_buf(c_value)) {
    axis_error_set(&err, axis_ERRNO_INVALID_ARGUMENT, "Value is not buf.");
    axis_py_raise_py_value_error_exception(axis_error_errmsg(&err));
    axis_error_deinit(&err);
    return NULL;
  }

  axis_buf_t *buf = axis_value_peek_buf(c_value);
  axis_ASSERT(buf && axis_buf_check_integrity(buf), "Invalid buf.");

  return PyByteArray_FromStringAndSize((const char *)buf->data,
                                       (Py_ssize_t)buf->size);
}

PyObject *axis_py_msg_set_property_buf(PyObject *self, PyObject *args) {
  axis_py_msg_t *py_msg = (axis_py_msg_t *)self;

  axis_ASSERT(py_msg && axis_py_msg_check_integrity(py_msg), "Invalid argument.");

  axis_shared_ptr_t *c_msg = py_msg->c_msg;
  if (!c_msg) {
    axis_ASSERT(0, "Should not happen.");
    return axis_py_raise_py_value_error_exception("Msg is invalidated.");
  }

  const char *path = NULL;
  Py_buffer py_buf;

  if (!PyArg_ParseTuple(args, "sy*", &path, &py_buf)) {
    return axis_py_raise_py_value_error_exception("Failed to parse arguments.");
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

  axis_buf_t buf;
  axis_buf_init_with_owned_data(&buf, size);

  memcpy(buf.data, data, size);

  axis_value_t *c_value = axis_value_create_buf_with_move(buf);
  axis_ASSERT(c_value && axis_value_check_integrity(c_value),
             "Failed to create value.");

  bool rc = axis_msg_set_property(c_msg, path, c_value, &err);
  if (!rc) {
    axis_py_raise_py_value_error_exception(axis_error_errmsg(&err));
    axis_value_destroy(c_value);
  }

  axis_error_deinit(&err);
  PyBuffer_Release(&py_buf);

  if (rc) {
    Py_RETURN_NONE;
  } else {
    return NULL;
  }
}

bool axis_py_msg_init_for_module(PyObject *module) {
  PyTypeObject *py_type = axis_py_msg_py_type();

  if (PyType_Ready(py_type) < 0) {
    axis_py_raise_py_system_error_exception("Python Msg class is not ready.");

    axis_ASSERT(0, "Should not happen.");
    return false;
  }

  if (PyModule_AddObjectRef(module, "_Msg", (PyObject *)py_type) < 0) {
    axis_py_raise_py_import_error_exception(
        "Failed to add Python type to module.");

    axis_ASSERT(0, "Should not happen.");
    return false;
  }
  return true;
}
