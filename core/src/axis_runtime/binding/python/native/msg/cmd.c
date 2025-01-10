//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/binding/python/msg/cmd.h"

#include "include_internal/axis_runtime/binding/python/common/error.h"
#include "include_internal/axis_runtime/binding/python/msg/msg.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "axis_runtime/msg/cmd/cmd.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/macro/mark.h"

static axis_py_cmd_t *axis_py_cmd_create_internal(PyTypeObject *py_type) {
  if (!py_type) {
    py_type = axis_py_cmd_py_type();
  }

  axis_py_cmd_t *py_cmd = (axis_py_cmd_t *)py_type->tp_alloc(py_type, 0);

  axis_signature_set(&py_cmd->msg.signature, axis_PY_MSG_SIGNATURE);
  py_cmd->msg.c_msg = NULL;

  return py_cmd;
}

void axis_py_cmd_destroy(PyObject *self) {
  axis_py_cmd_t *py_cmd = (axis_py_cmd_t *)self;

  axis_ASSERT(py_cmd && axis_py_msg_check_integrity((axis_py_msg_t *)py_cmd),
             "Invalid argument.");

  axis_py_msg_destroy_c_msg(&py_cmd->msg);
  Py_TYPE(self)->tp_free(self);
}

static axis_py_cmd_t *axis_py_cmd_init(axis_py_cmd_t *py_cmd, const char *name) {
  axis_ASSERT(py_cmd && axis_py_msg_check_integrity((axis_py_msg_t *)py_cmd),
             "Invalid argument.");

  py_cmd->msg.c_msg = axis_cmd_create(name, NULL);
  return py_cmd;
}

PyObject *axis_py_cmd_create(PyTypeObject *type, PyObject *args,
                            axis_UNUSED PyObject *kw) {
  const char *name = NULL;
  if (!PyArg_ParseTuple(args, "s", &name)) {
    return axis_py_raise_py_value_error_exception("Failed to parse arguments.");
  }

  axis_py_cmd_t *py_cmd = axis_py_cmd_create_internal(type);
  return (PyObject *)axis_py_cmd_init(py_cmd, name);
}

axis_py_cmd_t *axis_py_cmd_wrap(axis_shared_ptr_t *cmd) {
  axis_ASSERT(cmd && axis_msg_check_integrity(cmd), "Invalid argument.");

  axis_py_cmd_t *py_cmd = axis_py_cmd_create_internal(NULL);
  py_cmd->msg.c_msg = axis_shared_ptr_clone(cmd);
  return py_cmd;
}

void axis_py_cmd_invalidate(axis_py_cmd_t *self) {
  axis_ASSERT(self, "Invalid argument");
  Py_DECREF(self);
}

bool axis_py_cmd_init_for_module(PyObject *module) {
  PyTypeObject *py_type = axis_py_cmd_py_type();
  if (PyType_Ready(py_type) < 0) {
    axis_py_raise_py_system_error_exception("Python Cmd class is not ready.");

    axis_ASSERT(0, "Should not happen.");
    return false;
  }

  if (PyModule_AddObjectRef(module, "_Cmd", (PyObject *)py_type) < 0) {
    axis_py_raise_py_import_error_exception(
        "Failed to add Python type to module.");

    axis_ASSERT(0, "Should not happen.");
    return false;
  }
  return true;
}
