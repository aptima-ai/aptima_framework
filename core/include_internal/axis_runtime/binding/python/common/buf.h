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
#include "axis_utils/lib/buf.h"

typedef struct axis_py_buf_t {
  PyObject_HEAD

  axis_buf_t *c_buf;
} axis_py_buf_t;

axis_RUNTIME_PRIVATE_API PyTypeObject *axis_py_buf_py_type(void);

axis_RUNTIME_PRIVATE_API axis_py_buf_t *axis_py_buf_wrap(axis_buf_t *buf);

axis_RUNTIME_PRIVATE_API void axis_py_buf_destroy(PyObject *self);

axis_RUNTIME_PRIVATE_API int axis_py_buf_get_buffer(PyObject *self,
                                                  Py_buffer *view, int flags);

axis_RUNTIME_PRIVATE_API bool axis_py_buf_init_for_module(PyObject *module);
