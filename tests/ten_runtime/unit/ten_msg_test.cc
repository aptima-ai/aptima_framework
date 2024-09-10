//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "gtest/gtest.h"
#include "ten_runtime/msg/audio_frame/audio_frame.h"
#include "ten_runtime/msg/data/data.h"
#include "ten_runtime/msg/video_frame/video_frame.h"
#include "ten_utils/lib/smart_ptr.h"

TEST(TenMsgTest, create) {
  ten_shared_ptr_t *data = ten_data_create();
  ten_shared_ptr_destroy(data);

  ten_shared_ptr_t *audio_frame = ten_audio_frame_create();
  ten_shared_ptr_destroy(audio_frame);

  ten_shared_ptr_t *video_frame = ten_video_frame_create();
  ten_shared_ptr_destroy(video_frame);
}
