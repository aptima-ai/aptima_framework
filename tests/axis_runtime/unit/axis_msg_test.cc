//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "gtest/gtest.h"
#include "aptima_runtime/msg/audio_frame/audio_frame.h"
#include "aptima_runtime/msg/data/data.h"
#include "aptima_runtime/msg/video_frame/video_frame.h"
#include "aptima_utils/lib/smart_ptr.h"

TEST(TenMsgTest, create) {
  aptima_shared_ptr_t *data = aptima_data_create("test", nullptr);
  aptima_shared_ptr_destroy(data);

  aptima_shared_ptr_t *audio_frame = aptima_audio_frame_create("test", nullptr);
  aptima_shared_ptr_destroy(audio_frame);

  aptima_shared_ptr_t *video_frame = aptima_video_frame_create("test", nullptr);
  aptima_shared_ptr_destroy(video_frame);
}
