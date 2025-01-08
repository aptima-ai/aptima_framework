//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stddef.h>
#include <stdint.h>

#include "axis_utils/lib/error.h"
#include "axis_utils/lib/smart_ptr.h"

// Note: To achieve the best compatibility, any new enum item, should be added
// to the end to avoid changing the value of previous enum items.
typedef enum axis_PIXEL_FMT {
  axis_PIXEL_FMT_INVALID,

  axis_PIXEL_FMT_RGB24,
  axis_PIXEL_FMT_RGBA,

  axis_PIXEL_FMT_BGR24,
  axis_PIXEL_FMT_BGRA,

  axis_PIXEL_FMT_I422,
  axis_PIXEL_FMT_I420,

  axis_PIXEL_FMT_NV21,
  axis_PIXEL_FMT_NV12,
} axis_PIXEL_FMT;

typedef struct axis_video_frame_t axis_video_frame_t;

axis_RUNTIME_API axis_shared_ptr_t *axis_video_frame_create(const char *name,
                                                         axis_error_t *err);

axis_RUNTIME_API int32_t axis_video_frame_get_width(axis_shared_ptr_t *self);
axis_RUNTIME_API bool axis_video_frame_set_width(axis_shared_ptr_t *self,
                                               int32_t width);

axis_RUNTIME_API int32_t axis_video_frame_get_height(axis_shared_ptr_t *self);
axis_RUNTIME_API bool axis_video_frame_set_height(axis_shared_ptr_t *self,
                                                int32_t height);

axis_RUNTIME_API int64_t axis_video_frame_get_timestamp(axis_shared_ptr_t *self);
axis_RUNTIME_API bool axis_video_frame_set_timestamp(axis_shared_ptr_t *self,
                                                   int64_t timestamp);

axis_RUNTIME_API bool axis_video_frame_is_eof(axis_shared_ptr_t *self);
axis_RUNTIME_API bool axis_video_frame_set_eof(axis_shared_ptr_t *self,
                                             bool is_eof);

axis_RUNTIME_API axis_PIXEL_FMT
axis_video_frame_get_pixel_fmt(axis_shared_ptr_t *self);

axis_RUNTIME_API bool axis_video_frame_set_pixel_fmt(axis_shared_ptr_t *self,
                                                   axis_PIXEL_FMT type);

axis_RUNTIME_API uint8_t *axis_video_frame_alloc_data(axis_shared_ptr_t *self,
                                                    size_t size);

axis_RUNTIME_API axis_buf_t *axis_video_frame_peek_buf(axis_shared_ptr_t *self);
