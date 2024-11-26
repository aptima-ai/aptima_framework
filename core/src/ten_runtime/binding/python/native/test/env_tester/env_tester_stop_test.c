//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/binding/python/test/env_tester.h"
#include "ten_runtime/test/env_tester.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"

PyObject *ten_py_ten_env_tester_stop_test(PyObject *self,
                                          TEN_UNUSED PyObject *args) {
  ten_py_ten_env_tester_t *py_ten_env_tester = (ten_py_ten_env_tester_t *)self;
  TEN_ASSERT(py_ten_env_tester &&
                 ten_py_ten_env_tester_check_integrity(py_ten_env_tester),
             "Invalid argument.");

  bool rc = ten_env_tester_stop_test(py_ten_env_tester->c_ten_env_tester, NULL);

  if (rc) {
    Py_RETURN_NONE;
  } else {
    return NULL;
  }
}
