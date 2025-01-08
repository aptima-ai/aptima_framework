//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <nlohmann/json.hpp>
#include <string>

#include "gtest/gtest.h"
#include "include_internal/axis_runtime/binding/cpp/ten.h"
#include "axis_utils/lib/thread.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/axis_runtime/smoke/util/binding/cpp/check.h"

namespace {

class test_extension_1 : public ten::extension_t {
 public:
  explicit test_extension_1(const char *name) : ten::extension_t(name) {}

  void on_cmd(ten::axis_env_t &axis_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      axis_env.send_cmd_ex(
          std::move(cmd), [this](ten::axis_env_t &axis_env,
                                 std::unique_ptr<ten::cmd_result_t> cmd_result,
                                 ten::error_t *err) {
            ++received_result_cnt;

            if (received_result_cnt == 1) {
              axis_ENV_LOG_INFO(axis_env, "receives 1 cmd result");
            } else if (received_result_cnt == 2) {
              axis_ENV_LOG_INFO(axis_env, "receives 2 cmd result");
              axis_env.return_result_directly(std::move(cmd_result));
            }
          });
      return;
    }
  }

 private:
  int received_result_cnt{0};
};

class test_extension_2 : public ten::extension_t {
 public:
  explicit test_extension_2(const char *name) : ten::extension_t(name) {}

  void on_cmd(ten::axis_env_t &axis_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      auto cmd_result_1 = ten::cmd_result_t::create(axis_STATUS_CODE_OK);
      cmd_result_1->set_property("detail", "hello world, too");
      cmd_result_1->set_final(false);
      axis_env.return_result(std::move(cmd_result_1), std::move(cmd));

      auto cmd_result_2 = ten::cmd_result_t::create(axis_STATUS_CODE_OK);
      cmd_result_2->set_property("detail", "hello world, too");
      axis_env.return_result(std::move(cmd_result_2), std::move(cmd));
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

void *test_app_thread_main(axis_UNUSED void *args) {
  auto *app = new test_app();
  app->run();
  delete app;

  return nullptr;
}

axis_CPP_REGISTER_ADDON_AS_EXTENSION(multiple_result_1__test_extension_1,
                                    test_extension_1);
axis_CPP_REGISTER_ADDON_AS_EXTENSION(multiple_result_1__test_extension_2,
                                    test_extension_2);

}  // namespace

TEST(CmdResultTest, MultipleResult1) {  // NOLINT
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
                "name": "test_extension_1",
                "addon": "multiple_result_1__test_extension_1",
                "extension_group": "basic_extension_group",
                "app": "msgpack://127.0.0.1:8001/"
             },{
                "type": "extension",
                "name": "test_extension_2",
                "addon": "multiple_result_1__test_extension_2",
                "extension_group": "basic_extension_group",
                "app": "msgpack://127.0.0.1:8001/"
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
               }]
             }]
           })");
  auto cmd_result =
      client->send_cmd_and_recv_result(std::move(start_graph_cmd));
  axis_test::check_status_code(cmd_result, axis_STATUS_CODE_OK);

  // Send a user-defined 'hello world' command.
  auto hello_world_cmd = ten::cmd_t::create("hello_world");
  hello_world_cmd->set_dest("msgpack://127.0.0.1:8001/", nullptr,
                            "basic_extension_group", "test_extension_1");
  cmd_result = client->send_cmd_and_recv_result(std::move(hello_world_cmd));
  axis_test::check_status_code(cmd_result, axis_STATUS_CODE_OK);
  axis_test::check_detail_with_string(cmd_result, "hello world, too");

  delete client;

  axis_thread_join(app_thread, -1);
}
