//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/binding/python/test/env_tester.h"
#include "ten_runtime/test/env_tester.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"

PyObject *ten_py_ten_env_tester_on_start_done(PyObject *self,
                                              TEN_UNUSED PyObject *args) {
  ten_py_ten_env_tester_t *py_ten_env_tester = (ten_py_ten_env_tester_t *)self;
  TEN_ASSERT(py_ten_env_tester &&
                 ten_py_ten_env_tester_check_integrity(py_ten_env_tester),
             "Invalid argument.");

  ten_error_t err;
  ten_error_init(&err);

  ten_env_tester_on_start_done(py_ten_env_tester->c_ten_env_tester, &err);

  ten_error_deinit(&err);

  Py_RETURN_NONE;
}
