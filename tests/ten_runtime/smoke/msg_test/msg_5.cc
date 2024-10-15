//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "include_internal/ten_runtime/binding/cpp/ten.h"
#include "ten_utils/lib/thread.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/ten_runtime/smoke/extension_test/util/binding/cpp/check.h"

#define TIMER_TIMES 1

namespace {

class test_extension : public ten::extension_t {
 public:
  explicit test_extension(const std::string &name) : ten::extension_t(name) {}

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    nlohmann::json json = nlohmann::json::parse(cmd->to_json());
    if (json["_ten"]["name"] == "hello_world") {
      hello_world_cmd = std::move(cmd);

      // Start a timer.
      std::unique_ptr<ten::cmd_t> timer_cmd = ten::cmd_timer_t::create();
      timer_cmd->from_json(
          // clang-format off
                 R"({
                      "_ten": {
                        "dest": [{
                          "app": "localhost"
                        }],
                        "timer_id": 55,
                        "timeout_in_us": 100,
                        "times": 1
                      }
                    })"
          // clang-format on
      );

      bool success = ten_env.send_cmd(std::move(timer_cmd));
      EXPECT_EQ(success, true);
    } else if (json["_ten"]["type"] == "timeout" &&
               json["_ten"]["timer_id"].get<int64_t>() == 55) {
      auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
      cmd_result->set_property("detail", "hello world, too");
      ten_env.return_result(std::move(cmd_result), std::move(hello_world_cmd));
    }
  }

 private:
  std::unique_ptr<ten::cmd_t> hello_world_cmd;
};

class test_app : public ten::app_t {
 public:
  void on_configure(ten::ten_env_t &ten_env) override {
    ten_env.init_property_from_json(
        R"({
                      "_ten": {
                        "uri": "msgpack://127.0.0.1:8001/",
                        "log_level": 2
                      }
                    })");
    ten_env.on_configure_done();
  }
};

void *test_app_thread_main(TEN_UNUSED void *args) {
  auto *app = new test_app();
  app->run();
  delete app;

  return nullptr;
}

TEN_CPP_REGISTER_ADDON_AS_EXTENSION(msg_5__extension, test_extension);

}  // namespace

TEST(MsgTest, Msg5) {
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
               "type": "extension",
               "name": "test_extension",
               "addon": "msg_5__extension",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "msg_5__extension_group"
             }]
           }
         })"_json);
  ten_test::check_status_code_is(resp, TEN_STATUS_CODE_OK);

  // Send a user-defined 'hello world' command.
  resp = client->send_json_and_recv_resp_in_json(
      R"({
           "_ten": {
             "name": "hello_world",
             "seq_id": "137",
             "dest":[{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "msg_5__extension_group",
               "extension": "test_extension"
             }]
           }
         })"_json);
  ten_test::check_result_is(resp, "137", TEN_STATUS_CODE_OK,
                            "hello world, too");

  delete client;

  ten_thread_join(app_thread, -1);
}
