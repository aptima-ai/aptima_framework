//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "aptima_runtime/binding/cpp/aptima.h"

namespace default_extension {

class default_extension_t : public aptima::extension_t {
 public:
  explicit default_extension_t(const char *name) : extension_t(name) {}

  void on_init(aptima::aptima_env_t &aptima_env) override { aptima_env.on_init_done(); }

  void on_start(aptima::aptima_env_t &aptima_env) override { aptima_env.on_start_done(); }

  void on_cmd(aptima::aptima_env_t &aptima_env,
              std::unique_ptr<aptima::cmd_t> cmd) override {
    // Receive cmd from aptima graph.
    // You can write your logic here.

    const int size = 2 * 1024 * 1024;
    std::string str(size, 'a');

    auto cmd_result = aptima::cmd_result_t::create(aptima_STATUS_CODE_OK);
    cmd_result->set_property("detail", str);
    aptima_env.return_result(std::move(cmd_result), std::move(cmd));
  }

  void on_data(aptima::aptima_env_t &aptima_env,
               std::unique_ptr<aptima::data_t> data) override {
    // Receive data from aptima graph.
  }

  void on_audio_frame(aptima::aptima_env_t &aptima_env,
                      std::unique_ptr<aptima::audio_frame_t> frame) override {
    // Receive audio frame from aptima graph.
  }

  void on_video_frame(aptima::aptima_env_t &aptima_env,
                      std::unique_ptr<aptima::video_frame_t> frame) override {
    // Receive video frame from aptima graph.
  }

  void on_stop(aptima::aptima_env_t &aptima_env) override {
    // Extension stop.
    aptima_env.on_stop_done();
  }
};

aptima_CPP_REGISTER_ADDON_AS_EXTENSION(default_extension_cpp, default_extension_t);

}  // namespace default_extension
