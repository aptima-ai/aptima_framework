//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <nlohmann/json.hpp>
#include <string>

#include "gtest/gtest.h"
#include "include_internal/ten_runtime/binding/cpp/aptima.h"
#include "ten_utils/lib/thread.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/ten_runtime/smoke/util/binding/cpp/check.h"

#define TIMER_TIMES 1

namespace {

class test_extension : public aptima::extension_t {
 public:
  explicit test_extension(const char *name) : aptima::extension_t(name) {}

  void on_cmd(aptima::ten_env_t &ten_env,
              std::unique_ptr<aptima::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      hello_world_cmd = std::move(cmd);

      // Start a timer.
      auto timer_cmd = aptima::cmd_timer_t::create();
      timer_cmd->set_dest("localhost", nullptr, nullptr, nullptr);
      timer_cmd->set_timer_id(55);
      timer_cmd->set_timeout_in_us(100);
      timer_cmd->set_times(TIMER_TIMES);

      bool success = ten_env.send_cmd(std::move(timer_cmd));
      EXPECT_EQ(success, true);
    } else if (aptima::msg_internal_accessor_t::get_type(cmd.get()) ==
                   TEN_MSG_TYPE_CMD_TIMEOUT &&
               std::unique_ptr<aptima::cmd_timeout_t>(
                   static_cast<aptima::cmd_timeout_t *>(cmd.release()))
                       ->get_timer_id() == 55) {
      auto cmd_result = aptima::cmd_result_t::create(TEN_STATUS_CODE_OK);
      cmd_result->set_property("detail", "hello world, too");
      ten_env.return_result(std::move(cmd_result), std::move(hello_world_cmd));
    }
  }

 private:
  std::unique_ptr<aptima::cmd_t> hello_world_cmd;
};

class test_app : public aptima::app_t {
 public:
  void on_configure(aptima::ten_env_t &ten_env) override {
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

TEN_CPP_REGISTER_ADDON_AS_EXTENSION(msg_6__extension, test_extension);

}  // namespace

TEST(MsgTest, Msg6) {
  auto *app_thread =
      ten_thread_create("app thread", test_app_thread_main, nullptr);

  // Create a client and connect to the app.
  auto *client = new aptima::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

  // Send graph.
  auto start_graph_cmd = aptima::cmd_start_graph_t::create();
  start_graph_cmd->set_graph_from_json(R"({
           "nodes": [{
               "type": "extension",
               "name": "test_extension",
               "addon": "msg_6__extension",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "msg_6__extension_group"
             }]
           })");
  auto cmd_result =
      client->send_cmd_and_recv_result(std::move(start_graph_cmd));
  ten_test::check_status_code(cmd_result, TEN_STATUS_CODE_OK);

  // Send a user-defined 'hello world' command.
  auto hello_world_cmd = aptima::cmd_t::create("hello_world");
  hello_world_cmd->set_dest("msgpack://127.0.0.1:8001/", nullptr,
                            "msg_6__extension_group", "test_extension");
  cmd_result = client->send_cmd_and_recv_result(std::move(hello_world_cmd));
  ten_test::check_status_code(cmd_result, TEN_STATUS_CODE_OK);
  ten_test::check_detail_with_string(cmd_result, "hello world, too");

  delete client;

  ten_thread_join(app_thread, -1);
}
