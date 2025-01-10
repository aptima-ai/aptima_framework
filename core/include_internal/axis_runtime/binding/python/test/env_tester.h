//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

#include "include_internal/axis_runtime/binding/python/common/python_stuff.h"
#include "include_internal/axis_runtime/test/env_tester.h"
#include "axis_utils/lib/signature.h"

#define axis_PY_axis_ENV_TESTER_SIGNATURE 0x9DF807EAFAF9F6D5U

typedef struct axis_py_axis_env_tester_t {
  PyObject_HEAD

  axis_signature_t signature;

  axis_env_tester_t *c_axis_env_tester;
  PyObject *actual_py_axis_env_tester;
} axis_py_axis_env_tester_t;

axis_RUNTIME_PRIVATE_API PyTypeObject *axis_py_axis_env_tester_py_type(void);

axis_RUNTIME_PRIVATE_API bool axis_py_axis_env_tester_init_for_module(
    PyObject *module);

axis_RUNTIME_PRIVATE_API axis_py_axis_env_tester_t *axis_py_axis_env_tester_wrap(
    axis_env_tester_t *axis_env_tester);

axis_RUNTIME_PRIVATE_API void axis_py_axis_env_tester_invalidate(
    axis_py_axis_env_tester_t *py_ten);

axis_RUNTIME_PRIVATE_API PyTypeObject *axis_py_axis_env_tester_type(void);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_tester_on_start_done(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_tester_stop_test(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_tester_send_cmd(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_tester_send_data(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_tester_send_audio_frame(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_tester_send_video_frame(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_tester_return_result(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API bool axis_py_axis_env_tester_check_integrity(
    axis_py_axis_env_tester_t *self);
