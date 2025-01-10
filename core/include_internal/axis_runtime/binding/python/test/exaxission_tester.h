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
#include "include_internal/axis_runtime/test/extension_tester.h"
#include "axis_utils/lib/signature.h"

#define axis_PY_EXTENSION_TESTER_SIGNATURE 0x2B343E0B87397B5FU

typedef struct axis_py_extension_tester_t {
  PyObject_HEAD
  axis_signature_t signature;
  axis_extension_tester_t *c_extension_tester;

  // Companion axis_env_tester object, the actual type is
  // axis_py_axis_env_tester_t.
  PyObject *py_axis_env_tester;
} axis_py_extension_tester_t;

axis_RUNTIME_PRIVATE_API PyTypeObject *axis_py_extension_tester_py_type(void);

axis_RUNTIME_API bool axis_py_extension_tester_init_for_module(PyObject *module);
