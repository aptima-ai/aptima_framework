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
#include "axis_runtime/msg/audio_frame/audio_frame.h"

#define axis_AUDIO_FRAME_SIGNATURE 0x356C118642A0FD8FU
#define axis_AUDIO_FRAME_MAX_DATA_CNT 8

typedef struct axis_audio_frame_t {
  axis_msg_t msg_hdr;
  axis_signature_t signature;

  axis_value_t timestamp;    // int64. The timestamp (ms) of the audio frame.
  axis_value_t sample_rate;  // int32. The sample rate (Hz) of the pcm data.
  axis_value_t bytes_per_sample;     // int32. 1,2,4,8 bytes
  axis_value_t samples_per_channel;  // int32. The number of samples per channel.
  axis_value_t number_of_channel;    // int32. The channel number.

  // channel layout ID of FFmpeg. Please see 'channel_layout_map' of
  // https://github.com/FFmpeg/FFmpeg/blob/master/libavutil/channel_layout.c for
  // the values of the channel layout ID.
  // If this audio frame is from one FFmpeg software re-sampler and will be
  // consumed by another FFmpeg software re-sampler, you need to remember the
  // channel layout of the audio frame, so that it could be used by those
  // software re-sampler. However, if the audio frame is nothing to do with the
  // FFmpeg software re-sampler, you could just ignore this field.
  axis_value_t channel_layout;  // uint64

  axis_value_t data_fmt;  // int32 (axis_AUDIO_FRAME_DATA_FMT). Format of `data`.

  axis_value_t buf;  // buf

  // TODO(Liu): Add data size info for each channel.
  //
  // line_size is the size of data[i] if data[i] is not NULL, i is from 0 to
  // (axis_AUDIO_FRAME_MAX_DATA_CNT-1).
  // - If "format" is interleave, only data[0] is used, and line_size is the
  //   size of the memory space pointed by data[0], and it should be equal to
  //   "bytes_per_sample * samples_per_channel * number_of_channel".
  // - If "format" is non-interleave,
  //   data[0]~data[axis_AUDIO_FRAME_MAX_DATA_CNT-1] might be used, and line_size
  //   is the size of each memory space pointed to by data[i] if data[i] is not
  //   NULL, i is from 0 to (axis_AUDIO_FRAME_MAX_DATA_CNT-1), and it should be
  //   equal to "bytes_per_sample * samples_per_channel".
  axis_value_t line_size;  // int32

  axis_value_t is_eof;  // bool
} axis_audio_frame_t;

axis_RUNTIME_PRIVATE_API axis_shared_ptr_t *axis_audio_frame_create_empty(void);

axis_RUNTIME_API axis_shared_ptr_t *axis_audio_frame_create_with_name_len(
    const char *name, size_t name_len, axis_error_t *err);

axis_RUNTIME_PRIVATE_API void axis_raw_audio_frame_destroy(
    axis_audio_frame_t *self);

axis_RUNTIME_PRIVATE_API axis_msg_t *axis_raw_audio_frame_as_msg_clone(
    axis_msg_t *self, axis_list_t *excluded_field_ids);

axis_RUNTIME_PRIVATE_API int32_t
axis_raw_audio_frame_get_samples_per_channel(axis_audio_frame_t *self);

axis_RUNTIME_PRIVATE_API axis_buf_t *axis_raw_audio_frame_peek_buf(
    axis_audio_frame_t *self);

axis_RUNTIME_PRIVATE_API int32_t
axis_raw_audio_frame_get_sample_rate(axis_audio_frame_t *self);

axis_RUNTIME_PRIVATE_API uint64_t
axis_raw_audio_frame_get_channel_layout(axis_audio_frame_t *self);

axis_RUNTIME_PRIVATE_API bool axis_raw_audio_frame_is_eof(
    axis_audio_frame_t *self);

axis_RUNTIME_PRIVATE_API int32_t
axis_raw_audio_frame_get_line_size(axis_audio_frame_t *self);

axis_RUNTIME_PRIVATE_API int32_t
axis_raw_audio_frame_get_bytes_per_sample(axis_audio_frame_t *self);

axis_RUNTIME_PRIVATE_API int32_t
axis_raw_audio_frame_get_number_of_channel(axis_audio_frame_t *self);

axis_RUNTIME_PRIVATE_API axis_AUDIO_FRAME_DATA_FMT
axis_raw_audio_frame_get_data_fmt(axis_audio_frame_t *self);

axis_RUNTIME_PRIVATE_API int64_t
axis_raw_audio_frame_get_timestamp(axis_audio_frame_t *self);

axis_RUNTIME_PRIVATE_API bool axis_raw_audio_frame_set_samples_per_channel(
    axis_audio_frame_t *self, int32_t samples_per_channel);

axis_RUNTIME_PRIVATE_API bool axis_raw_audio_frame_set_sample_rate(
    axis_audio_frame_t *self, int32_t sample_rate);

axis_RUNTIME_PRIVATE_API bool axis_raw_audio_frame_set_channel_layout(
    axis_audio_frame_t *self, uint64_t channel_layout);

axis_RUNTIME_PRIVATE_API bool axis_raw_audio_frame_set_eof(
    axis_audio_frame_t *self, bool is_eof);

axis_RUNTIME_PRIVATE_API bool axis_raw_audio_frame_set_line_size(
    axis_audio_frame_t *self, int32_t line_size);

axis_RUNTIME_PRIVATE_API bool axis_raw_audio_frame_set_bytes_per_sample(
    axis_audio_frame_t *self, int32_t bytes_per_sample);

axis_RUNTIME_PRIVATE_API bool axis_raw_audio_frame_set_number_of_channel(
    axis_audio_frame_t *self, int32_t number);

axis_RUNTIME_PRIVATE_API bool axis_raw_audio_frame_set_data_fmt(
    axis_audio_frame_t *self, axis_AUDIO_FRAME_DATA_FMT data_fmt);

axis_RUNTIME_PRIVATE_API bool axis_raw_audio_frame_set_timestamp(
    axis_audio_frame_t *self, int64_t timestamp);

axis_RUNTIME_PRIVATE_API axis_value_t *axis_raw_audio_frame_peek_axis_property(
    axis_msg_t *self, axis_list_t *paths, axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool axis_raw_audio_frame_loop_all_fields(
    axis_msg_t *self, axis_raw_msg_process_one_field_func_t cb, void *user_data,
    axis_error_t *err);
