//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/binding/python/common/python_stuff.h"
#include "include_internal/axis_runtime/binding/python/msg/msg.h"

typedef struct axis_py_cmd_result_t {
  axis_py_msg_t msg;
} axis_py_cmd_result_t;

axis_RUNTIME_PRIVATE_API PyTypeObject *axis_py_cmd_result_py_type(void);

axis_RUNTIME_PRIVATE_API bool axis_py_cmd_result_init_for_module(
    PyObject *module);

axis_RUNTIME_PRIVATE_API axis_py_cmd_result_t *axis_py_cmd_result_wrap(
    axis_shared_ptr_t *cmd);

axis_RUNTIME_PRIVATE_API void axis_py_cmd_result_invalidate(
    axis_py_cmd_result_t *self);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_cmd_result_create(PyTypeObject *type,
                                                           PyObject *args,
                                                           PyObject *kw);

axis_RUNTIME_PRIVATE_API void axis_py_cmd_result_destroy(PyObject *self);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_cmd_result_get_status_code(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_cmd_result_set_status_code(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_cmd_result_set_final(PyObject *self,
                                                              PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_cmd_result_is_final(PyObject *self,
                                                             PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_cmd_result_is_completed(
    PyObject *self, PyObject *args);
