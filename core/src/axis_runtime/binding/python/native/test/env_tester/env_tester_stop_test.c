//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/binding/python/test/env_tester.h"
#include "axis_runtime/test/env_tester.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"

PyObject *axis_py_axis_env_tester_stop_test(PyObject *self,
                                          axis_UNUSED PyObject *args) {
  axis_py_axis_env_tester_t *py_axis_env_tester = (axis_py_axis_env_tester_t *)self;
  axis_ASSERT(py_axis_env_tester &&
                 axis_py_axis_env_tester_check_integrity(py_axis_env_tester),
             "Invalid argument.");

  bool rc = axis_env_tester_stop_test(py_axis_env_tester->c_axis_env_tester, NULL);

  if (rc) {
    Py_RETURN_NONE;
  } else {
    return NULL;
  }
}
