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
#include "aptima_utils/lib/thread.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/aptima_runtime/smoke/util/binding/cpp/check.h"

namespace {

class test_extension : public ten::extension_t {
 public:
  explicit test_extension(const char *name) : ten::extension_t(name) {}

  void on_cmd(ten::aptima_env_t &aptima_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      auto cmd_shared =
          std::make_shared<std::unique_ptr<ten::cmd_t>>(std::move(cmd));

      auto test_cmd = ten::cmd_t::create("test");
      test_cmd->set_dest("msgpack://127.0.0.1:8001/", "incorrect_graph_id",
                         "extension_send_msg_to_incorrect_engine",
                         "test_extension");
      aptima_env.send_cmd(
          std::move(test_cmd),
          [cmd_shared](ten::aptima_env_t &aptima_env,
                       std::unique_ptr<ten::cmd_result_t> cmd_result,
                       ten::error_t *err) {
            aptima_env.return_result(std::move(cmd_result),
                                  std::move(*cmd_shared));
            return true;
          });
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
                        "one_event_loop_per_engine": true,
                        "log_level": 2
                      }
                    })"
        // clang-format on
    );
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

aptima_CPP_REGISTER_ADDON_AS_EXTENSION(
    extension_send_msg_to_incorrect_engine__extension, test_extension);

}  // namespace

TEST(ExtensionTest, ExtensionSendMsgToIncorrectEngine) {  // NOLINT
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
               "name": "test_extension",
               "addon": "extension_send_msg_to_incorrect_engine__extension",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "extension_send_msg_to_incorrect_engine"
              }]
            })");
  auto cmd_result =
      client->send_cmd_and_recv_result(std::move(start_graph_cmd));
  aptima_test::check_status_code(cmd_result, aptima_STATUS_CODE_OK);

  // Send a user-defined 'hello world' command.
  auto hello_world_cmd = ten::cmd_t::create("hello_world");
  hello_world_cmd->set_dest("msgpack://127.0.0.1:8001/", nullptr,
                            "extension_send_msg_to_incorrect_engine",
                            "test_extension");

  cmd_result = client->send_cmd_and_recv_result(std::move(hello_world_cmd));

  aptima_test::check_status_code(cmd_result, aptima_STATUS_CODE_ERROR);
  aptima_test::check_detail_with_string(cmd_result, "Graph not found.");

  // Destroy the client.
  delete client;

  aptima_thread_join(app_thread, -1);
}
