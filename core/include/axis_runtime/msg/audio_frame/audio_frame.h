//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_runtime/aptima_config.h"

#include <stddef.h>
#include <stdint.h>

#include "aptima_utils/lib/buf.h"
#include "aptima_utils/lib/error.h"
#include "aptima_utils/lib/smart_ptr.h"

// Note: To achieve the best compatibility, any new enum item, should be added
// to the end to avoid changing the value of previous enum items.
typedef enum aptima_AUDIO_FRAME_DATA_FMT {
  aptima_AUDIO_FRAME_DATA_FMT_INVALID,

  // Packet format in FFmpeg. Ex: ABABABAB
  aptima_AUDIO_FRAME_DATA_FMT_INTERLEAVE,

  // Planar format in FFmpeg. Ex: AAAABBBB
  aptima_AUDIO_FRAME_DATA_FMT_NON_INTERLEAVE,
} aptima_AUDIO_FRAME_DATA_FMT;

typedef struct aptima_audio_frame_t aptima_audio_frame_t;

aptima_RUNTIME_API aptima_shared_ptr_t *aptima_audio_frame_create(const char *name,
                                                         aptima_error_t *err);

aptima_RUNTIME_API int64_t aptima_audio_frame_get_timestamp(aptima_shared_ptr_t *self);
aptima_RUNTIME_API bool aptima_audio_frame_set_timestamp(aptima_shared_ptr_t *self,
                                                   int64_t timestamp);

aptima_RUNTIME_API int32_t aptima_audio_frame_get_sample_rate(aptima_shared_ptr_t *self);
aptima_RUNTIME_API bool aptima_audio_frame_set_sample_rate(aptima_shared_ptr_t *self,
                                                     int32_t sample_rate);

aptima_RUNTIME_API uint64_t
aptima_audio_frame_get_channel_layout(aptima_shared_ptr_t *self);
aptima_RUNTIME_API bool aptima_audio_frame_set_channel_layout(
    aptima_shared_ptr_t *self, uint64_t channel_layout);

aptima_RUNTIME_API bool aptima_audio_frame_is_eof(aptima_shared_ptr_t *self);
aptima_RUNTIME_API bool aptima_audio_frame_set_eof(aptima_shared_ptr_t *self,
                                             bool is_eof);

aptima_RUNTIME_API int32_t
aptima_audio_frame_get_samples_per_channel(aptima_shared_ptr_t *self);
aptima_RUNTIME_API bool aptima_audio_frame_set_samples_per_channel(
    aptima_shared_ptr_t *self, int32_t samples_per_channel);

aptima_RUNTIME_API int32_t aptima_audio_frame_get_line_size(aptima_shared_ptr_t *self);
aptima_RUNTIME_API bool aptima_audio_frame_set_line_size(aptima_shared_ptr_t *self,
                                                   int32_t line_size);

aptima_RUNTIME_API int32_t
aptima_audio_frame_get_bytes_per_sample(aptima_shared_ptr_t *self);
aptima_RUNTIME_API bool aptima_audio_frame_set_bytes_per_sample(
    aptima_shared_ptr_t *self, int32_t size);

aptima_RUNTIME_API int32_t
aptima_audio_frame_get_number_of_channel(aptima_shared_ptr_t *self);
aptima_RUNTIME_API bool aptima_audio_frame_set_number_of_channel(
    aptima_shared_ptr_t *self, int32_t number);

aptima_RUNTIME_API aptima_AUDIO_FRAME_DATA_FMT
aptima_audio_frame_get_data_fmt(aptima_shared_ptr_t *self);
aptima_RUNTIME_API bool aptima_audio_frame_set_data_fmt(
    aptima_shared_ptr_t *self, aptima_AUDIO_FRAME_DATA_FMT data_fmt);

aptima_RUNTIME_API uint8_t *aptima_audio_frame_alloc_buf(aptima_shared_ptr_t *self,
                                                   size_t size);

aptima_RUNTIME_API aptima_buf_t *aptima_audio_frame_peek_buf(aptima_shared_ptr_t *self);
