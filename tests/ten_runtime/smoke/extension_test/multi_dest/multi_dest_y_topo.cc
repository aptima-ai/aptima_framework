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
    nlohmann::json json = nlohmann::json::parse(cmd->to_json());

    if (json["_ten"]["name"] == "hello_world") {
      // Remember the command sent from the client, so that we can send its
      // result back to the client.
      client_cmd = std::move(cmd);

      auto hello_world_cmd = ten::cmd_t::create("hello_world");
      ten_env.send_cmd(
          std::move(hello_world_cmd),
          [&](ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_result_t> /*cmd*/) {
            // Return to the client to notify that the whole process
            // is complete successfully.
            auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
            cmd_result->set_property("detail", "OK");
            ten_env.return_result(std::move(cmd_result), std::move(client_cmd));
          });

      return;
    }
  }

 private:
  std::unique_ptr<ten::cmd_t> client_cmd;
};

class test_extension_2 : public ten::extension_t {
 public:
  explicit test_extension_2(const std::string &name) : ten::extension_t(name) {}

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    // Extension 2 is just a forwarding proxy, forward all the commands it
    // received out.
    nlohmann::json json = nlohmann::json::parse(cmd->to_json());

    if (json["_ten"]["name"] == "hello_world") {
      ten_env.send_cmd(std::move(cmd));
      return;
    }
  }
};

class test_extension_3 : public ten::extension_t {
 public:
  explicit test_extension_3(const std::string &name) : ten::extension_t(name) {}

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    // Do not destroy the channel.
    auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
    cmd_result->set_property("detail", "hello world from extension 3, too");
    ten_env.return_result(std::move(cmd_result), std::move(cmd));
  }
};

class test_extension_4 : public ten::extension_t {
 public:
  explicit test_extension_4(const std::string &name) : ten::extension_t(name) {}

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    // Do not destroy the channel.
    auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
    cmd_result->set_property("detail", "hello world from extension 4, too");
    ten_env.return_result(std::move(cmd_result), std::move(cmd));
  }
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

TEN_CPP_REGISTER_ADDON_AS_EXTENSION(multi_dest_y_graph__extension_1,
                                    test_extension_1);
TEN_CPP_REGISTER_ADDON_AS_EXTENSION(multi_dest_y_graph__extension_2,
                                    test_extension_2);
TEN_CPP_REGISTER_ADDON_AS_EXTENSION(multi_dest_y_graph__extension_3,
                                    test_extension_3);
TEN_CPP_REGISTER_ADDON_AS_EXTENSION(multi_dest_y_graph__extension_4,
                                    test_extension_4);

}  // namespace

TEST(ExtensionTest, MultiDestYGraph) {  // NOLINT
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
               "type": "extension",
               "name": "extension_1",
               "addon": "multi_dest_y_graph__extension_1",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "extension_group"
             },{
               "type": "extension",
               "name": "extension_2",
               "addon": "multi_dest_y_graph__extension_2",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "extension_group"
             },{
               "type": "extension",
               "name": "extension_3",
               "addon": "multi_dest_y_graph__extension_3",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "extension_group"
             },{
               "type": "extension",
               "name": "extension_4",
               "addon": "multi_dest_y_graph__extension_4",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "extension_group"
             }],
             "connections": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "extension_group",
               "extension": "extension_1",
               "cmd": [{
                 "name": "hello_world",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "extension_group",
                   "extension": "extension_2"
                 }]
               }]
             },{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "extension_group",
               "extension": "extension_2",
               "cmd": [{
                 "name": "hello_world",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "extension_group",
                   "extension": "extension_3"
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "extension_group",
                   "extension": "extension_4"
                 }]
               }]
             }]
           }
         })"_json);
  ten_test::check_status_code_is(resp, TEN_STATUS_CODE_OK);

  resp = client->send_json_and_recv_resp_in_json(
      R"({
           "_ten": {
             "name": "hello_world",
             "seq_id": "137",
             "dest": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "extension_group",
               "extension": "extension_1"
             }]
           }
         })"_json);
  ten_test::check_result_is(resp, "137", TEN_STATUS_CODE_OK, "OK");

  delete client;

  ten_thread_join(app_thread, -1);
}
