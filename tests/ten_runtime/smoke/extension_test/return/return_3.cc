//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <nlohmann/json.hpp>
#include <string>
#include <thread>

#include "gtest/gtest.h"
#include "include_internal/ten_runtime/binding/cpp/ten.h"
#include "ten_utils/lib/thread.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/ten_runtime/smoke/extension_test/util/binding/cpp/check.h"

namespace {

class test_extension_1 : public ten::extension_t {
 public:
  explicit test_extension_1(const std::string &name) : ten::extension_t(name) {}

  void on_stop(ten::ten_env_t &ten_env) override {
    // Reclaim the C++ thread.
    outer_thread->join();
    delete outer_thread;

    ten_env.on_stop_done();
  }

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (std::string(cmd->get_name()) == "hello_world") {
      ten_env.send_cmd(
          std::move(cmd), [this](ten::ten_env_t &ten_env,
                                 std::unique_ptr<ten::cmd_result_t> status) {
            EXPECT_EQ(received_cmd_results_cnt, static_cast<size_t>(1));
            received_cmd_results_cnt--;

            auto *ten_env_proxy = ten::ten_env_proxy_t::create(ten_env);

            outer_thread =
                new std::thread(&test_extension_1::outer_thread_main, this,
                                status.release(), ten_env_proxy);
          });
      return;
    }
  }

 private:
  size_t received_cmd_results_cnt{1};
  std::thread *outer_thread{nullptr};

  void outer_thread_main(ten::cmd_result_t *cmd,
                         ten::ten_env_proxy_t *ten_env_proxy) {
    ten_env_proxy->notify(return_ok_from_outer_thread, cmd);
    delete ten_env_proxy;
  }

  static void return_ok_from_outer_thread(ten::ten_env_t &ten_env,
                                          void *user_data) {
    auto *cmd = static_cast<ten::cmd_result_t *>(user_data);

    cmd->set_property("detail", "return from extension 1");
    auto cmd_ptr = std::unique_ptr<ten::cmd_result_t>(cmd);
    ten_env.return_result_directly(std::move(cmd_ptr));
  }
};

class test_extension_2 : public ten::extension_t {
 public:
  explicit test_extension_2(const std::string &name) : ten::extension_t(name) {}

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (std::string(cmd->get_name()) == "hello_world") {
      auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
      cmd_result->set_property("detail", "hello world from extension 2");
      ten_env.return_result(std::move(cmd_result), std::move(cmd));
    }
  }
};

class test_extension_3 : public ten::extension_t {
 public:
  explicit test_extension_3(const std::string &name) : ten::extension_t(name) {}

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (std::string(cmd->get_name()) == "hello_world") {
      auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
      cmd_result->set_property("detail", "hello world from extension 3");
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

void *test_app_thread_main(TEN_UNUSED void *args) {
  auto *app = new test_app();
  app->run();
  delete app;

  return nullptr;
}

TEN_CPP_REGISTER_ADDON_AS_EXTENSION(return_3__extension_1, test_extension_1);
TEN_CPP_REGISTER_ADDON_AS_EXTENSION(return_3__extension_2, test_extension_2);
TEN_CPP_REGISTER_ADDON_AS_EXTENSION(return_3__extension_3, test_extension_3);

}  // namespace

TEST(ExtensionTest, Return3) {  // NOLINT
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
               "name": "test extension group 1",
               "addon": "default_extension_group",
               "app": "msgpack://127.0.0.1:8001/"
             },{
               "type": "extension",
               "name": "test extension 1",
               "addon": "return_3__extension_1",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "test extension group 1"
             },{
               "type": "extension",
               "name": "test extension 2",
               "addon": "return_3__extension_2",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "test extension group 1"
             },{
               "type": "extension_group",
               "name": "test extension group 2",
               "addon": "default_extension_group",
               "app": "msgpack://127.0.0.1:8001/"
             },{
               "type": "extension",
               "name": "test extension 3",
               "addon": "return_3__extension_3",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "test extension group 2"
             }],
             "connections": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "test extension group 1",
               "extension": "test extension 1",
               "cmd": [{
                 "name": "hello_world",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "test extension group 1",
                   "extension": "test extension 2"
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "test extension group 2",
                   "extension": "test extension 3"
                 }]
               }]
             }]
           }
         })"_json);
  ten_test::check_status_code_is(resp, TEN_STATUS_CODE_OK);

  // Send a user-defined 'hello world' command to 'extension 1'.
  resp = client->send_json_and_recv_resp_in_json(
      R"({
           "_ten": {
             "name": "hello_world",
             "seq_id": "137",
             "dest": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "test extension group 1",
               "extension": "test extension 1"
             }]
           }
         })"_json);

  ten_test::check_result_is(resp, "137", TEN_STATUS_CODE_OK,
                            "return from extension 1");

  delete client;

  ten_thread_join(app_thread, -1);
}
