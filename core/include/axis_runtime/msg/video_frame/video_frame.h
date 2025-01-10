//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_runtime/aptima_config.h"

#include <stddef.h>
#include <stdint.h>

#include "aptima_utils/lib/error.h"
#include "aptima_utils/lib/smart_ptr.h"

// Note: To achieve the best compatibility, any new enum item, should be added
// to the end to avoid changing the value of previous enum items.
typedef enum aptima_PIXEL_FMT {
  aptima_PIXEL_FMT_INVALID,

  aptima_PIXEL_FMT_RGB24,
  aptima_PIXEL_FMT_RGBA,

  aptima_PIXEL_FMT_BGR24,
  aptima_PIXEL_FMT_BGRA,

  aptima_PIXEL_FMT_I422,
  aptima_PIXEL_FMT_I420,

  aptima_PIXEL_FMT_NV21,
  aptima_PIXEL_FMT_NV12,
} aptima_PIXEL_FMT;

typedef struct aptima_video_frame_t aptima_video_frame_t;

aptima_RUNTIME_API aptima_shared_ptr_t *aptima_video_frame_create(const char *name,
                                                         aptima_error_t *err);

aptima_RUNTIME_API int32_t aptima_video_frame_get_width(aptima_shared_ptr_t *self);
aptima_RUNTIME_API bool aptima_video_frame_set_width(aptima_shared_ptr_t *self,
                                               int32_t width);

aptima_RUNTIME_API int32_t aptima_video_frame_get_height(aptima_shared_ptr_t *self);
aptima_RUNTIME_API bool aptima_video_frame_set_height(aptima_shared_ptr_t *self,
                                                int32_t height);

aptima_RUNTIME_API int64_t aptima_video_frame_get_timestamp(aptima_shared_ptr_t *self);
aptima_RUNTIME_API bool aptima_video_frame_set_timestamp(aptima_shared_ptr_t *self,
                                                   int64_t timestamp);

aptima_RUNTIME_API bool aptima_video_frame_is_eof(aptima_shared_ptr_t *self);
aptima_RUNTIME_API bool aptima_video_frame_set_eof(aptima_shared_ptr_t *self,
                                             bool is_eof);

aptima_RUNTIME_API aptima_PIXEL_FMT
aptima_video_frame_get_pixel_fmt(aptima_shared_ptr_t *self);

aptima_RUNTIME_API bool aptima_video_frame_set_pixel_fmt(aptima_shared_ptr_t *self,
                                                   aptima_PIXEL_FMT type);

aptima_RUNTIME_API uint8_t *aptima_video_frame_alloc_data(aptima_shared_ptr_t *self,
                                                    size_t size);

aptima_RUNTIME_API aptima_buf_t *aptima_video_frame_peek_buf(aptima_shared_ptr_t *self);
