//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <string.h>

#include "include_internal/axis_runtime/binding/python/common/error.h"
#include "include_internal/axis_runtime/binding/python/axis_env/axis_env.h"
#include "include_internal/axis_runtime/metadata/metadata_info.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "axis_runtime/axis_env/internal/on_xxx_done.h"
#include "axis_runtime/axis_env_proxy/axis_env_proxy.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"

static void axis_env_proxy_notify_on_init_done(axis_env_t *axis_env,
                                              axis_UNUSED void *user_data) {
  axis_ASSERT(
      axis_env &&
          axis_env_check_integrity(
              axis_env,
              axis_env->attach_to != axis_ENV_ATTACH_TO_ADDON ? true : false),
      "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  bool rc = axis_env_on_init_done(axis_env, &err);
  axis_ASSERT(rc, "Should not happen.");

  axis_error_deinit(&err);
}

PyObject *axis_py_axis_env_on_init_done(PyObject *self, PyObject *args) {
  axis_py_axis_env_t *py_axis_env = (axis_py_axis_env_t *)self;
  axis_ASSERT(py_axis_env && axis_py_axis_env_check_integrity(py_axis_env),
             "Invalid argument.");

  axis_error_t err;
  axis_error_init(&err);

  bool rc = true;

  if (py_axis_env->c_axis_env->attach_to == axis_ENV_ATTACH_TO_ADDON) {
    rc = axis_env_on_init_done(py_axis_env->c_axis_env, &err);
  } else {
    rc = axis_env_proxy_notify_async(py_axis_env->c_axis_env_proxy,
                                    axis_env_proxy_notify_on_init_done, NULL,
                                    &err);
  }

  if (!rc) {
    axis_error_deinit(&err);
    return axis_py_raise_py_runtime_error_exception(
        "Failed to notify on init done.");
  }

  axis_error_deinit(&err);

  Py_RETURN_NONE;
}
