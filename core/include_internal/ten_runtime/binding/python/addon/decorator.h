//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <stdbool.h>

#include "include_internal/ten_runtime/binding/python/common/python_stuff.h"
#include "ten_utils/lib/string.h"

typedef struct ten_py_decorator_register_addon_t {
  PyObject_HEAD
  ten_string_t addon_name;
} ten_py_decorator_register_addon_t;

TEN_RUNTIME_PRIVATE_API bool
ten_py_decorator_register_addon_as_extension_init_for_module(PyObject *module);

TEN_RUNTIME_PRIVATE_API bool
ten_py_decorator_register_addon_as_extension_group_init_for_module(
    PyObject *module);
