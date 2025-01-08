//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/binding/python/common/error.h"

#include <stdbool.h>

#include "include_internal/axis_runtime/binding/python/common/python_stuff.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/log/log.h"
#include "axis_utils/macro/check.h"

axis_py_error_t *axis_py_error_wrap(axis_error_t *error) {
  if (!error) {
    return NULL;
  }

  axis_py_error_t *py_error = (axis_py_error_t *)axis_py_error_py_type()->tp_alloc(
      axis_py_error_py_type(), 0);
  if (!py_error) {
    return NULL;
  }

  axis_error_init(&py_error->c_error);
  axis_error_copy(&py_error->c_error, error);

  return py_error;
}

void axis_py_error_invalidate(axis_py_error_t *py_error) {
  axis_ASSERT(py_error, "Invalid argument.");
  Py_DECREF(py_error);
}

void axis_py_error_destroy(PyObject *self) {
  axis_py_error_t *py_error = (axis_py_error_t *)self;
  if (!py_error) {
    return;
  }

  axis_error_deinit(&py_error->c_error);

  Py_TYPE(self)->tp_free(self);
}

PyObject *axis_py_error_get_errno(PyObject *self, PyObject *args) {
  axis_py_error_t *py_error = (axis_py_error_t *)self;
  if (!py_error) {
    return axis_py_raise_py_value_error_exception("Invalid argument.");
  }

  return PyLong_FromLong(axis_error_errno(&py_error->c_error));
}

PyObject *axis_py_error_get_errmsg(PyObject *self, PyObject *args) {
  axis_py_error_t *py_error = (axis_py_error_t *)self;
  if (!py_error) {
    return axis_py_raise_py_value_error_exception("Invalid argument.");
  }

  return PyUnicode_FromString(axis_error_errmsg(&py_error->c_error));
}

static void axis_py_print_py_error(void) {
  PyObject *ptype = NULL;
  PyObject *pvalue = NULL;
  PyObject *ptraceback = NULL;

  PyErr_Fetch(&ptype, &pvalue, &ptraceback);
  if (!ptype && !pvalue && !ptraceback) {
    // No error to fetch.
    return;
  }

  PyErr_NormalizeException(&ptype, &pvalue, &ptraceback);
  if (pvalue != NULL) {
    PyObject *py_err_msg = PyObject_Str(pvalue);
    if (!py_err_msg) {
      axis_LOGE("Failed to convert exception value to string.");
    } else {
      const char *err_msg = PyUnicode_AsUTF8(py_err_msg);
      if (!err_msg) {
        axis_LOGE("Failed to encode exception message as UTF-8.");
      } else {
        const char *py_exception_type = PyExceptionClass_Name(ptype);
        if (!py_exception_type) {
          py_exception_type = "Unknown Exception";
        }
        axis_LOGE("%s: %s", py_exception_type, err_msg);
      }
      Py_DECREF(py_err_msg);
    }
  } else {
    axis_LOGE("Failed to get exception value.");
  }

  if (ptraceback) {
    PyObject *stderr_file = PySys_GetObject("stderr");
    if (stderr_file) {
      // Dump the call stack of python codes to stderr.
      PyTraceBack_Print(ptraceback, stderr_file);
    } else {
      axis_LOGW("Failed to get stderr to dump backtrace.");
    }
  }

  Py_XDECREF(ptype);
  Py_XDECREF(pvalue);
  Py_XDECREF(ptraceback);
}

bool axis_py_check_and_clear_py_error(void) {
  bool err_occurred = PyErr_Occurred();
  if (err_occurred) {
    axis_py_print_py_error();

    PyErr_Clear();
  }
  return err_occurred;
}

PyObject *axis_py_raise_py_value_error_exception(const char *msg, ...) {
  axis_string_t err_msg;
  axis_string_init(&err_msg);

  va_list args;
  va_start(args, msg);
  axis_string_append_from_va_list(&err_msg, msg, args);
  va_end(args);

  axis_LOGD("Raise Python ValueError exception: %s",
           axis_string_get_raw_str(&err_msg));
  PyErr_SetString(PyExc_ValueError, axis_string_get_raw_str(&err_msg));

  axis_string_deinit(&err_msg);

  // Returning NULL indicates that an exception has occurred during the
  // function's execution, and signaling to the Python interpreter to handle the
  // exception.
  return NULL;
}

PyObject *axis_py_raise_py_type_error_exception(const char *msg) {
  axis_LOGD("Raise Python TypeError exception: %s", msg);
  PyErr_SetString(PyExc_TypeError, msg);

  return NULL;
}

PyObject *axis_py_raise_py_memory_error_exception(const char *msg) {
  axis_LOGD("Raise Python TypeError exception: %s", msg);
  PyErr_SetString(PyExc_MemoryError, msg);

  return NULL;
}

PyObject *axis_py_raise_py_system_error_exception(const char *msg) {
  axis_LOGD("Raise Python SystemError exception: %s", msg);
  PyErr_SetString(PyExc_SystemError, msg);

  return NULL;
}

PyObject *axis_py_raise_py_import_error_exception(const char *msg) {
  axis_LOGD("Raise Python ImportError exception: %s", msg);
  PyErr_SetString(PyExc_ImportError, msg);

  return NULL;
}

PyObject *axis_py_raise_py_runtime_error_exception(const char *msg) {
  axis_LOGD("Raise Python RuntimeError exception: %s", msg);
  PyErr_SetString(PyExc_RuntimeError, msg);

  return NULL;
}

PyObject *axis_py_raise_py_not_implemented_error_exception(const char *msg) {
  axis_LOGD("Raise Python NotImplementedError exception: %s", msg);
  PyErr_SetString(PyExc_NotImplementedError, msg);

  return NULL;
}

bool axis_py_error_init_for_module(PyObject *module) {
  PyTypeObject *py_type = axis_py_error_py_type();

  if (PyType_Ready(py_type) < 0) {
    axis_py_raise_py_system_error_exception("Python Error class is not ready.");

    axis_ASSERT(0, "Should not happen.");
    return false;
  }

  if (PyModule_AddObjectRef(module, "_TenError", (PyObject *)py_type) < 0) {
    axis_py_raise_py_import_error_exception(
        "Failed to add Python type to module.");

    axis_ASSERT(0, "Should not happen.");
    return false;
  }

  return true;
}
