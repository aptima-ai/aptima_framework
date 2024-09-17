//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <stdbool.h>

#include "include_internal/ten_runtime/binding/python/common/python_stuff.h"
#include "ten_utils/lib/signature.h"

#define TEN_PY_EXTENSION_SIGNATURE 0x37A997482A017BC8U

typedef struct ten_extension_t ten_extension_t;

typedef struct ten_py_extension_t {
  PyObject_HEAD
  ten_signature_t signature;
  ten_extension_t *c_extension;

  // Companion TEN object, the actual type is ten_py_ten_env_t.
  PyObject *py_ten_env;
} ten_py_extension_t;

TEN_RUNTIME_PRIVATE_API bool ten_py_extension_init_for_module(PyObject *module);

TEN_RUNTIME_PRIVATE_API PyTypeObject *ten_py_extension_py_type(void);
