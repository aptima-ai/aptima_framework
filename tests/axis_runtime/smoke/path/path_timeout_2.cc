//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <nlohmann/json.hpp>
#include <string>

#include "gtest/gtest.h"
#include "include_internal/aptima_runtime/binding/cpp/ten.h"
#include "aptima_utils/lib/thread.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/aptima_runtime/smoke/util/binding/cpp/check.h"

namespace {

class test_extension_1 : public ten::extension_t {
 public:
  explicit test_extension_1(const char *name) : ten::extension_t(name) {}

  void on_configure(ten::aptima_env_t &aptima_env) override {
    // Check path timeout every 1s. If any out_paths exist for more than 2s,
    // then they will be terminated.
    aptima_env.init_property_from_json(
        R"({
                      "_ten": {
                        "path_check_interval": 1000000,
                        "path_timeout": {
                          "in_path": 60000000,
                          "out_path": 1000000
                        }
                      }
                    })");
    aptima_env.on_configure_done();
  }

  void on_cmd(ten::aptima_env_t &aptima_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      // If the path table is not cleaned when stopped, then memory leak will be
      // detected.
      aptima_env.send_cmd(
          std::move(cmd),
          [](ten::aptima_env_t &aptima_env, std::unique_ptr<ten::cmd_result_t> cmd,
             ten::error_t *err) {
            aptima_env.return_result_directly(std::move(cmd));
          });
      return;
    }

    if (cmd->get_name() == "hello_world2") {
      aptima_env.send_cmd(std::move(cmd));
      return;
    }
  }
};

class test_extension_2 : public ten::extension_t {
 public:
  explicit test_extension_2(const char *name) : ten::extension_t(name) {}

  void on_cmd(ten::aptima_env_t &aptima_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      // Return nothing.
    }

    if (cmd->get_name() == "hello_world2") {
      auto cmd_result = ten::cmd_result_t::create(aptima_STATUS_CODE_OK);
      cmd_result->set_property("detail", "hello world, too");
      aptima_env.return_result(std::move(cmd_result), std::move(cmd));
    }
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

aptima_CPP_REGISTER_ADDON_AS_EXTENSION(path_timeout_2__test_extension_1,
                                    test_extension_1);
aptima_CPP_REGISTER_ADDON_AS_EXTENSION(path_timeout_2__test_extension_2,
                                    test_extension_2);

}  // namespace

TEST(ExtensionTest, PathTimeout2) {  // NOLINT
  // Start app.
  auto *app_thread =
      aptima_thread_create("app thread", test_app_thread_main, nullptr);

  // Create a client and connect to the app.
  auto *client = new ten::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

  // Send graph.
  auto start_graph_cmd = ten::cmd_start_graph_t::create();
  start_graph_cmd->set_graph_from_json(R"({
           "nodes": [{
               "type": "extension",
               "name": "test_extension_1",
               "addon": "path_timeout_2__test_extension_1",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "basic_extension_group"
             },{
               "type": "extension",
               "name": "test_extension_2",
               "addon": "path_timeout_2__test_extension_2",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "basic_extension_group"
             }],
             "connections": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension": "test_extension_1",
               "cmd": [{
                 "name": "hello_world",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension": "test_extension_2"
                 }]
               },{
                 "name": "hello_world2",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension": "test_extension_2"
                 }]
               }]
             }]
           })");
  auto cmd_result =
      client->send_cmd_and_recv_result(std::move(start_graph_cmd));
  aptima_test::check_status_code(cmd_result, aptima_STATUS_CODE_OK);

  // Send 'hello world' command and wait for the result.
  auto hello_world_cmd = ten::cmd_t::create("hello_world");
  hello_world_cmd->set_dest("msgpack://127.0.0.1:8001/", nullptr,
                            "basic_extension_group", "test_extension_1");
  cmd_result = client->send_cmd_and_recv_result(std::move(hello_world_cmd));
  aptima_test::check_status_code(cmd_result, aptima_STATUS_CODE_ERROR);
  aptima_test::check_detail_with_string(cmd_result, "Path timeout.");

  // Send a user-defined 'hello world2' command.
  auto hello_world2_cmd = ten::cmd_t::create("hello_world2");
  hello_world2_cmd->set_dest("msgpack://127.0.0.1:8001/", nullptr,
                             "basic_extension_group", "test_extension_1");
  cmd_result = client->send_cmd_and_recv_result(std::move(hello_world2_cmd));
  aptima_test::check_status_code(cmd_result, aptima_STATUS_CODE_OK);
  aptima_test::check_detail_with_string(cmd_result, "hello world, too");

  delete client;

  aptima_thread_join(app_thread, -1);
}
