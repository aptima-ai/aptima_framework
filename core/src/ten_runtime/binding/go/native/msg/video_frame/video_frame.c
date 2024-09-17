//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "ten_runtime/binding/go/interface/ten/video_frame.h"

#include <stdint.h>

#include "include_internal/ten_runtime/binding/go/internal/common.h"
#include "include_internal/ten_runtime/binding/go/msg/msg.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "ten_utils/macro/check.h"
#include "ten_runtime/binding/go/interface/ten/msg.h"
#include "ten_runtime/common/errno.h"
#include "ten_runtime/msg/msg.h"
#include "ten_runtime/msg/video_frame/video_frame.h"

ten_go_status_t ten_go_video_frame_create(const void *msg_name,
                                          int msg_name_len,
                                          uintptr_t *bridge_addr) {
  TEN_ASSERT(bridge_addr, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_shared_ptr_t *c_video_frame = ten_video_frame_create();
  TEN_ASSERT(c_video_frame, "Should not happen.");

  ten_msg_set_name_with_size(c_video_frame, msg_name, msg_name_len, NULL);

  ten_go_msg_t *bridge = ten_go_msg_create(c_video_frame);
  *bridge_addr = (uintptr_t)bridge;

  return status;
}

ten_go_status_t ten_go_video_frame_alloc_buf(uintptr_t bridge_addr, int size) {
  TEN_ASSERT(bridge_addr > 0 && size > 0, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_go_msg_t *video_frame_bridge = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(
      video_frame_bridge && ten_go_msg_check_integrity(video_frame_bridge),
      "Invalid argument.");

  ten_shared_ptr_t *c_video_frame = ten_go_msg_c_msg(video_frame_bridge);
  uint8_t *data = ten_video_frame_alloc_data(c_video_frame, size);
  if (!data) {
    ten_go_status_set(&status, TEN_ERRNO_GENERIC, "failed to allocate memory");
  }

  return status;
}

ten_go_status_t ten_go_video_frame_lock_buf(uintptr_t bridge_addr,
                                            uint8_t **buf_addr,
                                            uint64_t *buf_size) {
  TEN_ASSERT(bridge_addr > 0 && buf_size, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_go_msg_t *video_frame_bridge = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(
      video_frame_bridge && ten_go_msg_check_integrity(video_frame_bridge),
      "Invalid argument.");

  ten_shared_ptr_t *c_video_frame = ten_go_msg_c_msg(video_frame_bridge);
  ten_buf_t *c_video_frame_data = ten_video_frame_peek_data(c_video_frame);

  ten_error_t c_err;
  ten_error_init(&c_err);

  if (!ten_msg_add_locked_res_buf(c_video_frame, c_video_frame_data->data,
                                  &c_err)) {
    ten_go_status_set(&status, ten_error_errno(&c_err),
                      ten_error_errmsg(&c_err));
  } else {
    *buf_addr = c_video_frame_data->data;
    *buf_size = c_video_frame_data->size;
  }

  ten_error_deinit(&c_err);

  return status;
}

ten_go_status_t ten_go_video_frame_unlock_buf(uintptr_t bridge_addr,
                                              const void *buf_addr) {
  TEN_ASSERT(bridge_addr > 0 && buf_addr, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_go_msg_t *video_frame_bridge = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(
      video_frame_bridge && ten_go_msg_check_integrity(video_frame_bridge),
      "Invalid argument.");

  ten_shared_ptr_t *c_video_frame = ten_go_msg_c_msg(video_frame_bridge);

  ten_error_t c_err;
  ten_error_init(&c_err);

  bool result = ten_msg_remove_locked_res_buf(c_video_frame, buf_addr, &c_err);
  if (!result) {
    ten_go_status_set(&status, ten_error_errno(&c_err),
                      ten_error_errmsg(&c_err));
  }

  ten_error_deinit(&c_err);

  return status;
}

ten_go_status_t ten_go_video_frame_get_buf(uintptr_t bridge_addr,
                                           const void *buf_addr, int buf_size) {
  TEN_ASSERT(bridge_addr > 0 && buf_addr && buf_size > 0, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_go_msg_t *video_frame_bridge = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(
      video_frame_bridge && ten_go_msg_check_integrity(video_frame_bridge),
      "Invalid argument.");

  ten_shared_ptr_t *c_video_frame = ten_go_msg_c_msg(video_frame_bridge);
  uint64_t size = ten_video_frame_peek_data(c_video_frame)->size;
  if (buf_size < size) {
    ten_go_status_set(&status, TEN_ERRNO_GENERIC, "buffer is not enough");
  } else {
    ten_buf_t *data = ten_video_frame_peek_data(c_video_frame);
    memcpy((void *)buf_addr, data->data, size);
  }

  return status;
}

ten_go_status_t ten_go_video_frame_set_width(uintptr_t bridge_addr,
                                             int32_t width) {
  TEN_ASSERT(bridge_addr > 0 && width > 0, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_go_msg_t *video_frame_bridge = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(
      video_frame_bridge && ten_go_msg_check_integrity(video_frame_bridge),
      "Invalid argument.");

  ten_shared_ptr_t *c_video_frame = ten_go_msg_c_msg(video_frame_bridge);
  ten_video_frame_set_width(c_video_frame, width);

  return status;
}

ten_go_status_t ten_go_video_frame_get_width(uintptr_t bridge_addr,
                                             int32_t *width) {
  TEN_ASSERT(bridge_addr > 0 && width, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_go_msg_t *video_frame_bridge = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(
      video_frame_bridge && ten_go_msg_check_integrity(video_frame_bridge),
      "Invalid argument.");

  ten_shared_ptr_t *c_video_frame = ten_go_msg_c_msg(video_frame_bridge);
  *width = ten_video_frame_get_width(c_video_frame);

  return status;
}

ten_go_status_t ten_go_video_frame_set_height(uintptr_t bridge_addr,
                                              int32_t height) {
  TEN_ASSERT(bridge_addr > 0 && height > 0, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_go_msg_t *video_frame_bridge = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(
      video_frame_bridge && ten_go_msg_check_integrity(video_frame_bridge),
      "Invalid argument.");

  ten_shared_ptr_t *c_video_frame = ten_go_msg_c_msg(video_frame_bridge);
  ten_video_frame_set_height(c_video_frame, height);

  return status;
}

ten_go_status_t ten_go_video_frame_get_height(uintptr_t bridge_addr,
                                              int32_t *height) {
  TEN_ASSERT(bridge_addr > 0 && height, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_go_msg_t *video_frame_bridge = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(
      video_frame_bridge && ten_go_msg_check_integrity(video_frame_bridge),
      "Invalid argument.");

  ten_shared_ptr_t *c_video_frame = ten_go_msg_c_msg(video_frame_bridge);
  *height = ten_video_frame_get_height(c_video_frame);

  return status;
}

ten_go_status_t ten_go_video_frame_set_timestamp(uintptr_t bridge_addr,
                                                 int64_t timestamp) {
  TEN_ASSERT(bridge_addr > 0 && timestamp > 0, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_go_msg_t *video_frame_bridge = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(
      video_frame_bridge && ten_go_msg_check_integrity(video_frame_bridge),
      "Invalid argument.");

  ten_shared_ptr_t *c_video_frame = ten_go_msg_c_msg(video_frame_bridge);
  ten_video_frame_set_timestamp(c_video_frame, timestamp);

  return status;
}

ten_go_status_t ten_go_video_frame_get_timestamp(uintptr_t bridge_addr,
                                                 int64_t *timestamp) {
  TEN_ASSERT(bridge_addr > 0 && timestamp, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_go_msg_t *video_frame_bridge = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(
      video_frame_bridge && ten_go_msg_check_integrity(video_frame_bridge),
      "Invalid argument.");

  ten_shared_ptr_t *c_video_frame = ten_go_msg_c_msg(video_frame_bridge);
  *timestamp = ten_video_frame_get_timestamp(c_video_frame);

  return status;
}

ten_go_status_t ten_go_video_frame_set_is_eof(uintptr_t bridge_addr,
                                              bool is_eof) {
  TEN_ASSERT(bridge_addr > 0, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_go_msg_t *video_frame_bridge = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(
      video_frame_bridge && ten_go_msg_check_integrity(video_frame_bridge),
      "Invalid argument.");

  ten_shared_ptr_t *c_video_frame = ten_go_msg_c_msg(video_frame_bridge);
  ten_video_frame_set_is_eof(c_video_frame, is_eof);

  return status;
}

ten_go_status_t ten_go_video_frame_is_eof(uintptr_t bridge_addr, bool *is_eof) {
  TEN_ASSERT(bridge_addr > 0 && is_eof, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_go_msg_t *video_frame_bridge = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(
      video_frame_bridge && ten_go_msg_check_integrity(video_frame_bridge),
      "Invalid argument.");

  ten_shared_ptr_t *c_video_frame = ten_go_msg_c_msg(video_frame_bridge);
  *is_eof = ten_video_frame_is_eof(c_video_frame);

  return status;
}

ten_go_status_t ten_go_video_frame_get_pixel_fmt(uintptr_t bridge_addr,
                                                 uint32_t *fmt) {
  TEN_ASSERT(bridge_addr > 0 && fmt, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_go_msg_t *video_frame_bridge = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(
      video_frame_bridge && ten_go_msg_check_integrity(video_frame_bridge),
      "Invalid argument.");

  ten_shared_ptr_t *c_video_frame = ten_go_msg_c_msg(video_frame_bridge);
  *fmt = ten_video_frame_get_pixel_fmt(c_video_frame);

  return status;
}

ten_go_status_t ten_go_video_frame_set_pixel_fmt(uintptr_t bridge_addr,
                                                 uint32_t fmt) {
  TEN_ASSERT(bridge_addr > 0, "Invalid argument.");
  TEN_ASSERT(fmt >= TEN_PIXEL_FMT_RGB24 && fmt <= TEN_PIXEL_FMT_I422,
             "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_go_msg_t *video_frame_bridge = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(
      video_frame_bridge && ten_go_msg_check_integrity(video_frame_bridge),
      "Invalid argument.");

  ten_shared_ptr_t *c_video_frame = ten_go_msg_c_msg(video_frame_bridge);
  ten_video_frame_set_pixel_fmt(c_video_frame, fmt);

  return status;
}
