//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

typedef enum axis_AUDIO_FRAME_FIELD {
  axis_AUDIO_FRAME_FIELD_MSGHDR,

  axis_AUDIO_FRAME_FIELD_TIMESTAMP,
  axis_AUDIO_FRAME_FIELD_SAMPLE_RATE,
  axis_AUDIO_FRAME_FIELD_BYTES_PER_SAMPLE,
  axis_AUDIO_FRAME_FIELD_SAMPLES_PER_CHANNEL,
  axis_AUDIO_FRAME_FIELD_NUMBER_OF_CHANNEL,
  axis_AUDIO_FRAME_FIELD_DATA_FMT,
  axis_AUDIO_FRAME_FIELD_BUF,
  axis_AUDIO_FRAME_FIELD_LINE_SIZE,

  axis_AUDIO_FRAME_FIELD_LAST,
} axis_AUDIO_FRAME_FIELD;
