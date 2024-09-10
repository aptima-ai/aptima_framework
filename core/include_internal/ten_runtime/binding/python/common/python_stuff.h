//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#define PY_SSIZE_T_CLEAN

// Do not enable the debugging facility in Python even if TEN runtime is built
// in the debug mode.
#ifdef _DEBUG
  #undef _DEBUG
  #include <Python.h>  // IWYU pragma: always_keep
  #define _DEBUG
#else
  #include <Python.h>  // IWYU pragma: always_keep
#endif
