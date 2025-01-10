//
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#pragma once

#include <atomic>
#include <cassert>
#include <cstdint>
#include <list>
#include <string>

#include "muxer.h"
#include "ten_runtime/binding/cpp/aptima.h"
#include "ten_utils/lib/atomic.h"
#include "ten_utils/lib/mutex.h"
#include "ten_utils/lib/thread.h"

namespace aptima {
namespace ffmpeg_extension {

struct demuxer_settings_t {
  // @{
  // Source video settings.
  int src_video_width_;
  int src_video_height_;
  int64_t src_video_bit_rate_;
  int64_t src_video_number_of_frames_;
  AVRational src_video_frame_rate_;
  AVRational src_video_time_base_;
  // @}

  // @{
  // Source audio settings.
  int src_audio_sample_rate_;
  AVRational src_audio_time_base_;
  uint64_t src_audio_channel_layout_;
  // @}
};

class muxer_thread_t {
 public:
  explicit muxer_thread_t(aptima::ten_env_proxy_t *, demuxer_settings_t &,
                          std::string);
  ~muxer_thread_t();

  muxer_thread_t(const muxer_thread_t &other) = delete;
  muxer_thread_t(muxer_thread_t &&other) = delete;

  muxer_thread_t &operator=(const muxer_thread_t &other) = delete;
  muxer_thread_t &operator=(muxer_thread_t &&other) = delete;

  void start();
  void wait_for_start();

  void stop();
  void wait_for_stop();

  void on_ten_audio_frame(std::unique_ptr<aptima::audio_frame_t> frame);
  void on_ten_video_frame(std::unique_ptr<aptima::video_frame_t> frame);

 private:
  friend void *muxer_thread_main_(void *self_);

  void create_muxer_();
  void wait_for_the_first_av_frame_();
  void notify_completed(bool success = true);

  ten_thread_t *muxer_thread_;
  ten_event_t *muxer_thread_is_started_;
  ten_atomic_t stop_;
  muxer_t *muxer_;

  ten_mutex_t *out_lock_;
  ten_event_t *out_available_;
  std::list<std::unique_ptr<aptima::audio_frame_t>> out_audios_;
  std::list<std::unique_ptr<aptima::video_frame_t>> out_images_;

  demuxer_settings_t settings;
  std::string output_stream_;

  bool audio_eof_;
  bool video_eof_;

  aptima::ten_env_proxy_t *ten_env_proxy_;
};

}  // namespace ffmpeg_extension
}  // namespace aptima
