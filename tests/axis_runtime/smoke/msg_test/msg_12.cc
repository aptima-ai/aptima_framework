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
#include "include_internal/axis_runtime/binding/cpp/ten.h"
#include "axis_runtime/binding/cpp/detail/msg/cmd/cmd.h"
#include "axis_utils/lib/thread.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/axis_runtime/smoke/util/binding/cpp/check.h"

#define TEST_DATA 12344321

namespace {

class test_extension_1 : public ten::extension_t {
 public:
  explicit test_extension_1(const char *name) : ten::extension_t(name) {}

  void on_configure(ten::axis_env_t &axis_env) override {
    // clang-format off

    bool rc = ten::axis_env_internal_accessor_t::init_manifest_from_json(axis_env,
                 R"({
                      "type": "extension",
                      "name": "msg_12__extension_1",
                      "version": "0.1.0",
                      "api": {
                        "cmd_out": [
                          {
                            "name": "test",
                            "property": {
                              "test_data": {
                                "type": "int32"
                              }
                            }
                          }
                        ]
                      }
                    })");
    // clang-format on
    ASSERT_EQ(rc, true);

    axis_env.on_configure_done();
  }

  void on_cmd(ten::axis_env_t &axis_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      hello_world_cmd = std::move(cmd);

      auto test_cmd = ten::cmd_t::create("test");
      test_cmd->set_property("test_data", TEST_DATA);

      axis_env.send_cmd(
          std::move(test_cmd),
          [this](ten::axis_env_t &axis_env,
                 std::unique_ptr<ten::cmd_result_t> cmd, ten::error_t *err) {
            auto cmd_result = ten::cmd_result_t::create(axis_STATUS_CODE_OK);
            cmd_result->set_property("detail",
                                     cmd->get_property_string("detail"));
            axis_env.return_result(std::move(cmd_result),
                                  std::move(hello_world_cmd));
          });
    }
  }

 private:
  std::unique_ptr<ten::cmd_t> hello_world_cmd;
};

class test_extension_2 : public ten::extension_t {
 public:
  explicit test_extension_2(const char *name) : ten::extension_t(name) {}

  void on_cmd(ten::axis_env_t &axis_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (cmd->get_name() == "test") {
      auto const test_data = cmd->get_property_int32("test_data");
      axis_ASSERT(test_data == TEST_DATA, "Invalid argument.");

      auto cmd_result = ten::cmd_result_t::create(axis_STATUS_CODE_OK);
      cmd_result->set_property("detail", "hello world, too");
      axis_env.return_result(std::move(cmd_result), std::move(cmd));
    }
  }
};

class test_app : public ten::app_t {
 public:
  void on_configure(ten::axis_env_t &axis_env) override {
    bool rc = axis_env.init_property_from_json(
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

    axis_env.on_configure_done();
  }
};

void *test_app_thread_main(axis_UNUSED void *arg) {
  auto *app = new test_app();
  app->run();
  delete app;

  return nullptr;
}

axis_CPP_REGISTER_ADDON_AS_EXTENSION(msg_12__extension_1, test_extension_1);
axis_CPP_REGISTER_ADDON_AS_EXTENSION(msg_12__extension_2, test_extension_2);

}  // namespace

TEST(MsgTest, Msg12) {  // NOLINT
  // Start app.
  auto *app_thread =
      axis_thread_create("app thread", test_app_thread_main, nullptr);

  // Create a client and connect to the app.
  auto *client = new ten::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

  // Send graph.
  auto start_graph_cmd = ten::cmd_start_graph_t::create();
  start_graph_cmd->set_graph_from_json(R"({
           "nodes": [{
               "type": "extension",
               "name": "msg_12__extension_1",
               "addon": "msg_12__extension_1",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "msg_12__extension_group_1"
             },{
               "type": "extension",
               "name": "msg_12__extension_2",
               "addon": "msg_12__extension_2",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "msg_12__extension_group_2"
             }],
             "connections": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension": "msg_12__extension_1",
               "cmd": [{
                 "name": "test",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension": "msg_12__extension_2"
                 }]
               }]
             }]
           })");
  auto cmd_result =
      client->send_cmd_and_recv_result(std::move(start_graph_cmd));
  axis_test::check_status_code(cmd_result, axis_STATUS_CODE_OK);

  // Send a user-defined 'hello world' command.
  auto hello_world_cmd = ten::cmd_t::create("hello_world");
  hello_world_cmd->set_dest("msgpack://127.0.0.1:8001/", nullptr,
                            "msg_12__extension_group_1", "msg_12__extension_1");
  cmd_result = client->send_cmd_and_recv_result(std::move(hello_world_cmd));
  axis_test::check_status_code(cmd_result, axis_STATUS_CODE_OK);
  axis_test::check_detail_with_string(cmd_result, "hello world, too");

  delete client;

  axis_thread_join(app_thread, -1);
}
