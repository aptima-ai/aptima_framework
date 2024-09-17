//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_runtime/binding/go/interface/ten/audio_frame.h"

#include <stdint.h>

#include "include_internal/ten_runtime/binding/go/internal/common.h"
#include "include_internal/ten_runtime/binding/go/msg/msg.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "ten_runtime/binding/go/interface/ten/msg.h"
#include "ten_runtime/common/errno.h"
#include "ten_runtime/msg/audio_frame/audio_frame.h"
#include "ten_runtime/msg/msg.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/macro/check.h"

ten_go_status_t ten_go_audio_frame_create(const void *msg_name,
                                          int msg_name_len,
                                          uintptr_t *bridge_addr) {
  TEN_ASSERT(bridge_addr, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_shared_ptr_t *c_audio_frame = ten_audio_frame_create();
  TEN_ASSERT(c_audio_frame, "Should not happen.");

  ten_msg_set_name_with_size(c_audio_frame, msg_name, msg_name_len, NULL);

  ten_go_msg_t *bridge = ten_go_msg_create(c_audio_frame);
  *bridge_addr = (uintptr_t)bridge;

  return status;
}

ten_go_status_t ten_go_audio_frame_set_timestamp(uintptr_t bridge_addr,
                                                 int64_t timestamp) {
  TEN_ASSERT(bridge_addr > 0, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");

  ten_shared_ptr_t *c_audio_frame = ten_go_msg_c_msg(self);
  ten_audio_frame_set_timestamp(c_audio_frame, timestamp);

  return status;
}

ten_go_status_t ten_go_audio_frame_get_timestamp(uintptr_t bridge_addr,
                                                 int64_t *timestamp) {
  TEN_ASSERT(bridge_addr > 0 && timestamp, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");

  ten_shared_ptr_t *c_audio_frame = ten_go_msg_c_msg(self);
  *timestamp = ten_audio_frame_get_timestamp(c_audio_frame);

  return status;
}

ten_go_status_t ten_go_audio_frame_set_sample_rate(uintptr_t bridge_addr,
                                                   int32_t sample_rate) {
  TEN_ASSERT(bridge_addr > 0, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");

  ten_shared_ptr_t *c_audio_frame = ten_go_msg_c_msg(self);
  ten_audio_frame_set_sample_rate(c_audio_frame, sample_rate);

  return status;
}

ten_go_status_t ten_go_audio_frame_get_sample_rate(uintptr_t bridge_addr,
                                                   int32_t *sample_rate) {
  TEN_ASSERT(bridge_addr > 0 && sample_rate, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");

  ten_shared_ptr_t *c_audio_frame = ten_go_msg_c_msg(self);
  *sample_rate = ten_audio_frame_get_sample_rate(c_audio_frame);

  return status;
}

ten_go_status_t ten_go_audio_frame_set_channel_layout(uintptr_t bridge_addr,
                                                      uint64_t channel_layout) {
  TEN_ASSERT(bridge_addr > 0, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");

  ten_shared_ptr_t *c_audio_frame = ten_go_msg_c_msg(self);
  ten_audio_frame_set_channel_layout(c_audio_frame, channel_layout);

  return status;
}

ten_go_status_t ten_go_audio_frame_get_channel_layout(
    uintptr_t bridge_addr, uint64_t *channel_layout) {
  TEN_ASSERT(bridge_addr > 0 && channel_layout, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");

  ten_shared_ptr_t *c_audio_frame = ten_go_msg_c_msg(self);
  *channel_layout = ten_audio_frame_get_channel_layout(c_audio_frame);

  return status;
}

ten_go_status_t ten_go_audio_frame_set_samples_per_channel(
    uintptr_t bridge_addr, int32_t samples_per_channel) {
  TEN_ASSERT(bridge_addr > 0, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");

  ten_shared_ptr_t *c_audio_frame = ten_go_msg_c_msg(self);
  ten_audio_frame_set_samples_per_channel(c_audio_frame, samples_per_channel);

  return status;
}

ten_go_status_t ten_go_audio_frame_get_samples_per_channel(
    uintptr_t bridge_addr, int32_t *samples_per_channel) {
  TEN_ASSERT(bridge_addr > 0 && samples_per_channel, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");

  ten_shared_ptr_t *c_audio_frame = ten_go_msg_c_msg(self);
  *samples_per_channel = ten_audio_frame_get_samples_per_channel(c_audio_frame);

  return status;
}

ten_go_status_t ten_go_audio_frame_set_bytes_per_sample(
    uintptr_t bridge_addr, int32_t bytes_per_sample) {
  TEN_ASSERT(bridge_addr > 0, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");

  ten_shared_ptr_t *c_audio_frame = ten_go_msg_c_msg(self);
  ten_audio_frame_set_bytes_per_sample(c_audio_frame, bytes_per_sample);

  return status;
}

ten_go_status_t ten_go_audio_frame_get_bytes_per_sample(
    uintptr_t bridge_addr, int32_t *bytes_per_sample) {
  TEN_ASSERT(bridge_addr > 0 && bytes_per_sample, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");

  ten_shared_ptr_t *c_audio_frame = ten_go_msg_c_msg(self);
  *bytes_per_sample = ten_audio_frame_get_bytes_per_sample(c_audio_frame);

  return status;
}

ten_go_status_t ten_go_audio_frame_set_number_of_channels(
    uintptr_t bridge_addr, int32_t number_of_channels) {
  TEN_ASSERT(bridge_addr > 0, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");

  ten_shared_ptr_t *c_audio_frame = ten_go_msg_c_msg(self);
  ten_audio_frame_set_number_of_channel(c_audio_frame, number_of_channels);

  return status;
}

ten_go_status_t ten_go_audio_frame_get_number_of_channels(
    uintptr_t bridge_addr, int32_t *number_of_channels) {
  TEN_ASSERT(bridge_addr > 0 && number_of_channels, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");

  ten_shared_ptr_t *c_audio_frame = ten_go_msg_c_msg(self);
  *number_of_channels = ten_audio_frame_get_number_of_channel(c_audio_frame);

  return status;
}

ten_go_status_t ten_go_audio_frame_set_data_fmt(uintptr_t bridge_addr,
                                                uint32_t fmt) {
  TEN_ASSERT(bridge_addr > 0, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");

  ten_shared_ptr_t *c_audio_frame = ten_go_msg_c_msg(self);
  ten_audio_frame_set_data_fmt(c_audio_frame, (TEN_AUDIO_FRAME_DATA_FMT)fmt);

  return status;
}

ten_go_status_t ten_go_audio_frame_get_data_fmt(uintptr_t bridge_addr,
                                                uint32_t *fmt) {
  TEN_ASSERT(bridge_addr > 0 && fmt, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  TEN_ASSERT(fmt, "Invalid argument.");

  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");

  ten_shared_ptr_t *c_audio_frame = ten_go_msg_c_msg(self);
  *fmt = (uint8_t)ten_audio_frame_get_data_fmt(c_audio_frame);

  return status;
}

ten_go_status_t ten_go_audio_frame_set_line_size(uintptr_t bridge_addr,
                                                 int32_t line_size) {
  TEN_ASSERT(bridge_addr > 0, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");

  ten_shared_ptr_t *c_audio_frame = ten_go_msg_c_msg(self);
  ten_audio_frame_set_line_size(c_audio_frame, line_size);

  return status;
}

ten_go_status_t ten_go_audio_frame_get_line_size(uintptr_t bridge_addr,
                                                 int32_t *line_size) {
  TEN_ASSERT(bridge_addr > 0 && line_size, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  TEN_ASSERT(line_size, "Invalid argument.");

  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");

  ten_shared_ptr_t *c_audio_frame = ten_go_msg_c_msg(self);
  *line_size = ten_audio_frame_get_line_size(c_audio_frame);

  return status;
}

ten_go_status_t ten_go_audio_frame_set_is_eof(uintptr_t bridge_addr,
                                              bool is_eof) {
  TEN_ASSERT(bridge_addr > 0, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");

  ten_audio_frame_set_is_eof(ten_go_msg_c_msg(self), is_eof);

  return status;
}

ten_go_status_t ten_go_audio_frame_is_eof(uintptr_t bridge_addr, bool *is_eof) {
  TEN_ASSERT(bridge_addr > 0 && is_eof, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  TEN_ASSERT(is_eof, "Invalid argument.");

  *is_eof = ten_audio_frame_is_eof(
      ten_go_msg_c_msg(ten_go_msg_reinterpret(bridge_addr)));

  return status;
}

ten_go_status_t ten_go_audio_frame_alloc_buf(uintptr_t bridge_addr, int size) {
  TEN_ASSERT(bridge_addr > 0 && size > 0, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Invalid argument.");

  ten_shared_ptr_t *c_audio_frame = ten_go_msg_c_msg(self);
  uint8_t *data = ten_audio_frame_alloc_data(c_audio_frame, size);
  if (!data) {
    ten_go_status_set(&status, TEN_ERRNO_GENERIC, "failed to allocate memory");
  }

  return status;
}

ten_go_status_t ten_go_audio_frame_lock_buf(uintptr_t bridge_addr,
                                            uint8_t **buf_addr,
                                            uint64_t *buf_size) {
  TEN_ASSERT(bridge_addr > 0 && buf_addr && buf_size, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  TEN_ASSERT(buf_addr && buf_size, "Invalid argument.");

  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Invalid argument.");

  ten_shared_ptr_t *c_audio_frame = ten_go_msg_c_msg(self);

  ten_buf_t *c_audio_frame_data = ten_audio_frame_peek_data(c_audio_frame);

  ten_error_t c_err;
  ten_error_init(&c_err);

  if (!ten_msg_add_locked_res_buf(c_audio_frame, c_audio_frame_data->data,
                                  &c_err)) {
    ten_go_status_set(&status, ten_error_errno(&c_err),
                      ten_error_errmsg(&c_err));
  } else {
    *buf_addr = c_audio_frame_data->data;
    *buf_size = c_audio_frame_data->size;
  }

  ten_error_deinit(&c_err);

  return status;
}

ten_go_status_t ten_go_audio_frame_unlock_buf(uintptr_t bridge_addr,
                                              const void *buf_addr) {
  TEN_ASSERT(bridge_addr > 0 && buf_addr, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Invalid argument.");

  ten_shared_ptr_t *c_audio_frame = ten_go_msg_c_msg(self);

  ten_error_t c_err;
  ten_error_init(&c_err);

  bool result = ten_msg_remove_locked_res_buf(c_audio_frame, buf_addr, &c_err);
  if (!result) {
    ten_go_status_set(&status, ten_error_errno(&c_err),
                      ten_error_errmsg(&c_err));
  }

  ten_error_deinit(&c_err);

  return status;
}

ten_go_status_t ten_go_audio_frame_get_buf(uintptr_t bridge_addr,
                                           const void *buf_addr, int buf_size) {
  TEN_ASSERT(bridge_addr > 0 && buf_addr && buf_size > 0, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_go_msg_t *audio_frame_bridge = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(
      audio_frame_bridge && ten_go_msg_check_integrity(audio_frame_bridge),
      "Invalid argument.");

  ten_shared_ptr_t *c_audio_frame = ten_go_msg_c_msg(audio_frame_bridge);
  uint64_t size = ten_audio_frame_peek_data(c_audio_frame)->size;
  if (buf_size < size) {
    ten_go_status_set(&status, TEN_ERRNO_GENERIC, "buffer is not enough");
  } else {
    ten_buf_t *data = ten_audio_frame_peek_data(c_audio_frame);
    memcpy((void *)buf_addr, data->data, size);
  }

  return status;
}
