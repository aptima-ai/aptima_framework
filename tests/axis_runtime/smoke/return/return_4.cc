//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <nlohmann/json.hpp>
#include <string>
#include <thread>

#include "gtest/gtest.h"
#include "include_internal/aptima_runtime/binding/cpp/ten.h"
#include "aptima_utils/lib/thread.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/aptima_runtime/smoke/util/binding/cpp/check.h"

namespace {

class test_extension_1 : public ten::extension_t {
 public:
  explicit test_extension_1(const char *name) : ten::extension_t(name) {}

  void on_stop(ten::aptima_env_t &aptima_env) override {
    // Reclaim the C++ thread.
    outer_thread->join();
    delete outer_thread;

    aptima_env.on_stop_done();
  }

  void on_cmd(ten::aptima_env_t &aptima_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      aptima_env.send_cmd(
          std::move(cmd),
          [this](ten::aptima_env_t &aptima_env,
                 std::unique_ptr<ten::cmd_result_t> result, ten::error_t *err) {
            EXPECT_EQ(received_cmd_results_cnt, static_cast<size_t>(1));
            received_cmd_results_cnt--;

            auto *aptima_env_proxy = ten::aptima_env_proxy_t::create(aptima_env);

            outer_thread =
                new std::thread(&test_extension_1::outer_thread_main, this,
                                result.release(), aptima_env_proxy);
          });
      return;
    }
  }

 private:
  size_t received_cmd_results_cnt{1};
  std::thread *outer_thread{nullptr};

  void outer_thread_main(ten::cmd_result_t *cmd,
                         ten::aptima_env_proxy_t *aptima_env_proxy) {
    aptima_env_proxy->notify(return_ok_from_outer_thread, cmd);
    delete aptima_env_proxy;
  }

  static void return_ok_from_outer_thread(ten::aptima_env_t &aptima_env,
                                          void *user_data) {
    auto *cmd = static_cast<ten::cmd_result_t *>(user_data);

    cmd->set_property("detail", "return from extension 1");
    auto cmd_ptr = std::unique_ptr<ten::cmd_result_t>(cmd);
    aptima_env.return_result_directly(std::move(cmd_ptr));
  }
};

class test_extension_2 : public ten::extension_t {
 public:
  explicit test_extension_2(const char *name) : ten::extension_t(name) {}

  void on_cmd(ten::aptima_env_t &aptima_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      auto cmd_result = ten::cmd_result_t::create(aptima_STATUS_CODE_OK);
      cmd_result->set_property("detail", "hello world from extension 2");
      aptima_env.return_result(std::move(cmd_result), std::move(cmd));
    }
  }
};

class test_extension_3 : public ten::extension_t {
 public:
  explicit test_extension_3(const char *name) : ten::extension_t(name) {}

  void on_cmd(ten::aptima_env_t &aptima_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      auto cmd_result = ten::cmd_result_t::create(aptima_STATUS_CODE_OK);
      cmd_result->set_property("detail", "hello world from extension 3");
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

aptima_CPP_REGISTER_ADDON_AS_EXTENSION(return_4__extension_1, test_extension_1);
aptima_CPP_REGISTER_ADDON_AS_EXTENSION(return_4__extension_2, test_extension_2);
aptima_CPP_REGISTER_ADDON_AS_EXTENSION(return_4__extension_3, test_extension_3);

}  // namespace

TEST(ExtensionTest, Return4) {  // NOLINT
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
               "addon": "return_4__extension_1",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "test_extension_group"
             },{
               "type": "extension",
               "name": "test_extension_2",
               "addon": "return_4__extension_2",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "test_extension_group"
             },{
               "type": "extension",
               "name": "test_extension_3",
               "addon": "return_4__extension_3",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "test_extension_group"
             }],
             "connections": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension": "test_extension_1",
               "cmd": [{
                 "name": "hello_world",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension": "test_extension_2"
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension": "test_extension_3"
                 }]
               }]
             }]
           })");
  auto cmd_result =
      client->send_cmd_and_recv_result(std::move(start_graph_cmd));
  aptima_test::check_status_code(cmd_result, aptima_STATUS_CODE_OK);

  // Send a user-defined 'hello world' command to 'extension 1'.
  auto hello_world_cmd = ten::cmd_t::create("hello_world");
  hello_world_cmd->set_dest("msgpack://127.0.0.1:8001/", nullptr,
                            "test_extension_group", "test_extension_1");
  cmd_result = client->send_cmd_and_recv_result(std::move(hello_world_cmd));

  aptima_test::check_status_code(cmd_result, aptima_STATUS_CODE_OK);
  aptima_test::check_detail_with_string(cmd_result, "return from extension 1");

  delete client;

  aptima_thread_join(app_thread, -1);
}
