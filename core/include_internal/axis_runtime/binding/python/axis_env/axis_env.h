//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/binding/python/common/python_stuff.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "include_internal/axis_runtime/axis_env_proxy/axis_env_proxy.h"
#include "axis_runtime/axis_env/axis_env.h"

#define axis_PY_axis_ENV_SIGNATURE 0xCCCC1DD4BB4CA743U

typedef struct axis_py_axis_env_t {
  PyObject_HEAD

  axis_signature_t signature;

  axis_env_t *c_axis_env;
  axis_env_proxy_t *c_axis_env_proxy;
  PyObject *actual_py_axis_env;

  // Mark whether the gil state need to be released after 'on_deinit_done'.
  bool need_to_release_gil_state;
  PyThreadState *py_thread_state;
} axis_py_axis_env_t;

axis_RUNTIME_PRIVATE_API bool axis_py_axis_env_check_integrity(
    axis_py_axis_env_t *self);

axis_RUNTIME_PRIVATE_API PyTypeObject *axis_py_axis_env_type(void);

axis_RUNTIME_PRIVATE_API axis_py_axis_env_t *axis_py_axis_env_wrap(
    axis_env_t *axis_env);

axis_RUNTIME_PRIVATE_API void axis_py_axis_env_invalidate(
    axis_py_axis_env_t *py_ten);

axis_RUNTIME_PRIVATE_API bool axis_py_axis_env_init_for_module(PyObject *module);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_on_configure_done(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_on_init_done(PyObject *self,
                                                              PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_on_start_done(PyObject *self,
                                                               PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_on_stop_done(PyObject *self,
                                                              PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_on_deinit_done(PyObject *self,
                                                                PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_on_create_instance_done(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_send_cmd(PyObject *self,
                                                          PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_send_data(PyObject *self,
                                                           PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_send_video_frame(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_send_audio_frame(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_return_result(PyObject *self,
                                                               PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_return_result_directly(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_get_property_to_json(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_set_property_from_json(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_is_property_exist(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_get_property_int(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_set_property_int(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_get_property_string(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_set_property_string(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_get_property_bool(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_set_property_bool(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_get_property_float(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_set_property_float(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_init_property_from_json(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_get_property_to_json_async(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_set_property_from_json_async(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_is_property_exist_async(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_get_property_int_async(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_set_property_int_async(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_get_property_string_async(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_set_property_string_async(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_get_property_bool_async(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_set_property_bool_async(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_get_property_float_async(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_set_property_float_async(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_init_property_from_json_async(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_axis_env_log(PyObject *self,
                                                     PyObject *args);
