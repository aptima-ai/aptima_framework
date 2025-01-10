//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_runtime/binding/go/interface/aptima/audio_frame.h"

#include <stdint.h>

#include "include_internal/axis_runtime/binding/go/internal/common.h"
#include "include_internal/axis_runtime/binding/go/msg/msg.h"
#include "include_internal/axis_runtime/msg/audio_frame/audio_frame.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "axis_runtime/binding/go/interface/aptima/msg.h"
#include "axis_runtime/common/errno.h"
#include "axis_runtime/msg/msg.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/macro/check.h"

axis_go_error_t axis_go_audio_frame_create(const void *name, int name_len,
                                         uintptr_t *bridge_addr) {
  axis_ASSERT(bridge_addr, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_shared_ptr_t *c_audio_frame =
      axis_audio_frame_create_with_name_len(name, name_len, NULL);
  axis_ASSERT(c_audio_frame, "Should not happen.");

  axis_go_msg_t *bridge = axis_go_msg_create(c_audio_frame);
  *bridge_addr = (uintptr_t)bridge;

  // The ownership of the C message instance is transferred into the GO message
  // instance.
  axis_shared_ptr_destroy(c_audio_frame);

  return cgo_error;
}

axis_go_error_t axis_go_audio_frame_set_timestamp(uintptr_t bridge_addr,
                                                int64_t timestamp) {
  axis_ASSERT(bridge_addr, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");

  axis_shared_ptr_t *c_audio_frame = axis_go_msg_c_msg(self);
  axis_audio_frame_set_timestamp(c_audio_frame, timestamp);

  return cgo_error;
}

axis_go_error_t axis_go_audio_frame_get_timestamp(uintptr_t bridge_addr,
                                                int64_t *timestamp) {
  axis_ASSERT(bridge_addr && timestamp, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");

  axis_shared_ptr_t *c_audio_frame = axis_go_msg_c_msg(self);
  *timestamp = axis_audio_frame_get_timestamp(c_audio_frame);

  return cgo_error;
}

axis_go_error_t axis_go_audio_frame_set_sample_rate(uintptr_t bridge_addr,
                                                  int32_t sample_rate) {
  axis_ASSERT(bridge_addr, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");

  axis_shared_ptr_t *c_audio_frame = axis_go_msg_c_msg(self);
  axis_audio_frame_set_sample_rate(c_audio_frame, sample_rate);

  return cgo_error;
}

axis_go_error_t axis_go_audio_frame_get_sample_rate(uintptr_t bridge_addr,
                                                  int32_t *sample_rate) {
  axis_ASSERT(bridge_addr && sample_rate, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");

  axis_shared_ptr_t *c_audio_frame = axis_go_msg_c_msg(self);
  *sample_rate = axis_audio_frame_get_sample_rate(c_audio_frame);

  return cgo_error;
}

axis_go_error_t axis_go_audio_frame_set_channel_layout(uintptr_t bridge_addr,
                                                     uint64_t channel_layout) {
  axis_ASSERT(bridge_addr, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");

  axis_shared_ptr_t *c_audio_frame = axis_go_msg_c_msg(self);
  axis_audio_frame_set_channel_layout(c_audio_frame, channel_layout);

  return cgo_error;
}

axis_go_error_t axis_go_audio_frame_get_channel_layout(uintptr_t bridge_addr,
                                                     uint64_t *channel_layout) {
  axis_ASSERT(bridge_addr && channel_layout, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");

  axis_shared_ptr_t *c_audio_frame = axis_go_msg_c_msg(self);
  *channel_layout = axis_audio_frame_get_channel_layout(c_audio_frame);

  return cgo_error;
}

axis_go_error_t axis_go_audio_frame_set_samples_per_channel(
    uintptr_t bridge_addr, int32_t samples_per_channel) {
  axis_ASSERT(bridge_addr, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");

  axis_shared_ptr_t *c_audio_frame = axis_go_msg_c_msg(self);
  axis_audio_frame_set_samples_per_channel(c_audio_frame, samples_per_channel);

  return cgo_error;
}

axis_go_error_t axis_go_audio_frame_get_samples_per_channel(
    uintptr_t bridge_addr, int32_t *samples_per_channel) {
  axis_ASSERT(bridge_addr && samples_per_channel, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");

  axis_shared_ptr_t *c_audio_frame = axis_go_msg_c_msg(self);
  *samples_per_channel = axis_audio_frame_get_samples_per_channel(c_audio_frame);

  return cgo_error;
}

axis_go_error_t axis_go_audio_frame_set_bytes_per_sample(
    uintptr_t bridge_addr, int32_t bytes_per_sample) {
  axis_ASSERT(bridge_addr, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");

  axis_shared_ptr_t *c_audio_frame = axis_go_msg_c_msg(self);
  axis_audio_frame_set_bytes_per_sample(c_audio_frame, bytes_per_sample);

  return cgo_error;
}

axis_go_error_t axis_go_audio_frame_get_bytes_per_sample(
    uintptr_t bridge_addr, int32_t *bytes_per_sample) {
  axis_ASSERT(bridge_addr && bytes_per_sample, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");

  axis_shared_ptr_t *c_audio_frame = axis_go_msg_c_msg(self);
  *bytes_per_sample = axis_audio_frame_get_bytes_per_sample(c_audio_frame);

  return cgo_error;
}

axis_go_error_t axis_go_audio_frame_set_number_of_channels(
    uintptr_t bridge_addr, int32_t number_of_channels) {
  axis_ASSERT(bridge_addr, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");

  axis_shared_ptr_t *c_audio_frame = axis_go_msg_c_msg(self);
  axis_audio_frame_set_number_of_channel(c_audio_frame, number_of_channels);

  return cgo_error;
}

axis_go_error_t axis_go_audio_frame_get_number_of_channels(
    uintptr_t bridge_addr, int32_t *number_of_channels) {
  axis_ASSERT(bridge_addr && number_of_channels, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");

  axis_shared_ptr_t *c_audio_frame = axis_go_msg_c_msg(self);
  *number_of_channels = axis_audio_frame_get_number_of_channel(c_audio_frame);

  return cgo_error;
}

axis_go_error_t axis_go_audio_frame_set_data_fmt(uintptr_t bridge_addr,
                                               uint32_t fmt) {
  axis_ASSERT(bridge_addr, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");

  axis_shared_ptr_t *c_audio_frame = axis_go_msg_c_msg(self);
  axis_audio_frame_set_data_fmt(c_audio_frame, (axis_AUDIO_FRAME_DATA_FMT)fmt);

  return cgo_error;
}

axis_go_error_t axis_go_audio_frame_get_data_fmt(uintptr_t bridge_addr,
                                               uint32_t *fmt) {
  axis_ASSERT(bridge_addr && fmt, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_ASSERT(fmt, "Invalid argument.");

  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");

  axis_shared_ptr_t *c_audio_frame = axis_go_msg_c_msg(self);
  *fmt = (uint8_t)axis_audio_frame_get_data_fmt(c_audio_frame);

  return cgo_error;
}

axis_go_error_t axis_go_audio_frame_set_line_size(uintptr_t bridge_addr,
                                                int32_t line_size) {
  axis_ASSERT(bridge_addr, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");

  axis_shared_ptr_t *c_audio_frame = axis_go_msg_c_msg(self);
  axis_audio_frame_set_line_size(c_audio_frame, line_size);

  return cgo_error;
}

axis_go_error_t axis_go_audio_frame_get_line_size(uintptr_t bridge_addr,
                                                int32_t *line_size) {
  axis_ASSERT(bridge_addr && line_size, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_ASSERT(line_size, "Invalid argument.");

  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");

  axis_shared_ptr_t *c_audio_frame = axis_go_msg_c_msg(self);
  *line_size = axis_audio_frame_get_line_size(c_audio_frame);

  return cgo_error;
}

axis_go_error_t axis_go_audio_frame_set_eof(uintptr_t bridge_addr, bool is_eof) {
  axis_ASSERT(bridge_addr, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");

  axis_audio_frame_set_eof(axis_go_msg_c_msg(self), is_eof);

  return cgo_error;
}

axis_go_error_t axis_go_audio_frame_is_eof(uintptr_t bridge_addr, bool *is_eof) {
  axis_ASSERT(bridge_addr && is_eof, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_ASSERT(is_eof, "Invalid argument.");

  *is_eof = axis_audio_frame_is_eof(
      axis_go_msg_c_msg(axis_go_msg_reinterpret(bridge_addr)));

  return cgo_error;
}

axis_go_error_t axis_go_audio_frame_alloc_buf(uintptr_t bridge_addr, int size) {
  axis_ASSERT(bridge_addr && size, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Invalid argument.");

  axis_shared_ptr_t *c_audio_frame = axis_go_msg_c_msg(self);
  uint8_t *data = axis_audio_frame_alloc_buf(c_audio_frame, size);
  if (!data) {
    axis_go_error_set(&cgo_error, axis_ERRNO_GENERIC,
                     "failed to allocate memory");
  }

  return cgo_error;
}

axis_go_error_t axis_go_audio_frame_lock_buf(uintptr_t bridge_addr,
                                           uint8_t **buf_addr,
                                           uint64_t *buf_size) {
  axis_ASSERT(bridge_addr && buf_addr && buf_size, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_ASSERT(buf_addr && buf_size, "Invalid argument.");

  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Invalid argument.");

  axis_shared_ptr_t *c_audio_frame = axis_go_msg_c_msg(self);

  axis_buf_t *c_audio_frame_data = axis_audio_frame_peek_buf(c_audio_frame);

  axis_error_t c_err;
  axis_error_init(&c_err);

  if (!axis_msg_add_locked_res_buf(c_audio_frame, c_audio_frame_data->data,
                                  &c_err)) {
    axis_go_error_set(&cgo_error, axis_error_errno(&c_err),
                     axis_error_errmsg(&c_err));
  } else {
    *buf_addr = c_audio_frame_data->data;
    *buf_size = c_audio_frame_data->size;
  }

  axis_error_deinit(&c_err);

  return cgo_error;
}

axis_go_error_t axis_go_audio_frame_unlock_buf(uintptr_t bridge_addr,
                                             const void *buf_addr) {
  axis_ASSERT(bridge_addr && buf_addr, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Invalid argument.");

  axis_shared_ptr_t *c_audio_frame = axis_go_msg_c_msg(self);

  axis_error_t c_err;
  axis_error_init(&c_err);

  bool result = axis_msg_remove_locked_res_buf(c_audio_frame, buf_addr, &c_err);
  if (!result) {
    axis_go_error_set(&cgo_error, axis_error_errno(&c_err),
                     axis_error_errmsg(&c_err));
  }

  axis_error_deinit(&c_err);

  return cgo_error;
}

axis_go_error_t axis_go_audio_frame_get_buf(uintptr_t bridge_addr,
                                          const void *buf_addr,
                                          uint64_t buf_size) {
  axis_ASSERT(bridge_addr && buf_addr && buf_size > 0, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_msg_t *audio_frame_bridge = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(
      audio_frame_bridge && axis_go_msg_check_integrity(audio_frame_bridge),
      "Invalid argument.");

  axis_shared_ptr_t *c_audio_frame = axis_go_msg_c_msg(audio_frame_bridge);
  uint64_t size = axis_audio_frame_peek_buf(c_audio_frame)->size;
  if (buf_size < size) {
    axis_go_error_set(&cgo_error, axis_ERRNO_GENERIC, "buffer is not enough");
  } else {
    axis_buf_t *data = axis_audio_frame_peek_buf(c_audio_frame);
    memcpy((void *)buf_addr, data->data, size);
  }

  return cgo_error;
}

axis_go_error_t axis_go_audio_frame_get_buf_size(uintptr_t bridge_addr,
                                               uint64_t *buf_size) {
  axis_ASSERT(bridge_addr && buf_size, "Invalid argument.");
  axis_ASSERT(buf_size, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_msg_t *audio_frame_bridge = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(
      audio_frame_bridge && axis_go_msg_check_integrity(audio_frame_bridge),
      "Invalid argument.");

  axis_shared_ptr_t *c_audio_frame = axis_go_msg_c_msg(audio_frame_bridge);
  *buf_size = axis_audio_frame_peek_buf(c_audio_frame)->size;

  return cgo_error;
}
