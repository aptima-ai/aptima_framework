//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "gtest/gtest.h"
#include "include_internal/aptima_runtime/binding/cpp/aptima.h"
#include "aptima_runtime/binding/cpp/detail/extension.h"
#include "aptima_runtime/common/status_code.h"
#include "aptima_utils/lang/cpp/lib/value.h"
#include "tests/aptima_runtime/smoke/util/binding/cpp/check.h"

namespace {

// This part is the extension codes written by the developer, maintained in its
// final release form, and will not change due to testing requirements.

class test_extension_1 : public aptima::extension_t {
 public:
  explicit test_extension_1(const char *name) : aptima::extension_t(name) {}

  void on_data(aptima::aptima_env_t &aptima_env,
               std::unique_ptr<aptima::data_t> data) override {
    if (std::string(data->get_name().c_str()) == "test") {
      data_frame_recv_count_++;

      return_if_possible(aptima_env);
    } else {
      aptima_ASSERT(0, "Should not happen.");
    }
  }

  void on_audio_frame(
      aptima::aptima_env_t &aptima_env,
      std::unique_ptr<aptima::audio_frame_t> audio_frame) override {
    if (std::string(audio_frame->get_name().c_str()) == "test") {
      data_frame_recv_count_++;

      return_if_possible(aptima_env);
    } else {
      aptima_ASSERT(0, "Should not happen.");
    }
  }

  void on_video_frame(
      aptima::aptima_env_t &aptima_env,
      std::unique_ptr<aptima::video_frame_t> video_frame) override {
    if (std::string(video_frame->get_name().c_str()) == "test") {
      data_frame_recv_count_++;

      return_if_possible(aptima_env);
    } else {
      aptima_ASSERT(0, "Should not happen.");
    }
  }

  void on_cmd(aptima::aptima_env_t &aptima_env,
              std::unique_ptr<aptima::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      // Cache the cmd and return it later.
      cached_cmd_ = std::move(cmd);

      return_if_possible(aptima_env);
    } else {
      aptima_ASSERT(0, "Should not happen.");
    }
  }

  void return_if_possible(aptima::aptima_env_t &aptima_env) {
    if (cached_cmd_ && data_frame_recv_count_ == 3) {
      auto cmd_result = aptima::cmd_result_t::create(aptima_STATUS_CODE_OK);
      cmd_result->set_property("detail", "hello world, too");
      bool rc =
          aptima_env.return_result(std::move(cmd_result), std::move(cached_cmd_));
      EXPECT_EQ(rc, true);
    }
  }

 private:
  int data_frame_recv_count_ = 0;
  std::unique_ptr<aptima::cmd_t> cached_cmd_;
};

aptima_CPP_REGISTER_ADDON_AS_EXTENSION(
    standalone_test_basic_data__test_extension_1, test_extension_1);

}  // namespace

namespace {

class extension_tester_1 : public aptima::extension_tester_t {
 public:
  void on_start(aptima::aptima_env_tester_t &aptima_env) override {
    // Send the first command to the extension.
    auto new_cmd = aptima::cmd_t::create("hello_world");

    aptima_env.send_cmd(
        std::move(new_cmd),
        [](aptima::aptima_env_tester_t &aptima_env,
           std::unique_ptr<aptima::cmd_result_t> result, aptima::error_t *err) {
          if (result->get_status_code() == aptima_STATUS_CODE_OK) {
            aptima_env.stop_test();
          }
        });

    aptima_env.send_data(aptima::data_t::create("test"));
    aptima_env.send_audio_frame(aptima::audio_frame_t::create("test"));
    aptima_env.send_video_frame(aptima::video_frame_t::create("test"));

    aptima_env.on_start_done();
  }
};

}  // namespace

TEST(StandaloneTest, BasicData) {  // NOLINT
  auto *tester = new extension_tester_1();
  tester->set_test_mode_single("standalone_test_basic_data__test_extension_1");

  bool rc = tester->run();
  aptima_ASSERT(rc, "Should not happen.");

  delete tester;
}
