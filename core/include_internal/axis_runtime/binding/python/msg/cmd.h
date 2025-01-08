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
#include "axis_utils/lib/smart_ptr.h"

typedef struct axis_py_cmd_t {
  axis_py_msg_t msg;
} axis_py_cmd_t;

axis_RUNTIME_PRIVATE_API PyTypeObject *axis_py_cmd_py_type(void);

axis_RUNTIME_PRIVATE_API bool axis_py_cmd_init_for_module(PyObject *module);

axis_RUNTIME_PRIVATE_API axis_py_cmd_t *axis_py_cmd_wrap(axis_shared_ptr_t *cmd);

axis_RUNTIME_PRIVATE_API void axis_py_cmd_invalidate(axis_py_cmd_t *self);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_cmd_create(PyTypeObject *type,
                                                    PyObject *args,
                                                    PyObject *kw);

axis_RUNTIME_PRIVATE_API void axis_py_cmd_destroy(PyObject *self);
