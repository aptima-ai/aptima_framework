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

#define axis_PY_EXTENSION_SIGNATURE 0x37A997482A017BC8U

typedef struct axis_extension_t axis_extension_t;

typedef struct axis_py_extension_t {
  PyObject_HEAD
  axis_signature_t signature;
  axis_extension_t *c_extension;

  // Companion axis_env object, the actual type is axis_py_axis_env_t.
  PyObject *py_axis_env;
} axis_py_extension_t;

axis_RUNTIME_PRIVATE_API bool axis_py_extension_init_for_module(PyObject *module);

axis_RUNTIME_PRIVATE_API PyTypeObject *axis_py_extension_py_type(void);
