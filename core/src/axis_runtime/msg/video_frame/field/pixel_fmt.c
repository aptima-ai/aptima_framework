//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/msg/video_frame/field/pixel_fmt.h"

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/msg/video_frame/video_frame.h"
#include "axis_runtime/msg/msg.h"
#include "axis_runtime/msg/video_frame/video_frame.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"

const char *axis_video_frame_pixel_fmt_to_string(const axis_PIXEL_FMT pixel_fmt) {
  switch (pixel_fmt) {
    case axis_PIXEL_FMT_RGB24:
      return "rgb24";
    case axis_PIXEL_FMT_RGBA:
      return "rgba";
    case axis_PIXEL_FMT_BGR24:
      return "bgr24";
    case axis_PIXEL_FMT_BGRA:
      return "bgra";
    case axis_PIXEL_FMT_I420:
      return "i420";
    case axis_PIXEL_FMT_I422:
      return "i422";
    case axis_PIXEL_FMT_NV21:
      return "nv21";
    case axis_PIXEL_FMT_NV12:
      return "nv12";
    default:
      axis_ASSERT(0, "Should not happen.");
      return NULL;
  }
}

axis_PIXEL_FMT axis_video_frame_pixel_fmt_from_string(const char *pixel_fmt_str) {
  if (!strcmp(pixel_fmt_str, "rgb24")) {
    return axis_PIXEL_FMT_RGB24;
  } else if (!strcmp(pixel_fmt_str, "rgba")) {
    return axis_PIXEL_FMT_RGBA;
  } else if (!strcmp(pixel_fmt_str, "bgr24")) {
    return axis_PIXEL_FMT_BGR24;
  } else if (!strcmp(pixel_fmt_str, "bgra")) {
    return axis_PIXEL_FMT_BGRA;
  } else if (!strcmp(pixel_fmt_str, "i420")) {
    return axis_PIXEL_FMT_I420;
  } else if (!strcmp(pixel_fmt_str, "i422")) {
    return axis_PIXEL_FMT_I422;
  } else if (!strcmp(pixel_fmt_str, "nv21")) {
    return axis_PIXEL_FMT_NV21;
  } else if (!strcmp(pixel_fmt_str, "nv12")) {
    return axis_PIXEL_FMT_NV12;
  } else {
    axis_ASSERT(0, "Should not happen.");
    return axis_PIXEL_FMT_INVALID;
  }
}

void axis_video_frame_copy_pixel_fmt(axis_msg_t *self, axis_msg_t *src,
                                    axis_UNUSED axis_list_t *excluded_field_ids) {
  axis_ASSERT(self && src && axis_raw_msg_check_integrity(src) &&
                 axis_raw_msg_get_type(src) == axis_MSG_TYPE_VIDEO_FRAME,
             "Should not happen.");

  axis_raw_video_frame_set_pixel_fmt(
      (axis_video_frame_t *)self,
      axis_raw_video_frame_get_pixel_fmt((axis_video_frame_t *)src));
}

bool axis_video_frame_process_pixel_fmt(axis_msg_t *self,
                                       axis_raw_msg_process_one_field_func_t cb,
                                       void *user_data, axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");

  axis_msg_field_process_data_t pixel_fmt_field;
  axis_msg_field_process_data_init(&pixel_fmt_field, axis_STR_PIXEL_FMT,
                                  &((axis_video_frame_t *)self)->pixel_fmt,
                                  false);

  bool rc = cb(self, &pixel_fmt_field, user_data, err);

  return rc;
}