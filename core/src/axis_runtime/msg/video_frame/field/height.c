//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/msg/video_frame/field/height.h"

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/msg/video_frame/video_frame.h"
#include "axis_runtime/msg/msg.h"
#include "axis_runtime/msg/video_frame/video_frame.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"

void axis_video_frame_copy_height(axis_msg_t *self, axis_msg_t *src,
                                 axis_UNUSED axis_list_t *excluded_field_ids) {
  axis_ASSERT(self && src && axis_raw_msg_check_integrity(src) &&
                 axis_raw_msg_get_type(src) == axis_MSG_TYPE_VIDEO_FRAME,
             "Should not happen.");

  axis_raw_video_frame_set_height(
      (axis_video_frame_t *)self,
      axis_raw_video_frame_get_height((axis_video_frame_t *)src));
}

bool axis_video_frame_process_height(axis_msg_t *self,
                                    axis_raw_msg_process_one_field_func_t cb,
                                    void *user_data, axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");

  axis_msg_field_process_data_t height_field;
  axis_msg_field_process_data_init(&height_field, axis_STR_HEIGHT,
                                  &((axis_video_frame_t *)self)->height, false);

  bool rc = cb(self, &height_field, user_data, err);

  return rc;
}
