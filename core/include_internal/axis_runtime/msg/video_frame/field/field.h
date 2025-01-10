//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

typedef enum axis_VIDEO_FRAME_FIELD {
  axis_VIDEO_FRAME_FIELD_MSGHDR,

  axis_VIDEO_FRAME_FIELD_PIXEL_FMT,
  axis_VIDEO_FRAME_FIELD_TIMESTAMP,
  axis_VIDEO_FRAME_FIELD_WIDTH,
  axis_VIDEO_FRAME_FIELD_HEIGHT,
  axis_VIDEO_FRAME_FIELD_BUF,

  axis_VIDEO_FRAME_FIELD_LAST,
} axis_VIDEO_FRAME_FIELD;
