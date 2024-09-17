//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/msg/audio_frame/field/data_fmt.h"

#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/msg/audio_frame/audio_frame.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "ten_utils/macro/check.h"
#include "ten_runtime/msg/audio_frame/audio_frame.h"
#include "ten_runtime/msg/msg.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/macro/mark.h"

const char *ten_audio_frame_data_fmt_to_string(
    const TEN_AUDIO_FRAME_DATA_FMT data_fmt) {
  switch (data_fmt) {
    case TEN_AUDIO_FRAME_DATA_FMT_INTERLEAVE:
      return "interleave";
    case TEN_AUDIO_FRAME_DATA_FMT_NON_INTERLEAVE:
      return "non_interleave";
    default:
      TEN_ASSERT(0, "Should not happen.");
      return NULL;
  }
}

TEN_AUDIO_FRAME_DATA_FMT ten_audio_frame_data_fmt_from_string(
    const char *data_fmt_str) {
  if (!strcmp(data_fmt_str, "interleave")) {
    return TEN_AUDIO_FRAME_DATA_FMT_INTERLEAVE;
  } else if (!strcmp(data_fmt_str, "non_interleave")) {
    return TEN_AUDIO_FRAME_DATA_FMT_NON_INTERLEAVE;
  } else {
    TEN_ASSERT(0, "Should not happen.");
    return TEN_AUDIO_FRAME_DATA_FMT_INVALID;
  }
}

bool ten_audio_frame_put_data_fmt_to_json(ten_msg_t *self, ten_json_t *json,
                                          ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self) &&
                 ten_raw_msg_get_type(self) == TEN_MSG_TYPE_AUDIO_FRAME,
             "Should not happen.");

  ten_json_t *ten_json =
      ten_json_object_peek_object_forcibly(json, TEN_STR_UNDERLINE_TEN);
  TEN_ASSERT(ten_json, "Should not happen.");

  ten_json_object_set_new(
      ten_json, TEN_STR_DATA_FMT,
      ten_json_create_integer(
          ten_raw_audio_frame_get_data_fmt((ten_audio_frame_t *)self)));

  return true;
}

bool ten_audio_frame_get_data_fmt_from_json(ten_msg_t *self, ten_json_t *json,
                                            TEN_UNUSED ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self) &&
                 ten_raw_msg_get_type(self) == TEN_MSG_TYPE_AUDIO_FRAME,
             "Should not happen.");
  TEN_ASSERT(json && ten_json_check_integrity(json), "Should not happen.");

  ten_json_t *ten_json =
      ten_json_object_peek_object(json, TEN_STR_UNDERLINE_TEN);
  if (!ten_json) {
    return true;
  }

  TEN_AUDIO_FRAME_DATA_FMT data_fmt =
      (TEN_AUDIO_FRAME_DATA_FMT)ten_json_object_get_integer(ten_json,
                                                            TEN_STR_DATA_FMT);
  ten_raw_audio_frame_set_data_fmt((ten_audio_frame_t *)self, data_fmt);

  return true;
}

void ten_audio_frame_copy_data_fmt(ten_msg_t *self, ten_msg_t *src,
                                   TEN_UNUSED ten_list_t *excluded_field_ids) {
  TEN_ASSERT(self && src && ten_raw_msg_check_integrity(src) &&
                 ten_raw_msg_get_type(src) == TEN_MSG_TYPE_AUDIO_FRAME,
             "Should not happen.");

  ten_raw_audio_frame_set_data_fmt(
      (ten_audio_frame_t *)self,
      ten_raw_audio_frame_get_data_fmt((ten_audio_frame_t *)src));
}
