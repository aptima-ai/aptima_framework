//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "common.h"

axis_go_error_t axis_go_audio_frame_create(const void *msg_name, int msg_name_len,
                                         uintptr_t *bridge_addr);

axis_go_error_t axis_go_audio_frame_set_timestamp(uintptr_t bridge_addr,
                                                int64_t timestamp);

axis_go_error_t axis_go_audio_frame_get_timestamp(uintptr_t bridge_addr,
                                                int64_t *timestamp);

axis_go_error_t axis_go_audio_frame_set_sample_rate(uintptr_t bridge_addr,
                                                  int32_t sample_rate);

axis_go_error_t axis_go_audio_frame_get_sample_rate(uintptr_t bridge_addr,
                                                  int32_t *sample_rate);

axis_go_error_t axis_go_audio_frame_set_channel_layout(uintptr_t bridge_addr,
                                                     uint64_t channel_layout);

axis_go_error_t axis_go_audio_frame_get_channel_layout(uintptr_t bridge_addr,
                                                     uint64_t *channel_layout);

axis_go_error_t axis_go_audio_frame_set_samples_per_channel(
    uintptr_t bridge_addr, int32_t samples_per_channel);

axis_go_error_t axis_go_audio_frame_get_samples_per_channel(
    uintptr_t bridge_addr, int32_t *samples_per_channel);

axis_go_error_t axis_go_audio_frame_set_bytes_per_sample(
    uintptr_t bridge_addr, int32_t bytes_per_sample);

axis_go_error_t axis_go_audio_frame_get_bytes_per_sample(
    uintptr_t bridge_addr, int32_t *bytes_per_sample);

axis_go_error_t axis_go_audio_frame_set_number_of_channels(
    uintptr_t bridge_addr, int32_t number_of_channels);

axis_go_error_t axis_go_audio_frame_get_number_of_channels(
    uintptr_t bridge_addr, int32_t *number_of_channels);

axis_go_error_t axis_go_audio_frame_set_data_fmt(uintptr_t bridge_addr,
                                               uint32_t fmt);

axis_go_error_t axis_go_audio_frame_get_data_fmt(uintptr_t bridge_addr,
                                               uint32_t *fmt);

axis_go_error_t axis_go_audio_frame_set_line_size(uintptr_t bridge_addr,
                                                int32_t line_size);

axis_go_error_t axis_go_audio_frame_get_line_size(uintptr_t bridge_addr,
                                                int32_t *line_size);

axis_go_error_t axis_go_audio_frame_set_eof(uintptr_t bridge_addr, bool is_eof);

axis_go_error_t axis_go_audio_frame_is_eof(uintptr_t bridge_addr, bool *is_eof);

axis_go_error_t axis_go_audio_frame_alloc_buf(uintptr_t bridge_addr, int size);

axis_go_error_t axis_go_audio_frame_get_buf(uintptr_t bridge_addr,
                                          const void *buf_addr,
                                          uint64_t buf_size);

axis_go_error_t axis_go_audio_frame_lock_buf(uintptr_t bridge_addr,
                                           uint8_t **buf_addr,
                                           uint64_t *buf_size);

axis_go_error_t axis_go_audio_frame_unlock_buf(uintptr_t bridge_addr,
                                             const void *buf_addr);

axis_go_error_t axis_go_audio_frame_get_buf_size(uintptr_t bridge_addr,
                                               uint64_t *buf_size);
