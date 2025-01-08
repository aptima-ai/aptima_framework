//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "gtest/gtest.h"
#include "axis_runtime/msg/audio_frame/audio_frame.h"
#include "axis_runtime/msg/data/data.h"
#include "axis_runtime/msg/video_frame/video_frame.h"
#include "axis_utils/lib/smart_ptr.h"

TEST(TenMsgTest, create) {
  axis_shared_ptr_t *data = axis_data_create("test", nullptr);
  axis_shared_ptr_destroy(data);

  axis_shared_ptr_t *audio_frame = axis_audio_frame_create("test", nullptr);
  axis_shared_ptr_destroy(audio_frame);

  axis_shared_ptr_t *video_frame = axis_video_frame_create("test", nullptr);
  axis_shared_ptr_destroy(video_frame);
}
