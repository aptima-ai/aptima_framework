//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_runtime/binding/go/interface/ten/video_frame.h"

#include <stdint.h>

#include "include_internal/axis_runtime/binding/go/internal/common.h"
#include "include_internal/axis_runtime/binding/go/msg/msg.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/msg/video_frame/video_frame.h"
#include "axis_runtime/binding/go/interface/ten/msg.h"
#include "axis_runtime/common/errno.h"
#include "axis_runtime/msg/msg.h"
#include "axis_runtime/msg/video_frame/video_frame.h"
#include "axis_utils/macro/check.h"

axis_go_error_t axis_go_video_frame_create(const void *name, int name_len,
                                         uintptr_t *bridge_addr) {
  axis_ASSERT(bridge_addr, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_shared_ptr_t *c_video_frame =
      axis_video_frame_create_with_name_len(name, name_len, NULL);
  axis_ASSERT(c_video_frame, "Should not happen.");

  axis_go_msg_t *bridge = axis_go_msg_create(c_video_frame);
  *bridge_addr = (uintptr_t)bridge;

  // The ownership of the C message instance is transferred into the GO message
  // instance.
  axis_shared_ptr_destroy(c_video_frame);

  return cgo_error;
}

axis_go_error_t axis_go_video_frame_alloc_buf(uintptr_t bridge_addr, int size) {
  axis_ASSERT(bridge_addr && size, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_msg_t *video_frame_bridge = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(
      video_frame_bridge && axis_go_msg_check_integrity(video_frame_bridge),
      "Invalid argument.");

  axis_shared_ptr_t *c_video_frame = axis_go_msg_c_msg(video_frame_bridge);
  uint8_t *data = axis_video_frame_alloc_data(c_video_frame, size);
  if (!data) {
    axis_go_error_set(&cgo_error, axis_ERRNO_GENERIC,
                     "failed to allocate memory");
  }

  return cgo_error;
}

axis_go_error_t axis_go_video_frame_lock_buf(uintptr_t bridge_addr,
                                           uint8_t **buf_addr,
                                           uint64_t *buf_size) {
  axis_ASSERT(bridge_addr && buf_size, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_msg_t *video_frame_bridge = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(
      video_frame_bridge && axis_go_msg_check_integrity(video_frame_bridge),
      "Invalid argument.");

  axis_shared_ptr_t *c_video_frame = axis_go_msg_c_msg(video_frame_bridge);
  axis_buf_t *c_video_frame_data = axis_video_frame_peek_buf(c_video_frame);

  axis_error_t c_err;
  axis_error_init(&c_err);

  if (!axis_msg_add_locked_res_buf(c_video_frame, c_video_frame_data->data,
                                  &c_err)) {
    axis_go_error_set(&cgo_error, axis_error_errno(&c_err),
                     axis_error_errmsg(&c_err));
  } else {
    *buf_addr = c_video_frame_data->data;
    *buf_size = c_video_frame_data->size;
  }

  axis_error_deinit(&c_err);

  return cgo_error;
}

axis_go_error_t axis_go_video_frame_unlock_buf(uintptr_t bridge_addr,
                                             const void *buf_addr) {
  axis_ASSERT(bridge_addr && buf_addr, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_msg_t *video_frame_bridge = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(
      video_frame_bridge && axis_go_msg_check_integrity(video_frame_bridge),
      "Invalid argument.");

  axis_shared_ptr_t *c_video_frame = axis_go_msg_c_msg(video_frame_bridge);

  axis_error_t c_err;
  axis_error_init(&c_err);

  bool result = axis_msg_remove_locked_res_buf(c_video_frame, buf_addr, &c_err);
  if (!result) {
    axis_go_error_set(&cgo_error, axis_error_errno(&c_err),
                     axis_error_errmsg(&c_err));
  }

  axis_error_deinit(&c_err);

  return cgo_error;
}

axis_go_error_t axis_go_video_frame_get_buf(uintptr_t bridge_addr,
                                          const void *buf_addr,
                                          uint64_t buf_size) {
  axis_ASSERT(bridge_addr && buf_addr && buf_size > 0, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_msg_t *video_frame_bridge = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(
      video_frame_bridge && axis_go_msg_check_integrity(video_frame_bridge),
      "Invalid argument.");

  axis_shared_ptr_t *c_video_frame = axis_go_msg_c_msg(video_frame_bridge);
  uint64_t size = axis_video_frame_peek_buf(c_video_frame)->size;
  if (buf_size < size) {
    axis_go_error_set(&cgo_error, axis_ERRNO_GENERIC, "buffer is not enough");
  } else {
    axis_buf_t *data = axis_video_frame_peek_buf(c_video_frame);
    memcpy((void *)buf_addr, data->data, size);
  }

  return cgo_error;
}

axis_go_error_t axis_go_video_frame_get_buf_size(uintptr_t bridge_addr,
                                               uint64_t *buf_size) {
  axis_ASSERT(bridge_addr && buf_size, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_msg_t *video_frame_bridge = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(
      video_frame_bridge && axis_go_msg_check_integrity(video_frame_bridge),
      "Invalid argument.");

  axis_shared_ptr_t *c_video_frame = axis_go_msg_c_msg(video_frame_bridge);
  *buf_size = axis_video_frame_peek_buf(c_video_frame)->size;

  return cgo_error;
}

axis_go_error_t axis_go_video_frame_set_width(uintptr_t bridge_addr,
                                            int32_t width) {
  axis_ASSERT(bridge_addr && width > 0, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_msg_t *video_frame_bridge = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(
      video_frame_bridge && axis_go_msg_check_integrity(video_frame_bridge),
      "Invalid argument.");

  axis_shared_ptr_t *c_video_frame = axis_go_msg_c_msg(video_frame_bridge);
  axis_video_frame_set_width(c_video_frame, width);

  return cgo_error;
}

axis_go_error_t axis_go_video_frame_get_width(uintptr_t bridge_addr,
                                            int32_t *width) {
  axis_ASSERT(bridge_addr && width, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_msg_t *video_frame_bridge = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(
      video_frame_bridge && axis_go_msg_check_integrity(video_frame_bridge),
      "Invalid argument.");

  axis_shared_ptr_t *c_video_frame = axis_go_msg_c_msg(video_frame_bridge);
  *width = axis_video_frame_get_width(c_video_frame);

  return cgo_error;
}

axis_go_error_t axis_go_video_frame_set_height(uintptr_t bridge_addr,
                                             int32_t height) {
  axis_ASSERT(bridge_addr && height > 0, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_msg_t *video_frame_bridge = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(
      video_frame_bridge && axis_go_msg_check_integrity(video_frame_bridge),
      "Invalid argument.");

  axis_shared_ptr_t *c_video_frame = axis_go_msg_c_msg(video_frame_bridge);
  axis_video_frame_set_height(c_video_frame, height);

  return cgo_error;
}

axis_go_error_t axis_go_video_frame_get_height(uintptr_t bridge_addr,
                                             int32_t *height) {
  axis_ASSERT(bridge_addr && height, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_msg_t *video_frame_bridge = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(
      video_frame_bridge && axis_go_msg_check_integrity(video_frame_bridge),
      "Invalid argument.");

  axis_shared_ptr_t *c_video_frame = axis_go_msg_c_msg(video_frame_bridge);
  *height = axis_video_frame_get_height(c_video_frame);

  return cgo_error;
}

axis_go_error_t axis_go_video_frame_set_timestamp(uintptr_t bridge_addr,
                                                int64_t timestamp) {
  axis_ASSERT(bridge_addr && timestamp > 0, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_msg_t *video_frame_bridge = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(
      video_frame_bridge && axis_go_msg_check_integrity(video_frame_bridge),
      "Invalid argument.");

  axis_shared_ptr_t *c_video_frame = axis_go_msg_c_msg(video_frame_bridge);
  axis_video_frame_set_timestamp(c_video_frame, timestamp);

  return cgo_error;
}

axis_go_error_t axis_go_video_frame_get_timestamp(uintptr_t bridge_addr,
                                                int64_t *timestamp) {
  axis_ASSERT(bridge_addr && timestamp, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_msg_t *video_frame_bridge = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(
      video_frame_bridge && axis_go_msg_check_integrity(video_frame_bridge),
      "Invalid argument.");

  axis_shared_ptr_t *c_video_frame = axis_go_msg_c_msg(video_frame_bridge);
  *timestamp = axis_video_frame_get_timestamp(c_video_frame);

  return cgo_error;
}

axis_go_error_t axis_go_video_frame_set_eof(uintptr_t bridge_addr, bool is_eof) {
  axis_ASSERT(bridge_addr, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_msg_t *video_frame_bridge = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(
      video_frame_bridge && axis_go_msg_check_integrity(video_frame_bridge),
      "Invalid argument.");

  axis_shared_ptr_t *c_video_frame = axis_go_msg_c_msg(video_frame_bridge);
  axis_video_frame_set_eof(c_video_frame, is_eof);

  return cgo_error;
}

axis_go_error_t axis_go_video_frame_is_eof(uintptr_t bridge_addr, bool *is_eof) {
  axis_ASSERT(bridge_addr && is_eof, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_msg_t *video_frame_bridge = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(
      video_frame_bridge && axis_go_msg_check_integrity(video_frame_bridge),
      "Invalid argument.");

  axis_shared_ptr_t *c_video_frame = axis_go_msg_c_msg(video_frame_bridge);
  *is_eof = axis_video_frame_is_eof(c_video_frame);

  return cgo_error;
}

axis_go_error_t axis_go_video_frame_get_pixel_fmt(uintptr_t bridge_addr,
                                                uint32_t *fmt) {
  axis_ASSERT(bridge_addr && fmt, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_msg_t *video_frame_bridge = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(
      video_frame_bridge && axis_go_msg_check_integrity(video_frame_bridge),
      "Invalid argument.");

  axis_shared_ptr_t *c_video_frame = axis_go_msg_c_msg(video_frame_bridge);
  *fmt = axis_video_frame_get_pixel_fmt(c_video_frame);

  return cgo_error;
}

axis_go_error_t axis_go_video_frame_set_pixel_fmt(uintptr_t bridge_addr,
                                                uint32_t fmt) {
  axis_ASSERT(bridge_addr, "Invalid argument.");
  axis_ASSERT(fmt >= axis_PIXEL_FMT_RGB24 && fmt <= axis_PIXEL_FMT_I422,
             "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_msg_t *video_frame_bridge = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(
      video_frame_bridge && axis_go_msg_check_integrity(video_frame_bridge),
      "Invalid argument.");

  axis_shared_ptr_t *c_video_frame = axis_go_msg_c_msg(video_frame_bridge);
  axis_video_frame_set_pixel_fmt(c_video_frame, fmt);

  return cgo_error;
}
