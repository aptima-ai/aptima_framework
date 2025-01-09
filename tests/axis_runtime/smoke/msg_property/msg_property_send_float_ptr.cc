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
#include "aptima_utils/lib/alloc.h"
#include "aptima_utils/lib/thread.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/aptima_runtime/smoke/util/binding/cpp/check.h"

#define TEST_DATA 12344321.8888F

namespace {

class test_extension_1 : public ten::extension_t {
 public:
  explicit test_extension_1(const char *name) : ten::extension_t(name) {}

  void on_cmd(ten::aptima_env_t &aptima_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      auto *test_data = static_cast<float *>(aptima_malloc(sizeof(float)));
      *test_data = TEST_DATA;

      auto new_cmd = ten::cmd_t::create("send_ptr");
      new_cmd->set_property("test data", test_data);
      hello_world_cmd = std::move(cmd);

      aptima_env.send_cmd(
          std::move(new_cmd),
          [this](ten::aptima_env_t &aptima_env,
                 std::unique_ptr<ten::cmd_result_t> cmd, ten::error_t *err) {
            auto cmd_result = ten::cmd_result_t::create(aptima_STATUS_CODE_OK);
            cmd_result->set_property(
                "detail", cmd->get_property_string("detail").c_str());
            aptima_env.return_result(std::move(cmd_result),
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

  void on_cmd(ten::aptima_env_t &aptima_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (cmd->get_name() == "send_ptr") {
      auto *test_data_ptr =
          static_cast<float *>(cmd->get_property_ptr("test data"));
      aptima_ASSERT(test_data_ptr, "Invalid argument.");
      aptima_ASSERT(*test_data_ptr == TEST_DATA, "Invalid argument.");

      aptima_free(test_data_ptr);

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

void *test_app_thread_main(aptima_UNUSED void *arg) {
  auto *app = new test_app();
  app->run();
  delete app;

  return nullptr;
}

aptima_CPP_REGISTER_ADDON_AS_EXTENSION(msg_property_send_float_ptr__extension_1,
                                    test_extension_1);
aptima_CPP_REGISTER_ADDON_AS_EXTENSION(msg_property_send_float_ptr__extension_2,
                                    test_extension_2);

}  // namespace

TEST(MsgPropertyTest, SendFloatPtr) {  // NOLINT
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
               "name": "msg_property_send_float_ptr__extension_1",
               "addon": "msg_property_send_float_ptr__extension_1",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "msg_property_send_float_ptr__extension_group_1"
             },{
               "type": "extension",
               "name": "msg_property_send_float_ptr__extension_2",
               "addon": "msg_property_send_float_ptr__extension_2",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "msg_property_send_float_ptr__extension_group_2"
             }],
             "connections": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension": "msg_property_send_float_ptr__extension_1",
               "cmd": [{
                 "name": "send_ptr",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension": "msg_property_send_float_ptr__extension_2"
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
                            "msg_property_send_float_ptr__extension_group_1",
                            "msg_property_send_float_ptr__extension_1");
  cmd_result = client->send_cmd_and_recv_result(std::move(hello_world_cmd));
  aptima_test::check_status_code(cmd_result, aptima_STATUS_CODE_OK);
  aptima_test::check_detail_with_string(cmd_result, "hello world, too");

  delete client;

  aptima_thread_join(app_thread, -1);
}
