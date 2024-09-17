//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <memory>
#include <nlohmann/json.hpp>
#include <string>

#include "gtest/gtest.h"
#include "include_internal/ten_runtime/binding/cpp/ten.h"
#include "ten_utils/lib/thread.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/ten_runtime/smoke/extension_test/util/binding/cpp/check.h"

#define TEST_DATA 12344321

namespace {

class test_data_t {
 public:
  explicit test_data_t(int32_t v) : v_(new int32_t(v)) {}

  // This class done _not_ support move/copy constructor.
  test_data_t(const test_data_t &other) = delete;
  test_data_t(test_data_t &&other) = delete;

  ~test_data_t() { delete v_; };

  test_data_t &operator=(const test_data_t &other) = delete;
  test_data_t &operator=(test_data_t &&other) = delete;

  int32_t *v_;
};

class test_extension_1 : public ten::extension_t {
 public:
  explicit test_extension_1(const std::string &name) : ten::extension_t(name) {}

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (std::string(cmd->get_name()) == "hello_world") {
      auto new_cmd = ten::cmd_t::create("send_ptr");

      // Create a C++ object.
      auto *test_data = new test_data_t(TEST_DATA);

      // _Move_ the C++ object to the TEN message.
      new_cmd->set_property("test data", test_data);

      hello_world_cmd = std::move(cmd);

      ten_env.send_cmd(
          std::move(new_cmd), [this](ten::ten_env_t &ten_env,
                                     std::unique_ptr<ten::cmd_result_t> cmd) {
            auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
            cmd_result->set_property(
                "detail", cmd->get_property_string("detail").c_str());
            ten_env.return_result(std::move(cmd_result),
                                  std::move(hello_world_cmd));
          });
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
    if (std::string(cmd->get_name()) == "send_ptr") {
      // Get the TEN message property.
      auto *test_data =
          static_cast<test_data_t *>(cmd->get_property_ptr("test data"));
      TEN_ASSERT(*test_data->v_ == TEST_DATA, "Invalid argument.");

      delete test_data;

      auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
      cmd_result->set_property("detail", "hello world, too");
      ten_env.return_result(std::move(cmd_result), std::move(cmd));
    }
  }
};

class test_app : public ten::app_t {
 public:
  void on_init(ten::ten_env_t &ten_env) override {
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

    ten_env.on_init_done();
  }
};

void *test_app_thread_main(TEN_UNUSED void *arg) {
  auto *app = new test_app();
  app->run();
  delete app;

  return nullptr;
}

TEN_CPP_REGISTER_ADDON_AS_EXTENSION(msg_property_send_cpp_ptr__extension_1,
                                    test_extension_1);
TEN_CPP_REGISTER_ADDON_AS_EXTENSION(msg_property_send_cpp_ptr__extension_2,
                                    test_extension_2);

}  // namespace

TEST(ExtensionTest, MsgPropertySendCppPtr) {  // NOLINT
  // Start app.
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
               "type": "extension_group",
               "name": "msg_property_send_cpp_ptr__extension_group_1",
               "addon": "default_extension_group",
               "app": "msgpack://127.0.0.1:8001/"
             },{
               "type": "extension_group",
               "name": "msg_property_send_cpp_ptr__extension_group_2",
               "addon": "default_extension_group",
               "app": "msgpack://127.0.0.1:8001/"
             },{
               "type": "extension",
               "name": "msg_property_send_cpp_ptr__extension_1",
               "addon": "msg_property_send_cpp_ptr__extension_1",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "msg_property_send_cpp_ptr__extension_group_1"
             },{
               "type": "extension",
               "name": "msg_property_send_cpp_ptr__extension_2",
               "addon": "msg_property_send_cpp_ptr__extension_2",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "msg_property_send_cpp_ptr__extension_group_2"
             }],
             "connections": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "msg_property_send_cpp_ptr__extension_group_1",
               "extension": "msg_property_send_cpp_ptr__extension_1",
               "cmd": [{
                 "name": "send_ptr",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "msg_property_send_cpp_ptr__extension_group_2",
                   "extension": "msg_property_send_cpp_ptr__extension_2"
                 }]
               }]
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
             "dest": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "msg_property_send_cpp_ptr__extension_group_1",
               "extension": "msg_property_send_cpp_ptr__extension_1"
             }]
           }
         })"_json);
  ten_test::check_result_is(resp, "137", TEN_STATUS_CODE_OK,
                            "hello world, too");

  delete client;

  ten_thread_join(app_thread, -1);
}
