//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/msg/video_frame/field/height.h"

#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "include_internal/ten_runtime/msg/video_frame/video_frame.h"
#include "ten_utils/macro/check.h"
#include "ten_runtime/msg/msg.h"
#include "ten_runtime/msg/video_frame/video_frame.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/macro/mark.h"

bool ten_video_frame_put_height_to_json(ten_msg_t *self, ten_json_t *json,
                                        ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self) &&
                 ten_raw_msg_get_type(self) == TEN_MSG_TYPE_VIDEO_FRAME,
             "Should not happen.");

  ten_json_t *ten_json =
      ten_json_object_peek_object_forcibly(json, TEN_STR_UNDERLINE_TEN);
  TEN_ASSERT(ten_json, "Should not happen.");

  ten_json_object_set_new(
      ten_json, TEN_STR_HEIGHT,
      ten_json_create_integer(
          ten_raw_video_frame_get_height((ten_video_frame_t *)self)));

  return true;
}

bool ten_video_frame_get_height_from_json(ten_msg_t *self, ten_json_t *json,
                                          TEN_UNUSED ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self) &&
                 ten_raw_msg_get_type(self) == TEN_MSG_TYPE_VIDEO_FRAME,
             "Should not happen.");
  TEN_ASSERT(json && ten_json_check_integrity(json), "Should not happen.");

  ten_json_t *ten_json =
      ten_json_object_peek_object(json, TEN_STR_UNDERLINE_TEN);
  if (!ten_json) {
    return true;
  }

  int64_t height = ten_json_object_get_integer(ten_json, TEN_STR_HEIGHT);
  ten_raw_video_frame_set_height((ten_video_frame_t *)self, (int32_t)height);

  return true;
}

void ten_video_frame_copy_height(ten_msg_t *self, ten_msg_t *src,
                                 TEN_UNUSED ten_list_t *excluded_field_ids) {
  TEN_ASSERT(self && src && ten_raw_msg_check_integrity(src) &&
                 ten_raw_msg_get_type(src) == TEN_MSG_TYPE_VIDEO_FRAME,
             "Should not happen.");

  ten_raw_video_frame_set_height(
      (ten_video_frame_t *)self,
      ten_raw_video_frame_get_height((ten_video_frame_t *)src));
}
