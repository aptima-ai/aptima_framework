//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdint.h>

#include "include_internal/axis_runtime/msg/msg.h"
#include "axis_runtime/msg/video_frame/video_frame.h"

#define axis_VIDEO_FRAME_SIGNATURE 0xD03493E95807AC78U

typedef struct axis_video_frame_payload_t {
} axis_video_frame_payload_t;

typedef struct axis_video_frame_t {
  axis_msg_t msg_hdr;
  axis_signature_t signature;

  axis_value_t pixel_fmt;  // int32 (axis_PIXEL_FMT)
  axis_value_t timestamp;  // int64
  axis_value_t width;      // int32
  axis_value_t height;     // int32
  axis_value_t is_eof;     // bool
  axis_value_t data;       // buf
} axis_video_frame_t;

axis_RUNTIME_PRIVATE_API axis_shared_ptr_t *axis_video_frame_create_empty(void);

axis_RUNTIME_API axis_shared_ptr_t *axis_video_frame_create_with_name_len(
    const char *name, size_t name_len, axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool axis_raw_video_frame_check_integrity(
    axis_video_frame_t *self);

axis_RUNTIME_API axis_video_frame_payload_t *axis_raw_video_frame_raw_payload(
    axis_video_frame_t *self);

axis_RUNTIME_PRIVATE_API axis_msg_t *axis_raw_video_frame_as_msg_clone(
    axis_msg_t *self, axis_list_t *excluded_field_ids);

axis_RUNTIME_PRIVATE_API void axis_raw_video_frame_init(axis_video_frame_t *self);

axis_RUNTIME_PRIVATE_API void axis_raw_video_frame_destroy(
    axis_video_frame_t *self);

axis_RUNTIME_PRIVATE_API axis_PIXEL_FMT
axis_raw_video_frame_get_pixel_fmt(axis_video_frame_t *self);

axis_RUNTIME_PRIVATE_API bool axis_raw_video_frame_set_pixel_fmt(
    axis_video_frame_t *self, axis_PIXEL_FMT pixel_fmt);

axis_RUNTIME_PRIVATE_API int64_t
axis_raw_video_frame_get_timestamp(axis_video_frame_t *self);

axis_RUNTIME_PRIVATE_API int32_t
axis_raw_video_frame_get_width(axis_video_frame_t *self);

axis_RUNTIME_PRIVATE_API int32_t
axis_raw_video_frame_get_height(axis_video_frame_t *self);

axis_RUNTIME_PRIVATE_API size_t
axis_raw_video_frame_get_data_size(axis_video_frame_t *self);

axis_RUNTIME_PRIVATE_API bool axis_raw_video_frame_is_eof(
    axis_video_frame_t *self);

axis_RUNTIME_PRIVATE_API bool axis_raw_video_frame_set_width(
    axis_video_frame_t *self, int32_t width);

axis_RUNTIME_PRIVATE_API bool axis_raw_video_frame_set_height(
    axis_video_frame_t *self, int32_t height);

axis_RUNTIME_PRIVATE_API bool axis_raw_video_frame_set_timestamp(
    axis_video_frame_t *self, int64_t timestamp);

axis_RUNTIME_PRIVATE_API bool axis_raw_video_frame_set_eof(
    axis_video_frame_t *self, bool is_eof);

axis_RUNTIME_PRIVATE_API bool axis_raw_video_frame_set_axis_property(
    axis_msg_t *self, axis_list_t *paths, axis_value_t *value, axis_error_t *err);

axis_RUNTIME_PRIVATE_API axis_value_t *axis_raw_video_frame_peek_axis_property(
    axis_msg_t *self, axis_list_t *paths, axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool axis_raw_video_frame_loop_all_fields(
    axis_msg_t *self, axis_raw_msg_process_one_field_func_t cb, void *user_data,
    axis_error_t *err);
