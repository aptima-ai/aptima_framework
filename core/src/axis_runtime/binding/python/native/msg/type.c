//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/binding/python/common/buf.h"
#include "include_internal/axis_runtime/binding/python/common/error.h"
#include "include_internal/axis_runtime/binding/python/msg/audio_frame.h"
#include "include_internal/axis_runtime/binding/python/msg/cmd.h"
#include "include_internal/axis_runtime/binding/python/msg/cmd_result.h"
#include "include_internal/axis_runtime/binding/python/msg/data.h"
#include "include_internal/axis_runtime/binding/python/msg/msg.h"
#include "include_internal/axis_runtime/binding/python/msg/video_frame.h"

static PyMethodDef axis_py_msg_type_methods[] = {
    {"get_name", axis_py_msg_get_name, METH_VARARGS, NULL},
    {"set_name", axis_py_msg_set_name, METH_VARARGS, NULL},
    {"set_dest", axis_py_msg_set_dest, METH_VARARGS, NULL},
    {"get_property_to_json", axis_py_msg_get_property_to_json, METH_VARARGS,
     NULL},
    {"set_property_from_json", axis_py_msg_set_property_from_json, METH_VARARGS,
     NULL},
    {"get_property_string", axis_py_msg_get_property_string, METH_VARARGS, NULL},
    {"set_property_string", axis_py_msg_set_property_string, METH_VARARGS, NULL},
    {"get_property_int", axis_py_msg_get_property_int, METH_VARARGS, NULL},
    {"set_property_int", axis_py_msg_set_property_int, METH_VARARGS, NULL},
    {"get_property_float", axis_py_msg_get_property_float, METH_VARARGS, NULL},
    {"set_property_float", axis_py_msg_set_property_float, METH_VARARGS, NULL},
    {"get_property_bool", axis_py_msg_get_property_bool, METH_VARARGS, NULL},
    {"set_property_bool", axis_py_msg_set_property_bool, METH_VARARGS, NULL},
    {"get_property_buf", axis_py_msg_get_property_buf, METH_VARARGS, NULL},
    {"set_property_buf", axis_py_msg_set_property_buf, METH_VARARGS, NULL},
    {NULL, NULL, 0, NULL},
};

static PyTypeObject axis_py_msg_type_internal = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "libaxis_runtime_python._Msg",
    .tp_doc = PyDoc_STR("_Msg"),
    .tp_basicsize = sizeof(axis_py_msg_t),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,

    // The message instance will be created by subclass.
    .tp_new = NULL,

    .tp_init = NULL,
    .tp_dealloc = NULL,
    .tp_getset = NULL,
    .tp_methods = axis_py_msg_type_methods,
};

PyTypeObject *axis_py_msg_py_type(void) { return &axis_py_msg_type_internal; }

PyTypeObject *axis_py_cmd_py_type(void) {
  static PyTypeObject ty = {
      PyVarObject_HEAD_INIT(NULL, 0).tp_name = "libaxis_runtime_python._Cmd",
      .tp_doc = PyDoc_STR("_Cmd"),
      .tp_basicsize = sizeof(axis_py_cmd_t),
      .tp_itemsize = 0,
      .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
      .tp_base = &axis_py_msg_type_internal,
      .tp_new = axis_py_cmd_create,
      .tp_init = NULL,
      .tp_dealloc = axis_py_cmd_destroy,
      .tp_getset = NULL,
      .tp_methods = NULL,
  };

  return &ty;
}

PyTypeObject *axis_py_cmd_result_py_type(void) {
  static PyMethodDef py_methods[] = {
      {"get_status_code", axis_py_cmd_result_get_status_code, METH_VARARGS,
       NULL},
      {"set_status_code", axis_py_cmd_result_set_status_code, METH_VARARGS,
       NULL},
      {"set_final", axis_py_cmd_result_set_final, METH_VARARGS, NULL},
      {"is_final", axis_py_cmd_result_is_final, METH_VARARGS, NULL},
      {"is_completed", axis_py_cmd_result_is_completed, METH_VARARGS, NULL},
      {NULL, NULL, 0, NULL},
  };

  static PyTypeObject py_type = {
      PyVarObject_HEAD_INIT(NULL, 0).tp_name =
          "libaxis_runtime_python._CmdResult",
      .tp_doc = PyDoc_STR("CmdResult"),
      .tp_basicsize = sizeof(axis_py_cmd_result_t),
      .tp_itemsize = 0,
      .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
      .tp_base = &axis_py_msg_type_internal,
      .tp_new = axis_py_cmd_result_create,
      .tp_init = NULL,
      .tp_dealloc = axis_py_cmd_result_destroy,
      .tp_getset = NULL,
      .tp_methods = py_methods,
  };

  return &py_type;
}

PyTypeObject *axis_py_data_py_type(void) {
  static PyMethodDef py_methods[] = {
      {"alloc_buf", axis_py_data_alloc_buf, METH_VARARGS, NULL},
      {"lock_buf", axis_py_data_lock_buf, METH_VARARGS, NULL},
      {"unlock_buf", axis_py_data_unlock_buf, METH_VARARGS, NULL},
      {"get_buf", axis_py_data_get_buf, METH_VARARGS, NULL},
      {NULL, NULL, 0, NULL},
  };

  static PyTypeObject py_type = {
      PyVarObject_HEAD_INIT(NULL, 0).tp_name = "libaxis_runtime_python._Data",
      .tp_doc = PyDoc_STR("_Data"),
      .tp_basicsize = sizeof(axis_py_data_t),
      .tp_itemsize = 0,
      .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
      .tp_base = &axis_py_msg_type_internal,
      .tp_new = axis_py_data_create,
      .tp_init = NULL,
      .tp_dealloc = axis_py_data_destroy,
      .tp_getset = NULL,
      .tp_methods = py_methods,
  };

  return &py_type;
}

PyTypeObject *axis_py_video_frame_py_type(void) {
  static PyMethodDef py_methods[] = {
      {"alloc_buf", axis_py_video_frame_alloc_buf, METH_VARARGS, NULL},
      {"lock_buf", axis_py_video_frame_lock_buf, METH_VARARGS, NULL},
      {"unlock_buf", axis_py_video_frame_unlock_buf, METH_VARARGS, NULL},
      {"get_buf", axis_py_video_frame_get_buf, METH_VARARGS, NULL},
      {"get_width", axis_py_video_frame_get_width, METH_VARARGS, NULL},
      {"set_width", axis_py_video_frame_set_width, METH_VARARGS, NULL},
      {"get_height", axis_py_video_frame_get_height, METH_VARARGS, NULL},
      {"set_height", axis_py_video_frame_set_height, METH_VARARGS, NULL},
      {"get_timestamp", axis_py_video_frame_get_timestamp, METH_VARARGS, NULL},
      {"set_timestamp", axis_py_video_frame_set_timestamp, METH_VARARGS, NULL},
      {"is_eof", axis_py_video_frame_is_eof, METH_VARARGS, NULL},
      {"set_eof", axis_py_video_frame_set_eof, METH_VARARGS, NULL},
      {"get_pixel_fmt", axis_py_video_frame_get_pixel_fmt, METH_VARARGS, NULL},
      {"set_pixel_fmt", axis_py_video_frame_set_pixel_fmt, METH_VARARGS, NULL},
      {NULL, NULL, 0, NULL},
  };

  static PyTypeObject py_type = {
      PyVarObject_HEAD_INIT(NULL, 0).tp_name =
          "libaxis_runtime_python._VideoFrame",
      .tp_doc = PyDoc_STR("_VideoFrame"),
      .tp_basicsize = sizeof(axis_py_video_frame_t),
      .tp_itemsize = 0,
      .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
      .tp_base = &axis_py_msg_type_internal,
      .tp_new = axis_py_video_frame_create,
      .tp_init = NULL,
      .tp_dealloc = axis_py_video_frame_destroy,
      .tp_getset = NULL,
      .tp_methods = py_methods,
  };

  return &py_type;
}

PyTypeObject *axis_py_audio_frame_py_type(void) {
  static PyMethodDef py_methods[] = {
      {"alloc_buf", axis_py_audio_frame_alloc_buf, METH_VARARGS,
       PyDoc_STR("alloc buf")},
      {"lock_buf", axis_py_audio_frame_lock_buf, METH_VARARGS, NULL},
      {"unlock_buf", axis_py_audio_frame_unlock_buf, METH_VARARGS, NULL},
      {"get_buf", axis_py_audio_frame_get_buf, METH_VARARGS, NULL},
      {"get_timestamp", axis_py_audio_frame_get_timestamp, METH_VARARGS, NULL},
      {"set_timestamp", axis_py_audio_frame_set_timestamp, METH_VARARGS, NULL},
      {"get_sample_rate", axis_py_audio_frame_get_sample_rate, METH_VARARGS,
       NULL},
      {"set_sample_rate", axis_py_audio_frame_set_sample_rate, METH_VARARGS,
       NULL},
      {"get_samples_per_channel", axis_py_audio_frame_get_samples_per_channel,
       METH_VARARGS, NULL},
      {"set_samples_per_channel", axis_py_audio_frame_set_samples_per_channel,
       METH_VARARGS, NULL},
      {"get_bytes_per_sample", axis_py_audio_frame_get_bytes_per_sample,
       METH_VARARGS, NULL},
      {"set_bytes_per_sample", axis_py_audio_frame_set_bytes_per_sample,
       METH_VARARGS, NULL},
      {"get_number_of_channels", axis_py_audio_frame_get_number_of_channels,
       METH_VARARGS, NULL},
      {"set_number_of_channels", axis_py_audio_frame_set_number_of_channels,
       METH_VARARGS, NULL},
      {"get_data_fmt", axis_py_audio_frame_get_data_fmt, METH_VARARGS, NULL},
      {"set_data_fmt", axis_py_audio_frame_set_data_fmt, METH_VARARGS, NULL},
      {"get_line_size", axis_py_audio_frame_get_line_size, METH_VARARGS, NULL},
      {"set_line_size", axis_py_audio_frame_set_line_size, METH_VARARGS, NULL},
      {"is_eof", axis_py_audio_frame_is_eof, METH_VARARGS, NULL},
      {"set_eof", axis_py_audio_frame_set_eof, METH_VARARGS, NULL},
      {NULL, NULL, 0, NULL},
  };

  static PyTypeObject py_type = {
      PyVarObject_HEAD_INIT(NULL, 0).tp_name =
          "libaxis_runtime_python._AudioFrame",
      .tp_doc = PyDoc_STR("_AudioFrame"),
      .tp_basicsize = sizeof(axis_py_audio_frame_t),
      .tp_itemsize = 0,
      .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
      .tp_base = &axis_py_msg_type_internal,
      .tp_new = axis_py_audio_frame_create,
      .tp_init = NULL,
      .tp_dealloc = axis_py_audio_frame_destroy,
      .tp_methods = py_methods,
  };

  return &py_type;
}

PyTypeObject *axis_py_buf_py_type(void) {
  static PyBufferProcs py_buffer_procs = {
      .bf_getbuffer = axis_py_buf_get_buffer,
      .bf_releasebuffer = NULL,
  };

  static PyTypeObject py_type = {
      PyVarObject_HEAD_INIT(NULL, 0).tp_name = "libaxis_runtime_python._Buf",
      .tp_doc = PyDoc_STR("_Buf"),
      .tp_basicsize = sizeof(axis_py_buf_t),
      .tp_itemsize = 0,
      .tp_flags = Py_TPFLAGS_DEFAULT,
      .tp_dealloc = axis_py_buf_destroy,
      .tp_as_buffer = &py_buffer_procs,
  };

  return &py_type;
}

PyTypeObject *axis_py_error_py_type(void) {
  static PyMethodDef py_methods[] = {
      {"errno", axis_py_error_get_errno, METH_VARARGS, NULL},
      {"err_msg", axis_py_error_get_errmsg, METH_VARARGS, NULL},
      {NULL, NULL, 0, NULL},
  };

  static PyTypeObject py_type = {
      PyVarObject_HEAD_INIT(NULL, 0).tp_name =
          "libaxis_runtime_python._TenError",
      .tp_doc = PyDoc_STR("_TenError"),
      .tp_basicsize = sizeof(axis_py_error_t),
      .tp_itemsize = 0,
      .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
      .tp_dealloc = axis_py_error_destroy,
      .tp_methods = py_methods,
      .tp_init = NULL,
      .tp_getset = NULL,
      .tp_new = NULL,
  };

  return &py_type;
}
