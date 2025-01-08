//
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#include "demuxer_thread.h"
#include "axis_runtime/binding/cpp/ten.h"

namespace ten {
namespace ffmpeg_extension {

class demuxer_extension_t : public extension_t {
 public:
  explicit demuxer_extension_t(const char *name) : extension_t(name) {}

  void on_start(axis_env_t &axis_env) override { axis_env.on_start_done(); }

  void on_cmd(axis_env_t &axis_env, std::unique_ptr<ten::cmd_t> cmd) override {
    const auto cmd_name = cmd->get_name();

    if (std::string(cmd_name) == "prepare_demuxer") {
      auto input_stream_name = cmd->get_property_string("input_stream");
      if (input_stream_name.empty()) {
        input_stream_name =
            "axis_packages/extension/ffmpeg_demuxer/res/test.mp4";
      }

      auto *axis_env_proxy = ten::axis_env_proxy_t::create(axis_env);

      // Start the demuxer thread. ffmpeg is living in its own thread.
      demuxer_thread_ = new demuxer_thread_t(axis_env_proxy, std::move(cmd),
                                             this, input_stream_name);
      demuxer_thread_->start();
      // The cmd will be replied in the demuxer thread.
    }

    if (std::string(cmd_name) == "start_demuxer") {
      if (demuxer_thread_ == nullptr) {
        auto cmd_result = ten::cmd_result_t::create(axis_STATUS_CODE_ERROR);
        cmd_result->set_property("detail", "You should prepare first.");
        axis_env.return_result(std::move(cmd_result), std::move(cmd));
      } else {
        demuxer_thread_->start_demuxing();
        auto cmd_result = ten::cmd_result_t::create(axis_STATUS_CODE_OK);
        cmd_result->set_property("detail", "The demuxer has been started.");
        axis_env.return_result(std::move(cmd_result), std::move(cmd));
      }
    }
  }

  void on_stop(axis_UNUSED axis_env_t &axis_env) override {
    // Stop the demuxer thread. ffmpeg is living in its own thread.
    if (demuxer_thread_ != nullptr) {
      demuxer_thread_->stop();
      demuxer_thread_->wait_for_stop();
      delete demuxer_thread_;
    }
    axis_env.on_stop_done();
  }

 private:
  demuxer_thread_t *demuxer_thread_{};
};

}  // namespace ffmpeg_extension
}  // namespace ten

axis_CPP_REGISTER_ADDON_AS_EXTENSION(ffmpeg_demuxer,
                                    ten::ffmpeg_extension::demuxer_extension_t);
