//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "common.h"  // IWYU pragma: keep

axis_go_error_t axis_go_video_frame_create(const void *msg_name, int msg_name_len,
                                         uintptr_t *bridge_addr);

axis_go_error_t axis_go_video_frame_alloc_buf(uintptr_t bridge_addr, int size);

axis_go_error_t axis_go_video_frame_lock_buf(uintptr_t bridge_addr,
                                           uint8_t **buf_addr,
                                           uint64_t *buf_size);

axis_go_error_t axis_go_video_frame_unlock_buf(uintptr_t bridge_addr,
                                             const void *buf_addr);

axis_go_error_t axis_go_video_frame_get_buf(uintptr_t bridge_addr,
                                          const void *buf_addr,
                                          uint64_t buf_size);

axis_go_error_t axis_go_video_frame_get_buf_size(uintptr_t bridge_addr,
                                               uint64_t *buf_size);

axis_go_error_t axis_go_video_frame_set_width(uintptr_t bridge_addr,
                                            int32_t width);

axis_go_error_t axis_go_video_frame_get_width(uintptr_t bridge_addr,
                                            int32_t *width);

axis_go_error_t axis_go_video_frame_set_height(uintptr_t bridge_addr,
                                             int32_t height);

axis_go_error_t axis_go_video_frame_get_height(uintptr_t bridge_addr,
                                             int32_t *height);

axis_go_error_t axis_go_video_frame_set_timestamp(uintptr_t bridge_addr,
                                                int64_t timestamp);

axis_go_error_t axis_go_video_frame_get_timestamp(uintptr_t bridge_addr,
                                                int64_t *timestamp);

axis_go_error_t axis_go_video_frame_set_eof(uintptr_t bridge_addr, bool is_eof);

axis_go_error_t axis_go_video_frame_is_eof(uintptr_t bridge_addr, bool *is_eof);

axis_go_error_t axis_go_video_frame_get_pixel_fmt(uintptr_t bridge_addr,
                                                uint32_t *fmt);

axis_go_error_t axis_go_video_frame_set_pixel_fmt(uintptr_t bridge_addr,
                                                uint32_t fmt);
