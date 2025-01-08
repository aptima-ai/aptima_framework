//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

#include "include_internal/axis_runtime/binding/python/common/python_stuff.h"

typedef struct axis_py_addon_manager_register_addon_decorator_t {
  PyObject_HEAD
} axis_py_addon_manager_register_addon_decorator_t;

axis_RUNTIME_PRIVATE_API PyObject *
axis_py_addon_manager_register_addon_as_extension(PyObject *self,
                                                 PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_unregister_all_addons_and_cleanup(
    PyObject *self, PyObject *args);
