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
#include "axis_utils/lib/error.h"

typedef struct axis_py_error_t {
  PyObject_HEAD

  axis_error_t c_error;
} axis_py_error_t;

axis_RUNTIME_PRIVATE_API PyTypeObject *axis_py_error_py_type(void);

axis_RUNTIME_PRIVATE_API bool axis_py_error_init_for_module(PyObject *module);

axis_RUNTIME_PRIVATE_API axis_py_error_t *axis_py_error_wrap(axis_error_t *error);

axis_RUNTIME_PRIVATE_API void axis_py_error_invalidate(axis_py_error_t *error);

axis_RUNTIME_PRIVATE_API void axis_py_error_destroy(PyObject *self);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_error_get_errno(PyObject *self,
                                                         PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_error_get_errmsg(PyObject *self,
                                                          PyObject *args);

axis_RUNTIME_PRIVATE_API bool axis_py_check_and_clear_py_error(void);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_raise_py_value_error_exception(
    const char *msg, ...);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_raise_py_type_error_exception(
    const char *msg);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_raise_py_memory_error_exception(
    const char *msg);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_raise_py_system_error_exception(
    const char *msg);

axis_RUNTIME_PRIVATE_API PyObject *
axis_py_raise_py_not_implemented_error_exception(const char *msg);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_raise_py_import_error_exception(
    const char *msg);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_raise_py_runtime_error_exception(
    const char *msg);
