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

#include "axis_utils/lib/buf.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/smart_ptr.h"

// Note: To achieve the best compatibility, any new enum item, should be added
// to the end to avoid changing the value of previous enum items.
typedef enum axis_AUDIO_FRAME_DATA_FMT {
  axis_AUDIO_FRAME_DATA_FMT_INVALID,

  // Packet format in FFmpeg. Ex: ABABABAB
  axis_AUDIO_FRAME_DATA_FMT_INTERLEAVE,

  // Planar format in FFmpeg. Ex: AAAABBBB
  axis_AUDIO_FRAME_DATA_FMT_NON_INTERLEAVE,
} axis_AUDIO_FRAME_DATA_FMT;

typedef struct axis_audio_frame_t axis_audio_frame_t;

axis_RUNTIME_API axis_shared_ptr_t *axis_audio_frame_create(const char *name,
                                                         axis_error_t *err);

axis_RUNTIME_API int64_t axis_audio_frame_get_timestamp(axis_shared_ptr_t *self);
axis_RUNTIME_API bool axis_audio_frame_set_timestamp(axis_shared_ptr_t *self,
                                                   int64_t timestamp);

axis_RUNTIME_API int32_t axis_audio_frame_get_sample_rate(axis_shared_ptr_t *self);
axis_RUNTIME_API bool axis_audio_frame_set_sample_rate(axis_shared_ptr_t *self,
                                                     int32_t sample_rate);

axis_RUNTIME_API uint64_t
axis_audio_frame_get_channel_layout(axis_shared_ptr_t *self);
axis_RUNTIME_API bool axis_audio_frame_set_channel_layout(
    axis_shared_ptr_t *self, uint64_t channel_layout);

axis_RUNTIME_API bool axis_audio_frame_is_eof(axis_shared_ptr_t *self);
axis_RUNTIME_API bool axis_audio_frame_set_eof(axis_shared_ptr_t *self,
                                             bool is_eof);

axis_RUNTIME_API int32_t
axis_audio_frame_get_samples_per_channel(axis_shared_ptr_t *self);
axis_RUNTIME_API bool axis_audio_frame_set_samples_per_channel(
    axis_shared_ptr_t *self, int32_t samples_per_channel);

axis_RUNTIME_API int32_t axis_audio_frame_get_line_size(axis_shared_ptr_t *self);
axis_RUNTIME_API bool axis_audio_frame_set_line_size(axis_shared_ptr_t *self,
                                                   int32_t line_size);

axis_RUNTIME_API int32_t
axis_audio_frame_get_bytes_per_sample(axis_shared_ptr_t *self);
axis_RUNTIME_API bool axis_audio_frame_set_bytes_per_sample(
    axis_shared_ptr_t *self, int32_t size);

axis_RUNTIME_API int32_t
axis_audio_frame_get_number_of_channel(axis_shared_ptr_t *self);
axis_RUNTIME_API bool axis_audio_frame_set_number_of_channel(
    axis_shared_ptr_t *self, int32_t number);

axis_RUNTIME_API axis_AUDIO_FRAME_DATA_FMT
axis_audio_frame_get_data_fmt(axis_shared_ptr_t *self);
axis_RUNTIME_API bool axis_audio_frame_set_data_fmt(
    axis_shared_ptr_t *self, axis_AUDIO_FRAME_DATA_FMT data_fmt);

axis_RUNTIME_API uint8_t *axis_audio_frame_alloc_buf(axis_shared_ptr_t *self,
                                                   size_t size);

axis_RUNTIME_API axis_buf_t *axis_audio_frame_peek_buf(axis_shared_ptr_t *self);
