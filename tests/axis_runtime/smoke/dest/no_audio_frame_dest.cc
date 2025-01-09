//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <memory>
#include <nlohmann/json.hpp>
#include <string>

#include "gtest/gtest.h"
#include "include_internal/aptima_runtime/binding/cpp/ten.h"
#include "aptima_runtime/binding/cpp/detail/msg/cmd/start_graph.h"
#include "aptima_runtime/binding/cpp/detail/aptima_env.h"
#include "aptima_utils/lang/cpp/lib/error.h"
#include "aptima_utils/lib/thread.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/aptima_runtime/smoke/util/binding/cpp/check.h"

namespace {

class test_extension_1 : public ten::extension_t {
 public:
  explicit test_extension_1(const char *name) : ten::extension_t(name) {}

  void on_cmd(ten::aptima_env_t &aptima_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      hello_world_cmd = std::move(cmd);

      ten::error_t err;

      auto audio_frame_no_dest =
          ten::audio_frame_t::create("audio_frame_no_dest");
      bool rc = aptima_env.send_audio_frame(std::move(audio_frame_no_dest),
                                         nullptr, &err);
      ASSERT_EQ(rc, false);
      ASSERT_EQ(err.is_success(), false);

      aptima_ENV_LOG_ERROR(aptima_env, err.err_msg());

      auto audio_frame = ten::audio_frame_t::create("audio_frame");
      rc = aptima_env.send_audio_frame(std::move(audio_frame));
      ASSERT_EQ(rc, true);
    } else if (cmd->get_name() == "audio_frame_ack") {
      auto cmd_result = ten::cmd_result_t::create(aptima_STATUS_CODE_OK);
      cmd_result->set_property("detail", "hello world, too");
      bool rc = aptima_env.return_result(std::move(cmd_result),
                                      std::move(hello_world_cmd));
      ASSERT_EQ(rc, true);
    }
  }

 private:
  std::unique_ptr<ten::cmd_t> hello_world_cmd;
};

class test_extension_2 : public ten::extension_t {
 public:
  explicit test_extension_2(const char *name) : ten::extension_t(name) {}

  void on_audio_frame(
      ten::aptima_env_t &aptima_env,
      std::unique_ptr<ten::audio_frame_t> audio_frame) override {
    auto cmd = ten::cmd_t::create("audio_frame_ack");
    aptima_env.send_cmd(std::move(cmd));
  }
};

class test_app : public ten::app_t {
 public:
  void on_configure(ten::aptima_env_t &aptima_env) override {
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

aptima_CPP_REGISTER_ADDON_AS_EXTENSION(no_audio_frame_dest__test_extension_1,
                                    test_extension_1);
aptima_CPP_REGISTER_ADDON_AS_EXTENSION(no_audio_frame_dest__test_extension_2,
                                    test_extension_2);

}  // namespace

TEST(ExtensionTest, NoAudioFrameDest) {  // NOLINT
  // Start app.
  auto *app_thread =
      aptima_thread_create("app thread", test_app_thread_main, nullptr);

  // Create a client and connect to the app.
  auto *client = new ten::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

  // Send graph.
  auto start_graph_cmd = ten::cmd_start_graph_t::create();
  start_graph_cmd->set_graph_from_json(
      R"({
             "nodes": [{
                "type": "extension",
                "name": "test_extension_1",
                "addon": "no_audio_frame_dest__test_extension_1",
                "extension_group": "basic_extension_group",
                "app": "msgpack://127.0.0.1:8001/"
             },{
                "type": "extension",
                "name": "test_extension_2",
                "addon": "no_audio_frame_dest__test_extension_2",
                "extension_group": "basic_extension_group",
                "app": "msgpack://127.0.0.1:8001/"
             }],
             "connections": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension": "test_extension_1",
               "audio_frame": [{
                 "name": "audio_frame",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension": "test_extension_2"
                 }]
               }]
             },{
               "app": "msgpack://127.0.0.1:8001/",
               "extension": "test_extension_2",
               "cmd": [{
                 "name": "audio_frame_ack",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension": "test_extension_1"
                 }]
               }]
             }]
         })");
  auto cmd_result =
      client->send_cmd_and_recv_result(std::move(start_graph_cmd));
  aptima_test::check_status_code(cmd_result, aptima_STATUS_CODE_OK);

  // Send a user-defined 'hello world' command.
  auto hello_world_cmd = ten::cmd_t::create("hello_world");
  hello_world_cmd->set_dest("msgpack://127.0.0.1:8001/", nullptr,
                            "basic_extension_group", "test_extension_1");
  cmd_result = client->send_cmd_and_recv_result(std::move(hello_world_cmd));
  aptima_test::check_status_code(cmd_result, aptima_STATUS_CODE_OK);
  aptima_test::check_detail_with_string(cmd_result, "hello world, too");

  delete client;

  aptima_thread_join(app_thread, -1);
}
