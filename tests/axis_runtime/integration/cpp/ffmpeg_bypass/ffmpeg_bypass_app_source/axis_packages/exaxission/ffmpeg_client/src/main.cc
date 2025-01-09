//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <nlohmann/json.hpp>

#include "aptima_runtime/binding/cpp/ten.h"
#include "aptima_utils/macro/check.h"

class ffmpeg_client_extension : public ten::extension_t {
 public:
  explicit ffmpeg_client_extension(const char *name) : ten::extension_t(name) {}

  void on_start(ten::aptima_env_t &aptima_env) override {
    auto cmd = ten::cmd_t::create("prepare_demuxer");
    aptima_env.send_cmd(
        std::move(cmd), [](ten::aptima_env_t &aptima_env,
                           std::unique_ptr<ten::cmd_result_t> cmd_result,
                           aptima_UNUSED ten::error_t * /*error*/) {
          nlohmann::json cmd_result_json =
              nlohmann::json::parse(cmd_result->get_property_to_json());
          if (cmd_result->get_status_code() != aptima_STATUS_CODE_OK) {
            aptima_ASSERT(0, "should not happen.");
          }

          auto start_muxer_cmd = ten::cmd_t::create("start_muxer");
          start_muxer_cmd->set_property_from_json(
              nullptr, nlohmann::to_string(cmd_result_json).c_str());
          aptima_env.send_cmd(
              std::move(start_muxer_cmd),
              [](ten::aptima_env_t &aptima_env,
                 std::unique_ptr<ten::cmd_result_t> cmd_result,
                 aptima_UNUSED ten::error_t * /*error*/) {
                nlohmann::json json =
                    nlohmann::json::parse(cmd_result->get_property_to_json());
                if (cmd_result->get_status_code() != aptima_STATUS_CODE_OK) {
                  aptima_ASSERT(0, "should not happen.");
                }

                auto start_demuxer_cmd = ten::cmd_t::create("start_demuxer");
                aptima_env.send_cmd(std::move(start_demuxer_cmd));
                return true;
              });

          return true;
        });
    aptima_env.on_start_done();
  }

  void on_cmd(ten::aptima_env_t &aptima_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    const auto cmd_name = cmd->get_name();

    if (std::string(cmd_name) == "muxer_complete") {
      muxer_completed = true;
      auto cmd_result = ten::cmd_result_t::create(aptima_STATUS_CODE_OK);
      cmd_result->set_property("detail", "good");
      aptima_env.return_result(std::move(cmd_result), std::move(cmd));

      if (muxer_completed && demuxer_completed) {
        close_app(aptima_env);
      }
    }

    if (std::string(cmd_name) == "demuxer_complete") {
      demuxer_completed = true;

      auto cmd_result = ten::cmd_result_t::create(aptima_STATUS_CODE_OK);
      cmd_result->set_property("detail", "good");
      aptima_env.return_result(std::move(cmd_result), std::move(cmd));

      if (muxer_completed && demuxer_completed) {
        close_app(aptima_env);
      }
    }
  }

  void on_video_frame(
      ten::aptima_env_t &aptima_env,
      aptima_UNUSED std::unique_ptr<ten::video_frame_t> frame) override {
    // bypass
    aptima_env.send_video_frame(std::move(frame));
  }

  void on_audio_frame(
      ten::aptima_env_t &aptima_env,
      aptima_UNUSED std::unique_ptr<ten::audio_frame_t> frame) override {
    // bypass
    aptima_env.send_audio_frame(std::move(frame));
  }

  static void close_app(ten::aptima_env_t &aptima_env) {
    auto close_cmd = ten::cmd_close_app_t::create();
    close_cmd->set_dest("localhost", "", "", "");
    aptima_env.send_cmd(std::move(close_cmd));
  }

  bool muxer_completed{false};
  bool demuxer_completed{false};
};

aptima_CPP_REGISTER_ADDON_AS_EXTENSION(ffmpeg_client, ffmpeg_client_extension);
