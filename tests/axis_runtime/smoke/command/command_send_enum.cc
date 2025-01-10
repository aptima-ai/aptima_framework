//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <nlohmann/json.hpp>
#include <string>

#include "gtest/gtest.h"
#include "include_internal/aptima_runtime/binding/cpp/aptima.h"
#include "aptima_utils/lib/thread.h"
#include "aptima_utils/macro/check.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/aptima_runtime/smoke/util/binding/cpp/check.h"

namespace {

enum TEST_ENUM { TEST_1, TEST_2 };

class test_extension_1 : public aptima::extension_t {
 public:
  explicit test_extension_1(const char *name) : aptima::extension_t(name) {}

  void on_cmd(aptima::aptima_env_t &aptima_env,
              std::unique_ptr<aptima::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      hello_world_cmd = std::move(cmd);

      auto new_cmd = aptima::cmd_t::create("send enum");
      new_cmd->set_property("test data", TEST_1);

      aptima_env.send_cmd(
          std::move(new_cmd),
          [this](aptima::aptima_env_t &aptima_env,
                 std::unique_ptr<aptima::cmd_result_t> result, aptima::error_t *err) {
            nlohmann::json json =
                nlohmann::json::parse(result->get_property_to_json());

            auto cmd_result = aptima::cmd_result_t::create(aptima_STATUS_CODE_OK);
            cmd_result->set_property("detail", json.value("detail", ""));
            aptima_env.return_result(std::move(cmd_result),
                                  std::move(hello_world_cmd));
          });
    }
  }

 private:
  std::unique_ptr<aptima::cmd_t> hello_world_cmd;
};

class test_extension_2 : public aptima::extension_t {
 public:
  explicit test_extension_2(const char *name) : aptima::extension_t(name) {}

  void on_cmd(aptima::aptima_env_t &aptima_env,
              std::unique_ptr<aptima::cmd_t> cmd) override {
    if (cmd->get_name() == "send enum") {
      auto u_test_data = cmd->get_property_int32("test data");
      auto test_data = static_cast<TEST_ENUM>(u_test_data);
      if (test_data != TEST_1) {
        aptima_ASSERT(0, "Should not happen.");
      }

      auto cmd_result = aptima::cmd_result_t::create(aptima_STATUS_CODE_OK);
      cmd_result->set_property("detail", "hello world, too");
      aptima_env.return_result(std::move(cmd_result), std::move(cmd));
    }
  }
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

void *test_app_thread_main(aptima_UNUSED void *arg) {
  auto *app = new test_app();
  app->run();
  delete app;

  return nullptr;
}

aptima_CPP_REGISTER_ADDON_AS_EXTENSION(command_send_enum__extension_1,
                                    test_extension_1);
aptima_CPP_REGISTER_ADDON_AS_EXTENSION(command_send_enum__extension_2,
                                    test_extension_2);

}  // namespace

TEST(ExtensionTest, CommandSendEnum) {  // NOLINT
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
               "name": "command_send_enum__extension_1",
               "addon": "command_send_enum__extension_1",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "command_send_enum__extension_group_1"
             },{
               "type": "extension",
               "name": "command_send_enum__extension_2",
               "addon": "command_send_enum__extension_2",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "command_send_enum__extension_group_2"
             }],
             "connections": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension": "command_send_enum__extension_1",
               "cmd": [{
                 "name": "send enum",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension": "command_send_enum__extension_2"
                 }]
               }]
             }]
           })");
  auto cmd_result =
      client->send_cmd_and_recv_result(std::move(start_graph_cmd));
  aptima_test::check_status_code(cmd_result, aptima_STATUS_CODE_OK);

  // Send a user-defined 'hello world' command.
  auto hello_world_cmd = aptima::cmd_t::create("hello_world");
  hello_world_cmd->set_dest("msgpack://127.0.0.1:8001/", nullptr,
                            "command_send_enum__extension_group_1",
                            "command_send_enum__extension_1");
  cmd_result = client->send_cmd_and_recv_result(std::move(hello_world_cmd));
  aptima_test::check_status_code(cmd_result, aptima_STATUS_CODE_OK);
  aptima_test::check_detail_with_string(cmd_result, "hello world, too");

  delete client;

  aptima_thread_join(app_thread, -1);
}
