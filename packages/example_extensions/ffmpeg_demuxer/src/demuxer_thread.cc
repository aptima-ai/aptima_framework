//
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#include "demuxer_thread.h"

#include <atomic>
#include <cassert>
#include <cstdlib>
#include <memory>
#include <string>
#include <utility>

#include "demuxer.h"
#include "libavutil/rational.h"
#include "ten_runtime/binding/cpp/ten.h"
#include "ten_utils/lang/cpp/lib/value.h"
#include "ten_utils/lib/event.h"
#include "ten_utils/lib/thread.h"
#include "ten_utils/log/log.h"
#include "ten_utils/macro/check.h"

namespace ten {
namespace ffmpeg_extension {

demuxer_thread_t::demuxer_thread_t(ten::ten_env_proxy_t *ten_env_proxy,
                                   std::unique_ptr<ten::cmd_t> start_cmd,
                                   extension_t *extension,
                                   std::string &input_stream_loc)
    : ten_env_proxy_(ten_env_proxy),
      extension_(extension),
      stop_(false),
      demuxer_(nullptr),
      demuxer_thread(nullptr),
      demuxer_thread_is_started(ten_event_create(0, 0)),
      ready_for_demuxer(ten_event_create(0, 0)),
      input_stream_loc_(input_stream_loc),
      start_cmd_(std::move(start_cmd)) {
  TEN_ASSERT(extension, "Invalid argument.");
}

demuxer_thread_t::~demuxer_thread_t() {
  if (demuxer_ != nullptr) {
    delete demuxer_;
    demuxer_ = nullptr;
  }
  ten_event_destroy(demuxer_thread_is_started);
  ten_event_destroy(ready_for_demuxer);

  delete ten_env_proxy_;
}

void *demuxer_thread_main(void *self_) {
  TEN_LOGD("Demuxer thread is started.");

  auto *demuxer_thread = reinterpret_cast<demuxer_thread_t *>(self_);
  TEN_ASSERT(demuxer_thread, "Invalid argument.");

  // Notify that the demuxer thread is started successfully.
  ten_event_set(demuxer_thread->demuxer_thread_is_started);

  if (!demuxer_thread->create_demuxer()) {
    TEN_LOGW("Failed to create demuxer, stop the demuxer thread.");
    demuxer_thread->reply_to_start_cmd(false);
    return nullptr;
  }

  TEN_ASSERT(demuxer_thread->demuxer_, "Demuxer should have been created.");

  // Notify that the demuxer has been created.
  demuxer_thread->reply_to_start_cmd();

  // The demuxter thread will be blocked until receiving start signal.
  demuxer_thread->wait_for_demuxer();

  // Starts the demuxer loop.
  TEN_LOGD("Start the demuxer thread loop.");

  DECODE_STATUS status = DECODE_STATUS_SUCCESS;
  while (!demuxer_thread->is_stopped() && status == DECODE_STATUS_SUCCESS) {
    // Decode next input frame.
    status = demuxer_thread->demuxer_->decode_next_packet();

    switch (status) {
      case DECODE_STATUS_EOF:
        TEN_LOGD("Input stream is ended, stop the demuxer thread normally.");

        // Send EOF frame, so that the subsequent stages could know this fact.
        demuxer_thread->send_image_eof();
        demuxer_thread->send_audio_eof();
        break;

      case DECODE_STATUS_ERROR:
        TEN_LOGW("Something bad happened, stop the demuxer thread abruptly.");
        break;

      default:
        break;
    }
  }

  demuxer_thread->notify_completed(status == DECODE_STATUS_EOF);

  TEN_LOGD("Demuxer thread is stopped.");

  return nullptr;
}

void demuxer_thread_t::notify_completed(bool success) {
  ten_env_proxy_->notify([this, success](ten::ten_env_t &ten_env) {
    auto cmd = ten::cmd_t::create("complete");
    cmd->set_property("input_stream", input_stream_loc_);
    cmd->set_property("success", success);
    ten_env.send_cmd(std::move(cmd));
  });
}

void demuxer_thread_t::start() {
  demuxer_thread = ten_thread_create(nullptr, demuxer_thread_main, this);
  TEN_ASSERT(demuxer_thread, "Invalid argument.");
}

void demuxer_thread_t::start_demuxing() { ten_event_set(ready_for_demuxer); }

void demuxer_thread_t::wait_for_start() {
  ten_event_wait(demuxer_thread_is_started, -1);
}

void demuxer_thread_t::wait_for_demuxer() {
  ten_event_wait(ready_for_demuxer, -1);
}

void demuxer_thread_t::wait_for_stop() {
  int rc = ten_thread_join(demuxer_thread, -1);
  TEN_ASSERT(!rc, "Invalid argument.");

  TEN_LOGD("Demuxer thread has been reclaimed.");
}

bool demuxer_thread_t::create_demuxer() {
  demuxer_ = new demuxer_t(ten_env_proxy_, this);
  TEN_ASSERT(demuxer_, "Invalid argument.");

  return demuxer_->open_input_stream(input_stream_loc_);
}

// Called from the demuxer thread.
void demuxer_thread_t::send_image_eof() {
  auto frame = ten::video_frame_t::create("video_frame");
  frame->set_is_eof(true);

  auto frame_shared =
      std::make_shared<std::unique_ptr<ten::video_frame_t>>(std::move(frame));

  ten_env_proxy_->notify([frame_shared](ten::ten_env_t &ten_env) {
    ten_env.send_video_frame(std::move(*frame_shared));
  });
}

// Called from the demuxer thread.
void demuxer_thread_t::send_audio_eof() {
  auto frame = ten::audio_frame_t::create("audio_frame");
  frame->set_is_eof(true);

  auto frame_shared =
      std::make_shared<std::unique_ptr<ten::audio_frame_t>>(std::move(frame));

  ten_env_proxy_->notify([frame_shared](ten::ten_env_t &ten_env) {
    ten_env.send_audio_frame(std::move(*frame_shared));
  });
}

static inline double rational_to_double(AVRational &&r) {
  return r.num / static_cast<double>(r.den);
}

void demuxer_thread_t::reply_to_start_cmd(bool success) {
  if (!success || demuxer_ == nullptr) {
    ten_env_proxy_->notify([this](ten::ten_env_t &ten_env) {
      auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_ERROR);
      cmd_result->set_property("detail", "fail to prepare demuxer.");
      ten_env.return_result(std::move(cmd_result), std::move(start_cmd_));
    });
    return;
  }

  auto resp = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
  resp->set_property("detail", "The demuxer has been started.");

  // video settings
  resp->set_property("frame_rate_num", demuxer_->frame_rate().num);
  resp->set_property("frame_rate_den", demuxer_->frame_rate().den);
  resp->set_property("frame_rate_d",
                     rational_to_double(demuxer_->frame_rate()));

  resp->set_property("video_time_base_num", demuxer_->video_time_base().num);
  resp->set_property("video_time_base_den", demuxer_->video_time_base().den);
  resp->set_property("video_time_base_d",
                     rational_to_double(demuxer_->video_time_base()));

  resp->set_property("width", demuxer_->width());
  resp->set_property("height", demuxer_->height());
  resp->set_property("bit_rate", demuxer_->bit_rate());
  resp->set_property("num_of_frames", demuxer_->number_of_frames());

  // audio settings
  resp->set_property("audio_sample_rate", demuxer_->audio_sample_rate_);
  resp->set_property("audio_channel_layout", demuxer_->audio_channel_layout_);
  resp->set_property("audio_num_of_channels", demuxer_->audio_num_of_channels_);
  resp->set_property("audio_time_base_num", demuxer_->audio_time_base().num);
  resp->set_property("audio_time_base_den", demuxer_->audio_time_base().den);
  resp->set_property("audio_time_base_d",
                     rational_to_double(demuxer_->audio_time_base()));

  auto resp_shared =
      std::make_shared<std::unique_ptr<ten::cmd_result_t>>(std::move(resp));
  ten_env_proxy_->notify([resp_shared, this](ten::ten_env_t &ten_env) {
    ten_env.return_result(std::move(*resp_shared), std::move(start_cmd_));
  });
}

}  // namespace ffmpeg_extension
}  // namespace ten
