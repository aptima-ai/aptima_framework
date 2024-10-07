//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

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
    const auto *cmd_name = cmd->get_name();

    if (std::string(cmd_name) == "hello_world") {
      // Save the command for later using.
      hello_world_cmd = std::move(cmd);

      ten_env.send_json(
          R"({
               "_ten": {
                 "name": "hello_world_1"
               }
             })"_json.dump()
              .c_str(),
          [this](ten::ten_env_t &ten_env,
                 std::unique_ptr<ten::cmd_result_t> cmd) {
            // Got result of 'hello world 1',
            // Now return result for 'hello world'
            auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
            cmd_result->set_property("detail", "hello world, too");
            ten_env.return_result(std::move(cmd_result),
                                  std::move(hello_world_cmd));
          });
    } else if (std::string(cmd_name) == "hello_world_2") {
      auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
      cmd_result->set_property("detail", "hello world, too");
      ten_env.return_result(std::move(cmd_result), std::move(cmd));
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
    if (std::string(cmd->get_name()) == "hello_world_1") {
      // waiting for result
      pending_request = std::move(cmd);

      ten_env.send_json(
          R"({
                         "_ten": {
                           "name": "hello_world_2"
                         }
                       })"_json.dump()
              .c_str(),
          [this](TEN_UNUSED ten::ten_env_t &ten_env,
                 TEN_UNUSED std::unique_ptr<ten::cmd_result_t> cmd) {
            // Got result of 'hello world 2'.
            // Now return result for 'hello world 1'
            auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
            cmd_result->set_property("detail", "hello world, too");
            ten_env.return_result(std::move(cmd_result),
                                  std::move(pending_request));
          });
    }
  }

 private:
  std::unique_ptr<ten::cmd_t> pending_request;
};

class test_extension_group : public ten::extension_group_t {
 public:
  explicit test_extension_group(const std::string &name)
      : ten::extension_group_t(name) {}

  void on_create_extensions(ten::ten_env_t &ten_env) override {
    std::vector<ten::extension_t *> extensions;
    extensions.push_back(new test_extension_1("test_extension_1"));
    extensions.push_back(new test_extension_2("test_extension_2"));
    ten_env.on_create_extensions_done(extensions);
  }

  void on_destroy_extensions(
      ten::ten_env_t &ten_env,
      const std::vector<ten::extension_t *> &extensions) override {
    for (auto *extension : extensions) {
      delete extension;
    }
    ten_env.on_destroy_extensions_done();
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

TEN_CPP_REGISTER_ADDON_AS_EXTENSION_GROUP(
    basic_loop_cmd_snapshot__extension_group, test_extension_group);

}  // namespace

TEST(ExtensionTest, BasicLoopCmdSnapShot) {  // NOLINT
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
               "name": "test_extension_group",
               "addon": "basic_loop_cmd_snapshot__extension_group",
               "app": "msgpack://127.0.0.1:8001/"
             }],
             "connections": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "test_extension_group",
               "extension": "test_extension_1",
               "cmd": [{
                 "name": "hello_world_1",
                 "dest": [{
                    "app": "msgpack://127.0.0.1:8001/",
                    "extension_group": "test_extension_group",
                    "extension": "test_extension_2"
                 }]
               }]
             },{
              "app": "msgpack://127.0.0.1:8001/",
              "extension_group": "test_extension_group",
              "extension": "test_extension_2",
              "cmd": [{
                 "name": "hello_world_2",
                 "dest": [{
                    "app": "msgpack://127.0.0.1:8001/",
                    "extension_group": "test_extension_group",
                    "extension": "test_extension_1"
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
             "dest":[{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "test_extension_group",
               "extension": "test_extension_1"
             }]
           }
         })"_json);
  ten_test::check_result_is(resp, "137", TEN_STATUS_CODE_OK,
                            "hello world, too");

  delete client;

  ten_thread_join(app_thread, -1);
}
