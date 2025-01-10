//
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#include "muxer_thread.h"
#include "ten_runtime/binding/cpp/aptima.h"

namespace aptima {
namespace ffmpeg_extension {

static demuxer_settings_t read_settings(cmd_t &cmd) {
  demuxer_settings_t settings{};

  // Note that the `cmd` is created from json, so the type of integer property
  // is always int64_t.

  auto frame_rate_num =
      static_cast<int32_t>(cmd.get_property_int64("frame_rate_num"));
  auto frame_rate_den =
      static_cast<int32_t>(cmd.get_property_int64("frame_rate_den"));
  auto frame_rate = av_make_q(frame_rate_num, frame_rate_den);

  auto video_time_base_num =
      static_cast<int32_t>(cmd.get_property_int64("video_time_base_num"));
  auto video_time_base_den =
      static_cast<int32_t>(cmd.get_property_int64("video_time_base_den"));
  auto video_time_base = av_make_q(video_time_base_num, video_time_base_den);

  auto width = static_cast<int32_t>(cmd.get_property_int64("width"));
  auto height = static_cast<int32_t>(cmd.get_property_int64("height"));
  auto bit_rate = cmd.get_property_int64("bit_rate");
  auto num_of_frames = cmd.get_property_int64("num_of_frames");

  auto audio_sample_rate =
      static_cast<int32_t>(cmd.get_property_int64("audio_sample_rate"));
  auto audio_channel_layout = cmd.get_property_int64("audio_channel_layout");

  auto audio_time_base_num =
      static_cast<int32_t>(cmd.get_property_int64("audio_time_base_num"));
  auto audio_time_base_den =
      static_cast<int32_t>(cmd.get_property_int64("audio_time_base_den"));
  auto audio_time_base = av_make_q(audio_time_base_num, audio_time_base_den);

  settings.src_audio_sample_rate_ = audio_sample_rate;
  settings.src_video_bit_rate_ = bit_rate;
  settings.src_video_height_ = height;
  settings.src_video_width_ = width;
  settings.src_video_number_of_frames_ = num_of_frames;
  settings.src_video_frame_rate_ = frame_rate;
  settings.src_video_time_base_ = video_time_base;
  settings.src_audio_time_base_ = audio_time_base;
  settings.src_audio_channel_layout_ = audio_channel_layout;

  return settings;
}

class muxer_extension_t : public extension_t {
 public:
  explicit muxer_extension_t(const char *name) : extension_t(name) {}

  void on_start(TEN_UNUSED ten_env_t &ten_env) override {
    ten_env.on_start_done();
  }

  void on_cmd(ten_env_t &ten_env, std::unique_ptr<aptima::cmd_t> cmd) override {
    const auto cmd_name = cmd->get_name();

    if (std::string(cmd_name) == "start_muxer") {
      TEN_LOGE("muxer_extension_t::on_cmd, %s",
               cmd->get_property_to_json().c_str());

      auto settings = read_settings(*cmd);
      auto output = cmd->get_property_string("output_stream");
      if (output.empty()) {
        output = "ten_packages/extension/ffmpeg_muxer/test.mp4";
      }

      auto *ten_env_proxy = aptima::ten_env_proxy_t::create(ten_env);

      // Start the muxer thread. ffmpeg is living in its own thread.
      muxer_thread_ = new muxer_thread_t(ten_env_proxy, settings, output);
      muxer_thread_->start();
      muxer_thread_->wait_for_start();

      auto cmd_result = aptima::cmd_result_t::create(TEN_STATUS_CODE_OK);
      cmd_result->set_property("detail", "I am ready");
      ten_env.return_result(std::move(cmd_result), std::move(cmd));
    }
  }

  void on_stop(TEN_UNUSED ten_env_t &ten_env) override {
    // Stop the muxer thread. ffmpeg is living in its own thread.
    muxer_thread_->stop();
    muxer_thread_->wait_for_stop();
    delete muxer_thread_;

    ten_env.on_stop_done();
  }

  void on_audio_frame(TEN_UNUSED ten_env_t &ten_env,
                      std::unique_ptr<audio_frame_t> frame) override {
    muxer_thread_->on_ten_audio_frame(std::move(frame));
  }

  void on_video_frame(TEN_UNUSED ten_env_t &ten_env,
                      std::unique_ptr<video_frame_t> frame) override {
    muxer_thread_->on_ten_video_frame(std::move(frame));
  }

 private:
  muxer_thread_t *muxer_thread_{};
};

}  // namespace ffmpeg_extension
}  // namespace aptima

TEN_CPP_REGISTER_ADDON_AS_EXTENSION(ffmpeg_muxer,
                                    aptima::ffmpeg_extension::muxer_extension_t);
