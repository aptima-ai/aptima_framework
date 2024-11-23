//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <nlohmann/json.hpp>
#include <string>

#include "gtest/gtest.h"
#include "include_internal/ten_runtime/binding/cpp/ten.h"
#include "ten_utils/lib/thread.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/ten_runtime/smoke/extension_test/util/binding/cpp/check.h"

namespace {

class test_extension_1 : public ten::extension_t {
 public:
  explicit test_extension_1(const std::string &name) : ten::extension_t(name) {}

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    const auto *cmd_name = cmd->get_name();

    if (std::string(cmd_name) == "hello_world") {
      // Save the command for later using.
      hello_world_cmd = std::move(cmd);
      auto hello_world_1_cmd = ten::cmd_t::create("hello_world_1");
      ten_env.send_cmd(std::move(hello_world_1_cmd),
                       [this](ten::ten_env_t &ten_env,
                              std::unique_ptr<ten::cmd_result_t> cmd) {
                         // Got result of 'hello world 1',
                         // Now return result for 'hello world'
                         auto cmd_result =
                             ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
                         cmd_result->set_property("detail", "hello world, too");
                         ten_env.return_result(std::move(cmd_result),
                                               std::move(hello_world_cmd));
                       });
    } else if (std::string(cmd_name) == "hello_world_2") {
      auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
      cmd_result->set_property("detail", "hello world, too");
      ten_env.return_result(std::move(cmd_result), std::move(cmd));
    }
  }

 private:
  std::unique_ptr<ten::cmd_t> hello_world_cmd;
};

class test_extension_2 : public ten::extension_t {
 public:
  explicit test_extension_2(const std::string &name) : ten::extension_t(name) {}

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (std::string(cmd->get_name()) == "hello_world_1") {
      // waiting for result
      pending_request = std::move(cmd);
      auto hello_world_2_cmd = ten::cmd_t::create("hello_world_2");
      ten_env.send_cmd(
          std::move(hello_world_2_cmd),
          [this](TEN_UNUSED ten::ten_env_t &ten_env,
                 TEN_UNUSED std::unique_ptr<ten::cmd_result_t> cmd) {
            // Got result of 'hello world 2'.
            // Now return result for 'hello world 1'
            auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
            cmd_result->set_property("detail", "hello world, too");
            ten_env.return_result(std::move(cmd_result),
                                  std::move(pending_request));
          });
    }
  }

 private:
  std::unique_ptr<ten::cmd_t> pending_request;
};

class test_app : public ten::app_t {
 public:
  void on_configure(ten::ten_env_t &ten_env) override {
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

    ten_env.on_configure_done();
  }
};

void *test_app_thread_main(TEN_UNUSED void *args) {
  auto *app = new test_app();
  app->run();
  delete app;

  return nullptr;
}

TEN_CPP_REGISTER_ADDON_AS_EXTENSION(basic_loop_cmd_snapshot__extension_1,
                                    test_extension_1);
TEN_CPP_REGISTER_ADDON_AS_EXTENSION(basic_loop_cmd_snapshot__extension_2,
                                    test_extension_2);

}  // namespace

TEST(ExtensionTest, BasicLoopCmdSnapShot) {  // NOLINT
  auto *app_thread =
      ten_thread_create("app thread", test_app_thread_main, nullptr);

  // Create a client and connect to the app.
  auto *client = new ten::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

  // Send graph.
  auto start_graph_cmd = ten::cmd_start_graph_t::create();
  start_graph_cmd->set_nodes_and_connections_from_json(R"({
           "_ten": {"nodes": [{
               "type": "extension",
               "name": "test_extension_1",
               "addon": "basic_loop_cmd_snapshot__extension_1",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "test_extension_group"
             },{
               "type": "extension",
               "name": "test_extension_2",
               "addon": "basic_loop_cmd_snapshot__extension_2",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "test_extension_group"
             }],
             "connections": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "test_extension_group",
               "extension": "test_extension_1",
               "cmd": [{
                 "name": "hello_world_1",
                 "dest": [{
                    "app": "msgpack://127.0.0.1:8001/",
                    "extension_group": "test_extension_group",
                    "extension": "test_extension_2"
                 }]
               }]
             },{
              "app": "msgpack://127.0.0.1:8001/",
              "extension_group": "test_extension_group",
              "extension": "test_extension_2",
              "cmd": [{
                 "name": "hello_world_2",
                 "dest": [{
                    "app": "msgpack://127.0.0.1:8001/",
                    "extension_group": "test_extension_group",
                    "extension": "test_extension_1"
                 }]
               }]
             }]
           }
         })");
  auto cmd_result =
      client->send_cmd_and_recv_result(std::move(start_graph_cmd));
  ten_test::check_status_code(cmd_result, TEN_STATUS_CODE_OK);

  // Send a user-defined 'hello world' command.
  auto hello_world_cmd = ten::cmd_t::create("hello_world");
  hello_world_cmd->set_dest("msgpack://127.0.0.1:8001/", nullptr,
                            "test_extension_group", "test_extension_1");
  cmd_result = client->send_cmd_and_recv_result(std::move(hello_world_cmd));
  ten_test::check_status_code(cmd_result, TEN_STATUS_CODE_OK);
  ten_test::check_detail_with_string(cmd_result, "hello world, too");

  delete client;

  ten_thread_join(app_thread, -1);
}
