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
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/aptima_runtime/smoke/util/binding/cpp/check.h"

namespace {

class test_extension_1 : public aptima::extension_t {
 public:
  explicit test_extension_1(const char *name) : aptima::extension_t(name) {}

  void on_cmd(aptima::aptima_env_t &aptima_env,
              std::unique_ptr<aptima::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      hello_world_cmd = std::move(cmd);
      auto hello_world_1_cmd = aptima::cmd_t::create("hello_world_1");
      aptima_env.send_cmd(
          std::move(hello_world_1_cmd),
          [this](aptima::aptima_env_t &aptima_env,
                 std::unique_ptr<aptima::cmd_result_t> result, aptima::error_t *err) {
            // Got result of 'hello world 1',
            // Now return result for 'hello world'
            auto cmd_result = aptima::cmd_result_t::create(aptima_STATUS_CODE_OK);
            cmd_result->set_property("detail", "hello world, too");
            aptima_env.return_result(std::move(cmd_result),
                                  std::move(hello_world_cmd));
          });
      return;
    } else if (cmd->get_name() == "hello_world_2") {
      auto cmd_result = aptima::cmd_result_t::create(aptima_STATUS_CODE_OK);
      cmd_result->set_property("detail", "hello world, too");
      aptima_env.return_result(std::move(cmd_result), std::move(cmd));
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
    if (cmd->get_name() == "hello_world_1") {
      // waiting for result
      pending_request = std::move(cmd);
      auto hello_world_2_cmd = aptima::cmd_t::create("hello_world_2");
      aptima_env.send_cmd(
          std::move(hello_world_2_cmd),
          [this](aptima::aptima_env_t &aptima_env,
                 std::unique_ptr<aptima::cmd_result_t> result, aptima::error_t *err) {
            // Got result of 'hello world 2'.
            // Now return result for 'hello world 1'
            auto cmd_result = aptima::cmd_result_t::create(aptima_STATUS_CODE_OK);
            cmd_result->set_property("detail", "hello world, too");
            aptima_env.return_result(std::move(cmd_result),
                                  std::move(pending_request));
          });
    }
  }

 private:
  std::unique_ptr<aptima::cmd_t> pending_request;
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

aptima_CPP_REGISTER_ADDON_AS_EXTENSION(basic_loop__extension_1, test_extension_1);
aptima_CPP_REGISTER_ADDON_AS_EXTENSION(basic_loop__extension_2, test_extension_2);

}  // namespace

TEST(BasicTest, Loop) {  // NOLINT
  auto *app_thread =
      aptima_thread_create("app thread", test_app_thread_main, nullptr);

  // Create a client and connect to the app.
  auto *client = new aptima::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

  // Send graph.
  auto start_graph_cmd = aptima::cmd_start_graph_t::create();
  start_graph_cmd->set_graph_from_json(R"({
           "nodes": [{
               "type": "extension",
               "name": "test_extension_1",
               "addon": "basic_loop__extension_1",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "test_extension_group"
             },{
               "type": "extension",
               "name": "test_extension_2",
               "addon": "basic_loop__extension_2",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "test_extension_group"
             }],
             "connections": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension": "test_extension_1",
               "cmd": [{
                  "name": "hello_world_1",
                  "dest": [{
                     "app": "msgpack://127.0.0.1:8001/",
                     "extension": "test_extension_2"
                  }]
                }]
             },{
              "app": "msgpack://127.0.0.1:8001/",
              "extension": "test_extension_2",
              "cmd": [{
                 "name": "hello_world_2",
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
  auto hello_world_cmd = aptima::cmd_t::create("hello_world");
  hello_world_cmd->set_dest("msgpack://127.0.0.1:8001/", nullptr,
                            "test_extension_group", "test_extension_1");
  cmd_result = client->send_cmd_and_recv_result(std::move(hello_world_cmd));
  aptima_test::check_status_code(cmd_result, aptima_STATUS_CODE_OK);
  aptima_test::check_detail_with_string(cmd_result, "hello world, too");

  delete client;

  aptima_thread_join(app_thread, -1);
}
