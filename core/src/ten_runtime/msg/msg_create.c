//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd_result/cmd.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "include_internal/ten_runtime/msg/msg_info.h"
#include "ten_runtime/msg/msg.h"

static ten_msg_t *ten_raw_msg_create_from_json(ten_json_t *json,
                                               ten_error_t *err) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Should not happen.");

  const char *type_str = NULL;
  const char *name_str = NULL;

  ten_json_t *ten_json = ten_json_object_peek(json, TEN_STR_UNDERLINE_TEN);
  if (ten_json) {
    type_str = ten_json_object_peek_string(ten_json, TEN_STR_TYPE);
    name_str = ten_json_object_peek_string(ten_json, TEN_STR_NAME);
  }

  ten_msg_t *msg = NULL;
  ten_raw_msg_create_from_json_func_t create_raw_from_json =
      ten_msg_info[ten_msg_type_from_type_and_name_string(type_str, name_str)]
          .create_from_json;
  if (create_raw_from_json) {
    msg = create_raw_from_json(json, err);
  }

  return msg;
}

ten_shared_ptr_t *ten_msg_create_from_json(ten_json_t *json, ten_error_t *err) {
  TEN_ASSERT(json, "Should not happen.");

  const char *type_str = NULL;
  const char *name_str = NULL;

  ten_json_t *ten_json = ten_json_object_peek(json, TEN_STR_UNDERLINE_TEN);
  if (ten_json) {
    type_str = ten_json_object_peek_string(ten_json, TEN_STR_TYPE);
    name_str = ten_json_object_peek_string(ten_json, TEN_STR_NAME);
  }

  ten_shared_ptr_t *msg = NULL;
  ten_raw_msg_create_from_json_func_t from_json =
      ten_msg_info[ten_msg_type_from_type_and_name_string(type_str, name_str)]
          .create_from_json;
  if (from_json) {
    ten_msg_t *raw_msg = from_json(json, err);
    if (raw_msg) {
      msg = ten_shared_ptr_create(raw_msg, ten_raw_msg_destroy);
    }
  }

  return msg;
}

static ten_msg_t *ten_raw_msg_create_from_json_string(const char *json_str,
                                                      ten_error_t *err) {
  ten_json_t *json = ten_json_from_string(json_str, err);
  if (json == NULL) {
    return NULL;
  }

  ten_msg_t *msg = ten_raw_msg_create_from_json(json, err);

  ten_json_destroy(json);

  return msg;
}

void ten_raw_msg_destroy(ten_msg_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  switch (self->type) {
    case TEN_MSG_TYPE_CMD:
    case TEN_MSG_TYPE_CMD_STOP_GRAPH:
    case TEN_MSG_TYPE_CMD_CLOSE_APP:
    case TEN_MSG_TYPE_CMD_TIMEOUT:
    case TEN_MSG_TYPE_CMD_TIMER:
    case TEN_MSG_TYPE_CMD_START_GRAPH:
      ten_raw_cmd_destroy((ten_cmd_t *)self);
      break;

    case TEN_MSG_TYPE_CMD_RESULT:
      ten_raw_cmd_result_destroy((ten_cmd_result_t *)self);
      break;

    case TEN_MSG_TYPE_DATA:
      ten_raw_data_destroy((ten_data_t *)self);
      break;

    case TEN_MSG_TYPE_VIDEO_FRAME:
      ten_raw_video_frame_destroy((ten_video_frame_t *)self);
      break;

    case TEN_MSG_TYPE_AUDIO_FRAME:
      ten_raw_audio_frame_destroy((ten_audio_frame_t *)self);
      break;

    default:
      TEN_ASSERT(0, "Should not happen.");
      break;
  }
}

ten_shared_ptr_t *ten_msg_create_from_json_string(const char *json_str,
                                                  ten_error_t *err) {
  ten_msg_t *msg = ten_raw_msg_create_from_json_string(json_str, err);
  return ten_shared_ptr_create(msg, ten_raw_msg_destroy);
}
