//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <cstdlib>
#include <nlohmann/json.hpp>
#include <string>

#include "gtest/gtest.h"
#include "include_internal/aptima_runtime/binding/cpp/aptima.h"
#include "aptima_utils/lib/thread.h"
#include "aptima_utils/macro/check.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/aptima_runtime/smoke/util/binding/cpp/check.h"

#define WIDTH 640
#define HEIGHT 480

namespace {

class test_extension_1 : public aptima::extension_t {
 public:
  explicit test_extension_1(const char *name) : aptima::extension_t(name) {}

  static std::unique_ptr<aptima::video_frame_t> create420Buffer(int width,
                                                             int height) {
    auto video_frame = aptima::video_frame_t::create("video_frame");
    video_frame->set_pixel_fmt(aptima_PIXEL_FMT_I420);
    video_frame->set_width(width);
    video_frame->set_height(height);

    auto buf_size = width * height * 3 / 2;
    video_frame->alloc_buf(buf_size + 32);

    return video_frame;
  }

  void on_cmd(aptima::aptima_env_t &aptima_env,
              std::unique_ptr<aptima::cmd_t> cmd) override {
    if (cmd->get_name() == "dispatch_data") {
      auto video_frame = create420Buffer(WIDTH, HEIGHT);
      video_frame->set_property("test_prop", "test_prop_value");

      aptima_env.send_video_frame(std::move(video_frame));

      auto cmd_result = aptima::cmd_result_t::create(aptima_STATUS_CODE_OK);
      cmd_result->set_property("detail", "done");
      aptima_env.return_result(std::move(cmd_result), std::move(cmd));
    }
  }
};

class test_extension_2 : public aptima::extension_t {
 public:
  explicit test_extension_2(const char *name) : aptima::extension_t(name) {}

  void on_video_frame(
      aptima_UNUSED aptima::aptima_env_t &aptima_env,
      std::unique_ptr<aptima::video_frame_t> video_frame) override {
    auto test_value = video_frame->get_property_string("test_prop");
    aptima_ASSERT(test_value == "test_prop_value", "test_prop_value not match");

    if (video_frame->get_width() == WIDTH &&
        video_frame->get_height() == HEIGHT) {
      received = true;
    }
  }

  void on_cmd(aptima::aptima_env_t &aptima_env,
              std::unique_ptr<aptima::cmd_t> cmd) override {
    if (cmd->get_name() == "check_received") {
      if (received) {
        auto cmd_result = aptima::cmd_result_t::create(aptima_STATUS_CODE_OK);
        cmd_result->set_property("detail", "received confirmed");
        aptima_env.return_result(std::move(cmd_result), std::move(cmd));
      } else {
        auto cmd_result = aptima::cmd_result_t::create(aptima_STATUS_CODE_ERROR);
        cmd_result->set_property("detail", "received failed");
        aptima_env.return_result(std::move(cmd_result), std::move(cmd));
      }
    }
  }

 private:
  bool received{false};
};

class test_extension_3 : public aptima::extension_t {
 public:
  explicit test_extension_3(const char *name) : aptima::extension_t(name) {}

  void on_video_frame(
      aptima_UNUSED aptima::aptima_env_t &aptima_env,
      std::unique_ptr<aptima::video_frame_t> video_frame) override {
    auto test_value = video_frame->get_property_string("test_prop");
    aptima_ASSERT(test_value == "test_prop_value", "test_prop_value not match");

    if (video_frame->get_width() == WIDTH &&
        video_frame->get_height() == HEIGHT) {
      received = true;
    }
  }

  void on_cmd(aptima::aptima_env_t &aptima_env,
              std::unique_ptr<aptima::cmd_t> cmd) override {
    if (cmd->get_name() == "check_received") {
      if (received) {
        auto cmd_result = aptima::cmd_result_t::create(aptima_STATUS_CODE_OK);
        cmd_result->set_property("detail", "received confirmed");
        aptima_env.return_result(std::move(cmd_result), std::move(cmd));
      } else {
        auto cmd_result = aptima::cmd_result_t::create(aptima_STATUS_CODE_ERROR);
        cmd_result->set_property("detail", "received failed");
        aptima_env.return_result(std::move(cmd_result), std::move(cmd));
      }
    }
  }

 private:
  bool received{false};
};

class test_app : public aptima::app_t {
 public:
  void on_configure(aptima::aptima_env_t &aptima_env) override {
    bool rc = aptima_env.init_property_from_json(
        // clang-format off
                 R"({
                      "_ten": {
                        "uri": "msgpack://127.0.0.1:8001/",
                        "log_level": 2
                      }
                    })"
        // clang-format on
        ,
        nullptr);
    ASSERT_EQ(rc, true);

    aptima_env.on_configure_done();
  }
};

void *test_app_thread_main(aptima_UNUSED void *args) {
  auto *app = new test_app();
  app->run();
  delete app;

  return nullptr;
}

aptima_CPP_REGISTER_ADDON_AS_EXTENSION(multi_dest_video_frame__extension_1,
                                    test_extension_1);
aptima_CPP_REGISTER_ADDON_AS_EXTENSION(multi_dest_video_frame__extension_2,
                                    test_extension_2);
aptima_CPP_REGISTER_ADDON_AS_EXTENSION(multi_dest_video_frame__extension_3,
                                    test_extension_3);

}  // namespace

TEST(VideoFrameTest, MultiDestVideoFrame) {  // NOLINT
  // Start app.
  auto *app_thread =
      aptima_thread_create("app thread", test_app_thread_main, nullptr);

  // Create a client and connect to the app.
  auto *client = new aptima::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

  // Send graph.
  auto start_graph_cmd = aptima::cmd_start_graph_t::create();
  start_graph_cmd->set_graph_from_json(R"({
           "nodes": [{
               "type": "extension",
               "name": "extension 1",
               "addon": "multi_dest_video_frame__extension_1",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "test_extension_group"
             },{
               "type": "extension",
               "name": "extension 2",
               "addon": "multi_dest_video_frame__extension_2",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "test_extension_group"
             },{
               "type": "extension",
               "name": "extension 3",
               "addon": "multi_dest_video_frame__extension_3",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "test_extension_group"
             }],
             "connections": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension": "extension 1",
               "video_frame": [{
                 "name": "video_frame",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension": "extension 2"
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension": "extension 3"
                 }]
               }]
             }]
           })");
  auto cmd_result =
      client->send_cmd_and_recv_result(std::move(start_graph_cmd));
  aptima_test::check_status_code(cmd_result, aptima_STATUS_CODE_OK);

  // Send a user-defined 'dispatch_data' command.
  auto dispatch_data_cmd = aptima::cmd_t::create("dispatch_data");
  dispatch_data_cmd->set_dest("msgpack://127.0.0.1:8001/", nullptr,
                              "test_extension_group", "extension 1");
  cmd_result = client->send_cmd_and_recv_result(std::move(dispatch_data_cmd));
  aptima_test::check_status_code(cmd_result, aptima_STATUS_CODE_OK);
  aptima_test::check_detail_with_string(cmd_result, "done");

  auto check_received_cmd = aptima::cmd_t::create("check_received");
  check_received_cmd->set_dest("msgpack://127.0.0.1:8001/", nullptr,
                               "test_extension_group", "extension 2");
  cmd_result = client->send_cmd_and_recv_result(std::move(check_received_cmd));
  aptima_test::check_status_code(cmd_result, aptima_STATUS_CODE_OK);
  aptima_test::check_detail_with_string(cmd_result, "received confirmed");

  check_received_cmd = aptima::cmd_t::create("check_received");
  check_received_cmd->set_dest("msgpack://127.0.0.1:8001/", nullptr,
                               "test_extension_group", "extension 3");
  cmd_result = client->send_cmd_and_recv_result(std::move(check_received_cmd));
  aptima_test::check_status_code(cmd_result, aptima_STATUS_CODE_OK);
  aptima_test::check_detail_with_string(cmd_result, "received confirmed");

  delete client;

  aptima_thread_join(app_thread, -1);
}
