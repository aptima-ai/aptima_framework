//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

#include "include_internal/axis_runtime/binding/python/common/python_stuff.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/smart_ptr.h"

#define axis_PY_MSG_SIGNATURE 0x043846812DB094D9U

typedef struct axis_py_msg_t {
  PyObject_HEAD
  axis_signature_t signature;
  axis_shared_ptr_t *c_msg;
} axis_py_msg_t;

axis_RUNTIME_PRIVATE_API PyObject *axis_py_msg_get_name(PyObject *self,
                                                      PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_msg_set_name(PyObject *self,
                                                      PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_msg_set_dest(PyObject *self,
                                                      PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_msg_set_property_string(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_msg_get_property_string(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_msg_set_property_from_json(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_msg_get_property_to_json(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_msg_get_property_int(PyObject *self,
                                                              PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_msg_set_property_int(PyObject *self,
                                                              PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_msg_get_property_bool(PyObject *self,
                                                               PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_msg_set_property_bool(PyObject *self,
                                                               PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_msg_get_property_float(PyObject *self,
                                                                PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_msg_set_property_float(PyObject *self,
                                                                PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_msg_get_property_buf(PyObject *self,
                                                              PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_msg_set_property_buf(PyObject *self,
                                                              PyObject *args);

axis_RUNTIME_PRIVATE_API PyTypeObject *axis_py_msg_py_type(void);

axis_RUNTIME_PRIVATE_API void axis_py_msg_destroy_c_msg(axis_py_msg_t *self);

axis_RUNTIME_PRIVATE_API axis_shared_ptr_t *axis_py_msg_move_c_msg(
    axis_py_msg_t *self);

axis_RUNTIME_PRIVATE_API bool axis_py_msg_init_for_module(PyObject *module);

axis_RUNTIME_PRIVATE_API bool axis_py_msg_check_integrity(axis_py_msg_t *self);
