//
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#include "demuxer_thread.h"
#include "aptima_runtime/binding/cpp/aptima.h"

namespace aptima {
namespace ffmpeg_extension {

class demuxer_extension_t : public extension_t {
 public:
  explicit demuxer_extension_t(const char *name) : extension_t(name) {}

  void on_start(aptima_env_t &aptima_env) override { aptima_env.on_start_done(); }

  void on_cmd(aptima_env_t &aptima_env, std::unique_ptr<aptima::cmd_t> cmd) override {
    const auto cmd_name = cmd->get_name();

    if (std::string(cmd_name) == "prepare_demuxer") {
      auto input_stream_name = cmd->get_property_string("input_stream");
      if (input_stream_name.empty()) {
        input_stream_name =
            "aptima_packages/extension/ffmpeg_demuxer/res/test.mp4";
      }

      auto *aptima_env_proxy = aptima::aptima_env_proxy_t::create(aptima_env);

      // Start the demuxer thread. ffmpeg is living in its own thread.
      demuxer_thread_ = new demuxer_thread_t(aptima_env_proxy, std::move(cmd),
                                             this, input_stream_name);
      demuxer_thread_->start();
      // The cmd will be replied in the demuxer thread.
    }

    if (std::string(cmd_name) == "start_demuxer") {
      if (demuxer_thread_ == nullptr) {
        auto cmd_result = aptima::cmd_result_t::create(aptima_STATUS_CODE_ERROR);
        cmd_result->set_property("detail", "You should prepare first.");
        aptima_env.return_result(std::move(cmd_result), std::move(cmd));
      } else {
        demuxer_thread_->start_demuxing();
        auto cmd_result = aptima::cmd_result_t::create(aptima_STATUS_CODE_OK);
        cmd_result->set_property("detail", "The demuxer has been started.");
        aptima_env.return_result(std::move(cmd_result), std::move(cmd));
      }
    }
  }

  void on_stop(aptima_UNUSED aptima_env_t &aptima_env) override {
    // Stop the demuxer thread. ffmpeg is living in its own thread.
    if (demuxer_thread_ != nullptr) {
      demuxer_thread_->stop();
      demuxer_thread_->wait_for_stop();
      delete demuxer_thread_;
    }
    aptima_env.on_stop_done();
  }

 private:
  demuxer_thread_t *demuxer_thread_{};
};

}  // namespace ffmpeg_extension
}  // namespace aptima

aptima_CPP_REGISTER_ADDON_AS_EXTENSION(ffmpeg_demuxer,
                                    aptima::ffmpeg_extension::demuxer_extension_t);
