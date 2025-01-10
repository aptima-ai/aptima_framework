//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/msg/video_frame/field/buf.h"

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/msg/loop_fields.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/msg/video_frame/video_frame.h"
#include "axis_runtime/msg/msg.h"
#include "axis_runtime/msg/video_frame/video_frame.h"
#include "axis_utils/macro/check.h"

bool axis_video_frame_process_buf(axis_msg_t *self,
                                 axis_raw_msg_process_one_field_func_t cb,
                                 void *user_data, axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");

  axis_msg_field_process_data_t data_field;
  axis_msg_field_process_data_init(&data_field, axis_STR_BUF,
                                  &((axis_video_frame_t *)self)->data, false);

  bool rc = cb(self, &data_field, user_data, err);

  return rc;
}