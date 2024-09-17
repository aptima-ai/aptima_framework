//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include <stdbool.h>

#include "include_internal/ten_runtime/binding/python/common/python_stuff.h"
#include "ten_utils/log/log.h"

static void ten_py_print_py_error(void) {
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
      TEN_LOGE("Failed to convert exception value to string.");
    } else {
      const char *err_msg = PyUnicode_AsUTF8(py_err_msg);
      if (!err_msg) {
        TEN_LOGE("Failed to encode exception message as UTF-8.");
      } else {
        const char *py_exception_type = PyExceptionClass_Name(ptype);
        if (!py_exception_type) {
          py_exception_type = "Unknown Exception";
        }
        TEN_LOGE("%s: %s", py_exception_type, err_msg);
      }
      Py_DECREF(py_err_msg);
    }
  } else {
    TEN_LOGE("Failed to get exception value.");
  }

  if (ptraceback) {
    PyObject *stderr_file = PySys_GetObject("stderr");
    if (stderr_file) {
      // Dump the call stack of python codes to stderr.
      PyTraceBack_Print(ptraceback, stderr_file);
    } else {
      TEN_LOGW("Failed to get stderr to dump backtrace.");
    }
  }

  Py_XDECREF(ptype);
  Py_XDECREF(pvalue);
  Py_XDECREF(ptraceback);
}

bool ten_py_check_and_clear_py_error(void) {
  bool err_occurred = PyErr_Occurred();
  if (err_occurred) {
    ten_py_print_py_error();

    PyErr_Clear();
  }
  return err_occurred;
}

PyObject *ten_py_raise_py_value_error_exception(const char *msg) {
  TEN_LOGD("Raise Python ValueError exception: %s", msg);
  PyErr_SetString(PyExc_ValueError, msg);

  // Returning NULL indicates that an exception has occurred during the
  // function's execution, and signaling to the Python interpreter to handle the
  // exception.
  return NULL;
}

PyObject *ten_py_raise_py_type_error_exception(const char *msg) {
  TEN_LOGD("Raise Python TypeError exception: %s", msg);
  PyErr_SetString(PyExc_TypeError, msg);

  return NULL;
}

PyObject *ten_py_raise_py_memory_error_exception(const char *msg) {
  TEN_LOGD("Raise Python TypeError exception: %s", msg);
  PyErr_SetString(PyExc_MemoryError, msg);

  return NULL;
}

PyObject *ten_py_raise_py_system_error_exception(const char *msg) {
  TEN_LOGD("Raise Python SystemError exception: %s", msg);
  PyErr_SetString(PyExc_SystemError, msg);

  return NULL;
}

PyObject *ten_py_raise_py_import_error_exception(const char *msg) {
  TEN_LOGD("Raise Python ImportError exception: %s", msg);
  PyErr_SetString(PyExc_ImportError, msg);

  return NULL;
}

PyObject *ten_py_raise_py_runtime_error_exception(const char *msg) {
  TEN_LOGD("Raise Python RuntimeError exception: %s", msg);
  PyErr_SetString(PyExc_RuntimeError, msg);

  return NULL;
}

PyObject *ten_py_raise_py_not_implemented_error_exception(const char *msg) {
  TEN_LOGD("Raise Python NotImplementedError exception: %s", msg);
  PyErr_SetString(PyExc_NotImplementedError, msg);

  return NULL;
}
