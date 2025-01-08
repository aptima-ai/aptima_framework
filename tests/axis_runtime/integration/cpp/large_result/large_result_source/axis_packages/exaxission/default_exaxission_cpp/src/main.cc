//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_runtime/binding/cpp/ten.h"

namespace default_extension {

class default_extension_t : public ten::extension_t {
 public:
  explicit default_extension_t(const char *name) : extension_t(name) {}

  void on_init(ten::axis_env_t &axis_env) override { axis_env.on_init_done(); }

  void on_start(ten::axis_env_t &axis_env) override { axis_env.on_start_done(); }

  void on_cmd(ten::axis_env_t &axis_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    // Receive cmd from ten graph.
    // You can write your logic here.

    const int size = 2 * 1024 * 1024;
    std::string str(size, 'a');

    auto cmd_result = ten::cmd_result_t::create(axis_STATUS_CODE_OK);
    cmd_result->set_property("detail", str);
    axis_env.return_result(std::move(cmd_result), std::move(cmd));
  }

  void on_data(ten::axis_env_t &axis_env,
               std::unique_ptr<ten::data_t> data) override {
    // Receive data from ten graph.
  }

  void on_audio_frame(ten::axis_env_t &axis_env,
                      std::unique_ptr<ten::audio_frame_t> frame) override {
    // Receive audio frame from ten graph.
  }

  void on_video_frame(ten::axis_env_t &axis_env,
                      std::unique_ptr<ten::video_frame_t> frame) override {
    // Receive video frame from ten graph.
  }

  void on_stop(ten::axis_env_t &axis_env) override {
    // Extension stop.
    axis_env.on_stop_done();
  }
};

axis_CPP_REGISTER_ADDON_AS_EXTENSION(default_extension_cpp, default_extension_t);

}  // namespace default_extension
