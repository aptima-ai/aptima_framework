//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/msg/audio_frame/field/data_fmt.h"

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/msg/audio_frame/audio_frame.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "axis_runtime/msg/audio_frame/audio_frame.h"
#include "axis_runtime/msg/msg.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"

const char *axis_audio_frame_data_fmt_to_string(
    const axis_AUDIO_FRAME_DATA_FMT data_fmt) {
  switch (data_fmt) {
    case axis_AUDIO_FRAME_DATA_FMT_INTERLEAVE:
      return "interleave";
    case axis_AUDIO_FRAME_DATA_FMT_NON_INTERLEAVE:
      return "non_interleave";
    default:
      axis_ASSERT(0, "Should not happen.");
      return NULL;
  }
}

axis_AUDIO_FRAME_DATA_FMT axis_audio_frame_data_fmt_from_string(
    const char *data_fmt_str) {
  if (!strcmp(data_fmt_str, "interleave")) {
    return axis_AUDIO_FRAME_DATA_FMT_INTERLEAVE;
  } else if (!strcmp(data_fmt_str, "non_interleave")) {
    return axis_AUDIO_FRAME_DATA_FMT_NON_INTERLEAVE;
  } else {
    axis_ASSERT(0, "Should not happen.");
    return axis_AUDIO_FRAME_DATA_FMT_INVALID;
  }
}

void axis_audio_frame_copy_data_fmt(axis_msg_t *self, axis_msg_t *src,
                                   axis_UNUSED axis_list_t *excluded_field_ids) {
  axis_ASSERT(self && src && axis_raw_msg_check_integrity(src) &&
                 axis_raw_msg_get_type(src) == axis_MSG_TYPE_AUDIO_FRAME,
             "Should not happen.");

  axis_raw_audio_frame_set_data_fmt(
      (axis_audio_frame_t *)self,
      axis_raw_audio_frame_get_data_fmt((axis_audio_frame_t *)src));
}

bool axis_audio_frame_process_data_fmt(axis_msg_t *self,
                                      axis_raw_msg_process_one_field_func_t cb,
                                      void *user_data, axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");

  axis_msg_field_process_data_t data_fmt_field;
  axis_msg_field_process_data_init(&data_fmt_field, axis_STR_DATA_FMT,
                                  &((axis_audio_frame_t *)self)->data_fmt,
                                  false);

  bool rc = cb(self, &data_fmt_field, user_data, err);

  return rc;
}
