//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_runtime/binding/cpp/detail/test/env_tester_proxy.h"
#include "aptima_runtime/binding/cpp/detail/test/extension_tester.h"
#include "aptima_utils/log/log.h"

namespace aptima {

using aptima_client_proxy_send_cmd_result_handler_func_t =
    std::function<void(std::unique_ptr<aptima::cmd_result_t>, error_t *err)>;

class aptima_client_proxy_event_handler_t {
 public:
  aptima_client_proxy_event_handler_t() = default;
  virtual ~aptima_client_proxy_event_handler_t() = default;

  aptima_client_proxy_event_handler_t(const aptima_client_proxy_event_handler_t &) =
      delete;
  aptima_client_proxy_event_handler_t(aptima_client_proxy_event_handler_t &&) =
      delete;
  aptima_client_proxy_event_handler_t &operator=(
      const aptima_client_proxy_event_handler_t &) = delete;
  aptima_client_proxy_event_handler_t &operator=(
      const aptima_client_proxy_event_handler_t &&) = delete;

  virtual void on_start() {}

  virtual void on_cmd(std::unique_ptr<cmd_t> cmd) {}

  virtual void on_data(std::unique_ptr<data_t> data) {}

  virtual void on_audio_frame(std::unique_ptr<audio_frame_t> audio_frame) {}

  virtual void on_video_frame(std::unique_ptr<video_frame_t> video_frame) {}
};

namespace {

class aptima_client_proxy_internal_impl_t : public aptima::extension_tester_t {
 protected:
  void on_start(aptima::aptima_env_tester_t &aptima_env_tester) override {
    aptima_env_tester_proxy_ = std::unique_ptr<aptima::aptima_env_tester_proxy_t>(
        aptima::aptima_env_tester_proxy_t::create(aptima_env_tester));
    aptima_ASSERT(aptima_env_tester_proxy_, "Should not happen.");

    if (event_handler_ != nullptr) {
      event_handler_->on_start();
    }

    aptima_env_tester.on_start_done();
  }

  void on_cmd(aptima::aptima_env_tester_t & /*aptima_env_tester*/,
              std::unique_ptr<aptima::cmd_t> cmd) override {
    if (event_handler_ != nullptr) {
      event_handler_->on_cmd(std::move(cmd));
    }
  }

  void on_data(aptima::aptima_env_tester_t & /*aptima_env_tester*/,
               std::unique_ptr<aptima::data_t> data) override {
    if (event_handler_ != nullptr) {
      event_handler_->on_data(std::move(data));
    }
  }

  void on_audio_frame(
      aptima::aptima_env_tester_t & /*aptima_env_tester*/,
      std::unique_ptr<aptima::audio_frame_t> audio_frame) override {
    if (event_handler_ != nullptr) {
      event_handler_->on_audio_frame(std::move(audio_frame));
    }
  }

  void on_video_frame(
      aptima::aptima_env_tester_t & /*aptima_env_tester*/,
      std::unique_ptr<aptima::video_frame_t> video_frame) override {
    if (event_handler_ != nullptr) {
      event_handler_->on_video_frame(std::move(video_frame));
    }
  }

  static void proxy_on_cmd_result(
      std::unique_ptr<aptima::cmd_result_t> cmd_result,
      const aptima::aptima_client_proxy_send_cmd_result_handler_func_t
          &result_handler,
      error_t *err) {
    if (result_handler) {
      result_handler(std::move(cmd_result), err);
    }
  }

 public:
  void register_callback(aptima::aptima_client_proxy_event_handler_t *event_handler) {
    event_handler_ = event_handler;
  }

  bool send_cmd(
      std::unique_ptr<aptima::cmd_t> cmd,
      aptima::aptima_client_proxy_send_cmd_result_handler_func_t &&result_handler) {
    aptima_ASSERT(aptima_env_tester_proxy_, "Invalid state.");

    if (aptima_env_tester_proxy_ == nullptr) {
      aptima_LOGE("Failed to send_cmd: %s before started.",
               cmd->get_name().c_str());
      return false;
    }

    auto cmd_shared =
        std::make_shared<std::unique_ptr<aptima::cmd_t>>(std::move(cmd));

    return aptima_env_tester_proxy_->notify(
        [cmd_shared, result_handler](aptima::aptima_env_tester_t &aptima_env_tester) {
          aptima_env_tester.send_cmd(
              std::move(*cmd_shared),
              [result_handler](aptima::aptima_env_tester_t & /*aptima_env_tester*/,
                               std::unique_ptr<aptima::cmd_result_t> cmd_result,
                               error_t *err) {
                proxy_on_cmd_result(std::move(cmd_result), result_handler, err);
              });
        },
        nullptr);
  }

  bool send_data(std::unique_ptr<aptima::data_t> data) {
    aptima_ASSERT(aptima_env_tester_proxy_, "Invalid state.");

    if (aptima_env_tester_proxy_ == nullptr) {
      aptima_LOGE("Failed to send_data before started.");
      return false;
    }

    auto data_shared =
        std::make_shared<std::unique_ptr<aptima::data_t>>(std::move(data));

    return aptima_env_tester_proxy_->notify(
        [data_shared](aptima::aptima_env_tester_t &env_tester) {
          env_tester.send_data(std::move(*data_shared));
        },
        nullptr);
  }

  bool send_audio_frame(std::unique_ptr<aptima::audio_frame_t> audio_frame) {
    aptima_ASSERT(aptima_env_tester_proxy_, "Invalid state.");

    if (aptima_env_tester_proxy_ == nullptr) {
      aptima_LOGE("Failed to send_audio_frame before started.");
      return false;
    }

    auto audio_frame_shared =
        std::make_shared<std::unique_ptr<aptima::audio_frame_t>>(
            std::move(audio_frame));

    return aptima_env_tester_proxy_->notify(
        [audio_frame_shared](aptima::aptima_env_tester_t &env_tester) {
          env_tester.send_audio_frame(std::move(*audio_frame_shared));
        },
        nullptr);
  }

  bool send_video_frame(std::unique_ptr<aptima::video_frame_t> video_frame) {
    aptima_ASSERT(aptima_env_tester_proxy_, "Invalid state.");

    if (aptima_env_tester_proxy_ == nullptr) {
      aptima_LOGE("Failed to send_video_frame before started.");
      return false;
    }

    auto video_frame_shared =
        std::make_shared<std::unique_ptr<aptima::video_frame_t>>(
            std::move(video_frame));

    return aptima_env_tester_proxy_->notify(
        [video_frame_shared](aptima::aptima_env_tester_t &env_tester) {
          env_tester.send_video_frame(std::move(*video_frame_shared));
        },
        nullptr);
  }

  bool stop() {
    if (aptima_env_tester_proxy_ == nullptr) {
      aptima_LOGE("Failed to stop before started.");
      return false;
    }

    return aptima_env_tester_proxy_->notify(
        [this](aptima::aptima_env_tester_t &env_tester) {
          env_tester.stop_test();
          aptima_env_tester_proxy_ = nullptr;
        },
        nullptr);
  }

 private:
  aptima::aptima_client_proxy_event_handler_t *event_handler_;

  // The thread-safety should be guaranteed by the caller.
  std::unique_ptr<aptima::aptima_env_tester_proxy_t> aptima_env_tester_proxy_;
};

}  // namespace

class aptima_client_proxy_t {
 public:
  aptima_client_proxy_t() = default;
  virtual ~aptima_client_proxy_t() = default;

  aptima_client_proxy_t(const aptima_client_proxy_t &) = delete;
  aptima_client_proxy_t(aptima_client_proxy_t &&) = delete;
  aptima_client_proxy_t &operator=(const aptima_client_proxy_t &) = delete;
  aptima_client_proxy_t &operator=(const aptima_client_proxy_t &&) = delete;

  void add_addon_base_dir(const char *addon_path) {
    aptima_ASSERT(addon_path, "Invalid argument.");
    impl_.add_addon_base_dir(addon_path);
  }

  void start_graph(const char *graph_json) {
    aptima_ASSERT(graph_json, "Invalid argument.");
    impl_.set_test_mode_graph(graph_json);
    impl_.run();
  }

  void stop() { impl_.stop(); }

  // These functions should be called after the on_start callback is received.

  void send_cmd(
      std::unique_ptr<cmd_t> cmd,
      aptima_client_proxy_send_cmd_result_handler_func_t &&result_handler) {
    impl_.send_cmd(std::move(cmd), std::move(result_handler));
  }

  void send_data(std::unique_ptr<data_t> data) {
    impl_.send_data(std::move(data));
  }

  void send_audio_frame(std::unique_ptr<audio_frame_t> audio_frame) {
    impl_.send_audio_frame(std::move(audio_frame));
  }

  void send_video_frame(std::unique_ptr<video_frame_t> video_frame) {
    impl_.send_video_frame(std::move(video_frame));
  }

  void register_event_handler(aptima_client_proxy_event_handler_t *event_handler) {
    impl_.register_callback(event_handler);
  }

 private:
  aptima_client_proxy_internal_impl_t impl_;
};

}  // namespace aptima
