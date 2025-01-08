//
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#include "muxer_thread.h"

#include <list>
#include <utility>

#include "muxer.h"
#include "axis_runtime/binding/cpp/ten.h"
#include "axis_utils/lib/event.h"
#include "axis_utils/lib/mutex.h"
#include "axis_utils/lib/thread.h"
#include "axis_utils/lib/time.h"
#include "axis_utils/log/log.h"

namespace ten {
namespace ffmpeg_extension {

enum {
  // Audio continuity is very important, and the data is relatively small, so
  // try not to drop it, and to accumulate up to the following amount in the
  // queue.
  AUDIO_FRAME_FIFO_SIZE = 10000,

  VIDEO_FRAME_FIFO_SIZE = 10000,
};

muxer_thread_t::muxer_thread_t(ten::axis_env_proxy_t *axis_env_proxy,
                               demuxer_settings_t &settings,
                               std::string output_stream)
    : muxer_thread_(nullptr),
      muxer_thread_is_started_(axis_event_create(0, 0)),
      stop_(0),
      muxer_(nullptr),
      out_lock_(axis_mutex_create()),
      out_available_(axis_event_create(0, 1)),
      settings(settings),
      output_stream_(std::move(output_stream)),
      audio_eof_(false),
      video_eof_(false),
      axis_env_proxy_(axis_env_proxy) {}

muxer_thread_t::~muxer_thread_t() {
  axis_mutex_destroy(out_lock_);
  axis_event_destroy(out_available_);

  out_audios_.clear();
  out_images_.clear();

  // Clear muxer relevant resources.
  axis_event_destroy(muxer_thread_is_started_);

  delete muxer_;
  delete axis_env_proxy_;

  axis_LOGD("All the muxer resources have been cleaned.");
}

void muxer_thread_t::wait_for_the_first_av_frame_() {
  while ((axis_atomic_load(&stop_) == 0) && out_audios_.empty() &&
         out_images_.empty()) {
    int rc = axis_mutex_lock(out_lock_);
    axis_ASSERT(!rc, "Should not happen.");

    if (out_audios_.empty() && out_images_.empty()) {
      rc = axis_mutex_unlock(out_lock_);
      axis_ASSERT(!rc, "Should not happen.");

      axis_LOGD("No further frames need to be muxed, wait...");

      rc = axis_event_wait(out_available_, -1);
      axis_ASSERT(!rc, "Should not happen.");

      continue;
    }

    rc = axis_mutex_unlock(out_lock_);
    axis_ASSERT(!rc, "Should not happen.");
  }
}

void *muxer_thread_main_(void *self_) {
  axis_LOGD("Muxer thread is started.");

  auto *self = static_cast<muxer_thread_t *>(self_);
  axis_ASSERT(self, "Invalid argument.");

  if (axis_atomic_load(&self->stop_) != 0) {
    // Muxer thread has already been triggered to stop.
    return nullptr;
  }

  self->create_muxer_();

  axis_event_set(self->muxer_thread_is_started_);

  // Wait for the first frame to come before starting, to avoid meaningless CPU
  // resource wastage.
  self->wait_for_the_first_av_frame_();

  axis_LOGD("Starting to mux...");

  const int64_t start_time = axis_current_time();
  int64_t sleep_overhead = 0;
  bool status = true;

  std::list<std::unique_ptr<ten::audio_frame_t>> out_audios;
  std::list<std::unique_ptr<ten::video_frame_t>> out_images;

  while ((axis_atomic_load(&self->stop_) == 0 || !self->out_audios_.empty() ||
          !self->out_images_.empty()) &&
         status && !(self->audio_eof_ && self->video_eof_)) {
    out_audios.clear();
    out_images.clear();

    {
      // Get the received audio + video frames at once.

      int rc = axis_mutex_lock(self->out_lock_);
      axis_ASSERT(!rc, "Should not happen.");

      out_audios.swap(self->out_audios_);
      out_images.swap(self->out_images_);

      rc = axis_mutex_unlock(self->out_lock_);
      axis_ASSERT(!rc, "Should not happen.");
    }

    for (auto &out_audio : out_audios) {
      auto encode_status =
          self->muxer_->encode_audio_frame(std::move(out_audio));
      status = encode_status != ENCODE_STATUS_ERROR;
      axis_ASSERT(status, "Should not happen.");

      if (encode_status == ENCODE_STATUS_EOF) {
        self->audio_eof_ = true;
      }
    }

    for (auto &out_image : out_images) {
      auto encode_status =
          self->muxer_->encode_video_frame(std::move(out_image));
      status = encode_status != ENCODE_STATUS_ERROR;
      axis_ASSERT(status, "Should not happen.");

      if (encode_status == ENCODE_STATUS_EOF) {
        self->video_eof_ = true;
      }

      if (status) {
        int64_t now = axis_current_time();
        int64_t expected_time = start_time + self->muxer_->next_video_timing();
        int64_t sleep_time = expected_time - now - sleep_overhead;
        if (sleep_time > 0) {
          axis_sleep(sleep_time);

          // 'sleep' would cause thread switching, and thread switching would
          // have time overhead. Based on our frame rate, this time cannot be
          // ignored, so we need to consider it when we muxing next frame.
          sleep_overhead = axis_current_time() - now - sleep_time;
        } else {
          sleep_overhead = 0;
        }
      }
    }
  }

  out_audios.clear();
  out_images.clear();

  self->notify_completed(status);

  axis_LOGD("Muxer thread is stopped.");
  return nullptr;
}

void muxer_thread_t::start() {
  muxer_thread_ = axis_thread_create(nullptr, muxer_thread_main_, this);
  axis_ASSERT(muxer_thread_, "Invalid argument.");
}

void muxer_thread_t::wait_for_start() {
  axis_event_wait(muxer_thread_is_started_, -1);
}

void muxer_thread_t::stop() {
  axis_LOGD("Signal muxer thread to close.");

  axis_atomic_store(&stop_, 1);

  // Kick muxer thread in case it is waiting for the first av frame.
  axis_event_set(out_available_);
}

void muxer_thread_t::notify_completed(bool success) {
  auto cmd = ten::cmd_t::create("complete");
  cmd->set_property("input_stream", output_stream_);
  cmd->set_property("success", success);

  auto cmd_shared =
      std::make_shared<std::unique_ptr<ten::cmd_t>>(std::move(cmd));

  axis_env_proxy_->notify([cmd_shared](ten::axis_env_t &axis_env) {
    axis_env.send_cmd(std::move(*cmd_shared));
  });
}

void muxer_thread_t::wait_for_stop() {
  int rc = axis_thread_join(muxer_thread_, -1);
  axis_ASSERT(!rc, "Invalid argument.");

  axis_LOGD("Muxer thread has been reclaimed.");
}

void muxer_thread_t::create_muxer_() {
  muxer_ = new muxer_t();
  axis_ASSERT(muxer_, "Invalid argument.");

  muxer_->src_video_width_ = settings.src_video_width_;
  muxer_->src_video_height_ = settings.src_video_height_;
  muxer_->src_video_number_of_frames_ = settings.src_video_number_of_frames_;
  muxer_->src_video_bit_rate_ = settings.src_video_bit_rate_;
  muxer_->src_video_frame_rate_ = settings.src_video_frame_rate_;
  muxer_->src_video_time_base_ = settings.src_video_time_base_;

  muxer_->src_audio_sample_rate_ = settings.src_audio_sample_rate_;
  muxer_->src_audio_time_base_ = settings.src_audio_time_base_;
  muxer_->src_audio_channel_layout_ = settings.src_audio_channel_layout_;

  muxer_->open(output_stream_, false);
}

// Put received audio frame from TEN world to FFmpeg.
void muxer_thread_t::on_axis_audio_frame(
    std::unique_ptr<ten::audio_frame_t> frame) {
  int rc = axis_mutex_lock(out_lock_);
  axis_ASSERT(!rc, "Should not happen.");

  // Discard one old video frame if the queue is full.
  if (out_audios_.size() >= AUDIO_FRAME_FIFO_SIZE) {
    out_audios_.pop_front();
    axis_LOGD("out_audios buffer overflow. One oldest audio frame is dropped");
  }

  out_audios_.push_back(std::move(frame));

  rc = axis_mutex_unlock(out_lock_);
  axis_ASSERT(!rc, "Should not happen.");

  axis_event_set(out_available_);
}

// Put received video frame from TEN world to FFmpeg.
void muxer_thread_t::on_axis_video_frame(
    std::unique_ptr<ten::video_frame_t> frame) {
  int rc = axis_mutex_lock(out_lock_);
  axis_ASSERT(!rc, "Should not happen.");

  // Discard one old video frame if the queue is full.
  if (out_images_.size() >= VIDEO_FRAME_FIFO_SIZE) {
    out_images_.pop_front();
    axis_LOGD("out_images buffer overflow. One oldest video frame is dropped.");
  }

  out_images_.push_back(std::move(frame));

  rc = axis_mutex_unlock(out_lock_);
  axis_ASSERT(!rc, "Should not happen.");

  axis_event_set(out_available_);
}

}  // namespace ffmpeg_extension
}  // namespace ten
