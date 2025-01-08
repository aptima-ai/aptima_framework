//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_runtime/axis_config.h"

#include <Python.h>

#include "axis_runtime/app/app.h"

#define axis_PY_APP_SIGNATURE 0x3227E7A2722B6BB2U

typedef struct axis_py_app_t {
  PyObject_HEAD
  axis_signature_t signature;
  axis_app_t *c_app;
} axis_py_app_t;

axis_RUNTIME_PRIVATE_API bool axis_py_app_init_for_module(PyObject *module);
