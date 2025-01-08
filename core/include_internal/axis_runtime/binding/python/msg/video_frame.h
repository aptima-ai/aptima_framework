//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/binding/python/common/python_stuff.h"
#include "include_internal/axis_runtime/binding/python/msg/msg.h"
#include "axis_utils/lib/smart_ptr.h"

typedef struct axis_py_video_frame_t {
  axis_py_msg_t msg;
} axis_py_video_frame_t;

axis_RUNTIME_PRIVATE_API PyTypeObject *axis_py_video_frame_py_type(void);

axis_RUNTIME_PRIVATE_API bool axis_py_video_frame_init_for_module(
    PyObject *module);

axis_RUNTIME_PRIVATE_API axis_py_video_frame_t *axis_py_video_frame_wrap(
    axis_shared_ptr_t *video_frame);

axis_RUNTIME_PRIVATE_API void axis_py_video_frame_invalidate(
    axis_py_video_frame_t *self);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_video_frame_create(PyTypeObject *type,
                                                            PyObject *args,
                                                            PyObject *kwds);

axis_RUNTIME_PRIVATE_API void axis_py_video_frame_destroy(PyObject *self);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_video_frame_alloc_buf(PyObject *self,
                                                               PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_video_frame_lock_buf(PyObject *self,
                                                              PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_video_frame_unlock_buf(PyObject *self,
                                                                PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_video_frame_get_buf(PyObject *self,
                                                             PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_video_frame_get_width(PyObject *self,
                                                               PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_video_frame_set_width(PyObject *self,
                                                               PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_video_frame_get_height(PyObject *self,
                                                                PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_video_frame_set_height(PyObject *self,
                                                                PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_video_frame_get_timestamp(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_video_frame_set_timestamp(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_video_frame_is_eof(PyObject *self,
                                                            PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_video_frame_set_eof(PyObject *self,
                                                             PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_video_frame_get_pixel_fmt(
    PyObject *self, PyObject *args);

axis_RUNTIME_PRIVATE_API PyObject *axis_py_video_frame_set_pixel_fmt(
    PyObject *self, PyObject *args);
