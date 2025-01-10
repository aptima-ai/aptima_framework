//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <nlohmann/json.hpp>

#include "ten_runtime/binding/cpp/aptima.h"
#include "ten_utils/macro/check.h"

class ffmpeg_client_extension : public aptima::extension_t {
 public:
  explicit ffmpeg_client_extension(const char *name) : aptima::extension_t(name) {}

  void on_start(aptima::ten_env_t &ten_env) override {
    auto cmd = aptima::cmd_t::create("prepare_demuxer");
    ten_env.send_cmd(
        std::move(cmd), [](aptima::ten_env_t &ten_env,
                           std::unique_ptr<aptima::cmd_result_t> cmd_result,
                           TEN_UNUSED aptima::error_t * /*error*/) {
          nlohmann::json cmd_result_json =
              nlohmann::json::parse(cmd_result->get_property_to_json());
          if (cmd_result->get_status_code() != TEN_STATUS_CODE_OK) {
            TEN_ASSERT(0, "should not happen.");
          }

          auto start_muxer_cmd = aptima::cmd_t::create("start_muxer");
          start_muxer_cmd->set_property_from_json(
              nullptr, nlohmann::to_string(cmd_result_json).c_str());
          ten_env.send_cmd(
              std::move(start_muxer_cmd),
              [](aptima::ten_env_t &ten_env,
                 std::unique_ptr<aptima::cmd_result_t> cmd_result,
                 TEN_UNUSED aptima::error_t * /*error*/) {
                nlohmann::json json =
                    nlohmann::json::parse(cmd_result->get_property_to_json());
                if (cmd_result->get_status_code() != TEN_STATUS_CODE_OK) {
                  TEN_ASSERT(0, "should not happen.");
                }

                auto start_demuxer_cmd = aptima::cmd_t::create("start_demuxer");
                ten_env.send_cmd(std::move(start_demuxer_cmd));
                return true;
              });

          return true;
        });
    ten_env.on_start_done();
  }

  void on_cmd(aptima::ten_env_t &ten_env,
              std::unique_ptr<aptima::cmd_t> cmd) override {
    const auto cmd_name = cmd->get_name();

    if (std::string(cmd_name) == "muxer_complete") {
      muxer_completed = true;
      auto cmd_result = aptima::cmd_result_t::create(TEN_STATUS_CODE_OK);
      cmd_result->set_property("detail", "good");
      ten_env.return_result(std::move(cmd_result), std::move(cmd));

      if (muxer_completed && demuxer_completed) {
        close_app(ten_env);
      }
    }

    if (std::string(cmd_name) == "demuxer_complete") {
      demuxer_completed = true;

      auto cmd_result = aptima::cmd_result_t::create(TEN_STATUS_CODE_OK);
      cmd_result->set_property("detail", "good");
      ten_env.return_result(std::move(cmd_result), std::move(cmd));

      if (muxer_completed && demuxer_completed) {
        close_app(ten_env);
      }
    }
  }

  void on_video_frame(
      aptima::ten_env_t &ten_env,
      TEN_UNUSED std::unique_ptr<aptima::video_frame_t> frame) override {
    // bypass
    ten_env.send_video_frame(std::move(frame));
  }

  void on_audio_frame(
      aptima::ten_env_t &ten_env,
      TEN_UNUSED std::unique_ptr<aptima::audio_frame_t> frame) override {
    // bypass
    ten_env.send_audio_frame(std::move(frame));
  }

  static void close_app(aptima::ten_env_t &ten_env) {
    auto close_cmd = aptima::cmd_close_app_t::create();
    close_cmd->set_dest("localhost", "", "", "");
    ten_env.send_cmd(std::move(close_cmd));
  }

  bool muxer_completed{false};
  bool demuxer_completed{false};
};

TEN_CPP_REGISTER_ADDON_AS_EXTENSION(ffmpeg_client, ffmpeg_client_extension);
