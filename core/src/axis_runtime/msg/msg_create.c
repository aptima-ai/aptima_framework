//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/msg/audio_frame/audio_frame.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/cmd.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd_result/cmd.h"
#include "include_internal/axis_runtime/msg/data/data.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/msg/video_frame/video_frame.h"
#include "axis_runtime/msg/msg.h"

void axis_raw_msg_destroy(axis_msg_t *self) {
  axis_ASSERT(self, "Should not happen.");

  switch (self->type) {
    case axis_MSG_TYPE_CMD:
    case axis_MSG_TYPE_CMD_STOP_GRAPH:
    case axis_MSG_TYPE_CMD_CLOSE_APP:
    case axis_MSG_TYPE_CMD_TIMEOUT:
    case axis_MSG_TYPE_CMD_TIMER:
    case axis_MSG_TYPE_CMD_START_GRAPH:
      axis_raw_cmd_destroy((axis_cmd_t *)self);
      break;

    case axis_MSG_TYPE_CMD_RESULT:
      axis_raw_cmd_result_destroy((axis_cmd_result_t *)self);
      break;

    case axis_MSG_TYPE_DATA:
      axis_raw_data_destroy((axis_data_t *)self);
      break;

    case axis_MSG_TYPE_VIDEO_FRAME:
      axis_raw_video_frame_destroy((axis_video_frame_t *)self);
      break;

    case axis_MSG_TYPE_AUDIO_FRAME:
      axis_raw_audio_frame_destroy((axis_audio_frame_t *)self);
      break;

    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }
}
