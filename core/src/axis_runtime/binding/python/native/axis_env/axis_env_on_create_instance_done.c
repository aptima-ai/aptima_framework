//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <string.h>

#include "include_internal/axis_runtime/binding/python/common/error.h"
#include "include_internal/axis_runtime/binding/python/extension/extension.h"
#include "include_internal/axis_runtime/binding/python/axis_env/axis_env.h"
#include "object.h"
#include "axis_runtime/axis_env/internal/on_xxx_done.h"
#include "axis_utils/macro/mark.h"

PyObject *axis_py_axis_env_on_create_instance_done(PyObject *self,
                                                 axis_UNUSED PyObject *args) {
  axis_py_axis_env_t *py_axis_env = (axis_py_axis_env_t *)self;
  axis_ASSERT(py_axis_env && axis_py_axis_env_check_integrity(py_axis_env),
             "Invalid argument.");

  axis_error_t err;
  axis_error_init(&err);

  axis_py_extension_t *extension = NULL;
  void *context = NULL;

  if (!PyArg_ParseTuple(args, "O!l", axis_py_extension_py_type(), &extension,
                        &context)) {
    axis_py_raise_py_value_error_exception(
        "Invalid argument count when axis_env.on_create_instance_done.");
  }

  bool rc = axis_env_on_create_instance_done(
      py_axis_env->c_axis_env, extension->c_extension, context, &err);
  axis_ASSERT(rc, "Should not happen.");

  // It's necessary to keep the reference of the extension object to
  // prevent the python object from being destroyed when GC is triggered util
  // the addon's 'on_destroy_instance' function is called.
  Py_INCREF(extension);

  axis_error_deinit(&err);

  Py_RETURN_NONE;
}
