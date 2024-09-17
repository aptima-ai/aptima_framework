//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <cstdlib>
#include <nlohmann/json.hpp>
#include <string>

#include "gtest/gtest.h"
#include "include_internal/ten_runtime/binding/cpp/ten.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/lib/thread.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/ten_runtime/smoke/extension_test/util/binding/cpp/check.h"

#define WIDTH 640
#define HEIGHT 480

namespace {

class test_extension_1 : public ten::extension_t {
 public:
  explicit test_extension_1(const std::string &name) : ten::extension_t(name) {}

  static std::unique_ptr<ten::video_frame_t> create420Buffer(int width,
                                                             int height) {
    auto video_frame = ten::video_frame_t::create("video_frame");
    video_frame->set_pixel_fmt(TEN_PIXEL_FMT_I420);
    video_frame->set_width(width);
    video_frame->set_height(height);

    auto buf_size = width * height * 3 / 2;
    video_frame->alloc_buf(buf_size + 32);

    return video_frame;
  }

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    nlohmann::json json = nlohmann::json::parse(cmd->to_json());
    if (json["_ten"]["name"] == "dispatch_data") {
      auto video_frame = create420Buffer(WIDTH, HEIGHT);
      video_frame->set_property("test_prop", "test_prop_value");

      ten_env.send_video_frame(std::move(video_frame));

      auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
      cmd_result->set_property("detail", "done");
      ten_env.return_result(std::move(cmd_result), std::move(cmd));
    }
  }
};

class test_extension_2 : public ten::extension_t {
 public:
  explicit test_extension_2(const std::string &name) : ten::extension_t(name) {}

  void on_video_frame(
      TEN_UNUSED ten::ten_env_t &ten_env,
      std::unique_ptr<ten::video_frame_t> video_frame) override {
    auto test_value = video_frame->get_property_string("test_prop");
    TEN_ASSERT(test_value == "test_prop_value", "test_prop_value not match");

    if (video_frame->get_width() == WIDTH &&
        video_frame->get_height() == HEIGHT) {
      received = true;
    }
  }

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    nlohmann::json json = nlohmann::json::parse(cmd->to_json());
    if (json["_ten"]["name"] == "check_received") {
      if (received) {
        auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
        cmd_result->set_property("detail", "received confirmed");
        ten_env.return_result(std::move(cmd_result), std::move(cmd));
      } else {
        auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_ERROR);
        cmd_result->set_property("detail", "received failed");
        ten_env.return_result(std::move(cmd_result), std::move(cmd));
      }
    }
  }

 private:
  bool received{false};
};

class test_extension_3 : public ten::extension_t {
 public:
  explicit test_extension_3(const std::string &name) : ten::extension_t(name) {}

  void on_video_frame(
      TEN_UNUSED ten::ten_env_t &ten_env,
      std::unique_ptr<ten::video_frame_t> video_frame) override {
    auto test_value = video_frame->get_property_string("test_prop");
    TEN_ASSERT(test_value == "test_prop_value", "test_prop_value not match");

    if (video_frame->get_width() == WIDTH &&
        video_frame->get_height() == HEIGHT) {
      received = true;
    }
  }

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    nlohmann::json json = nlohmann::json::parse(cmd->to_json());
    if (json["_ten"]["name"] == "check_received") {
      if (received) {
        auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
        cmd_result->set_property("detail", "received confirmed");
        ten_env.return_result(std::move(cmd_result), std::move(cmd));
      } else {
        auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_ERROR);
        cmd_result->set_property("detail", "received failed");
        ten_env.return_result(std::move(cmd_result), std::move(cmd));
      }
    }
  }

 private:
  bool received{false};
};

class test_app : public ten::app_t {
 public:
  void on_init(ten::ten_env_t &ten_env) override {
    bool rc = ten_env.init_property_from_json(
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

    ten_env.on_init_done();
  }
};

void *test_app_thread_main(TEN_UNUSED void *args) {
  auto *app = new test_app();
  app->run();
  delete app;

  return nullptr;
}

TEN_CPP_REGISTER_ADDON_AS_EXTENSION(multi_dest_video_frame__extension_1,
                                    test_extension_1);
TEN_CPP_REGISTER_ADDON_AS_EXTENSION(multi_dest_video_frame__extension_2,
                                    test_extension_2);
TEN_CPP_REGISTER_ADDON_AS_EXTENSION(multi_dest_video_frame__extension_3,
                                    test_extension_3);

}  // namespace

TEST(VideoFrameTest, MultiDestVideoFrame) {  // NOLINT
  // Start app.
  auto *app_thread =
      ten_thread_create("app thread", test_app_thread_main, nullptr);

  // Create a client and connect to the app.
  auto *client = new ten::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

  // Send graph.
  nlohmann::json resp = client->send_json_and_recv_resp_in_json(
      R"({
           "_ten": {
             "type": "start_graph",
             "seq_id": "55",
             "nodes": [{
               "type": "extension_group",
               "name": "test extension group",
               "addon": "default_extension_group",
               "app": "msgpack://127.0.0.1:8001/"
             },{
               "type": "extension",
               "name": "extension 1",
               "addon": "multi_dest_video_frame__extension_1",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "test extension group"
             },{
               "type": "extension",
               "name": "extension 2",
               "addon": "multi_dest_video_frame__extension_2",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "test extension group"
             },{
               "type": "extension",
               "name": "extension 3",
               "addon": "multi_dest_video_frame__extension_3",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "test extension group"
             }],
             "connections": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "test extension group",
               "extension": "extension 1",
               "video_frame": [{
                 "name": "video_frame",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "test extension group",
                   "extension": "extension 2"
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "test extension group",
                   "extension": "extension 3"
                 }]
               }]
             }]
           }
         })"_json);
  ten_test::check_status_code_is(resp, TEN_STATUS_CODE_OK);

  // Send a user-defined 'dispatch_data' command.
  resp = client->send_json_and_recv_resp_in_json(
      R"({
           "_ten": {
             "name": "dispatch_data",
             "seq_id": "137",
             "dest": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "test extension group",
               "extension": "extension 1"
             }]
           }
         })"_json);
  ten_test::check_result_is(resp, "137", TEN_STATUS_CODE_OK, "done");

  resp = client->send_json_and_recv_resp_in_json(
      R"({
           "_ten": {
             "name": "check_received",
             "seq_id": "137",
             "dest": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "test extension group",
               "extension": "extension 2"
             }]
           }
         })"_json);
  ten_test::check_result_is(resp, "137", TEN_STATUS_CODE_OK,
                            "received confirmed");

  resp = client->send_json_and_recv_resp_in_json(
      R"({
           "_ten": {
             "name": "check_received",
             "seq_id": "137",
             "dest": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "test extension group",
               "extension": "extension 3"
             }]
           }
         })"_json);
  ten_test::check_result_is(resp, "137", TEN_STATUS_CODE_OK,
                            "received confirmed");

  delete client;

  ten_thread_join(app_thread, -1);
}
