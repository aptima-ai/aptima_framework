//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <atomic>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>

#include "gtest/gtest.h"
#include "include_internal/aptima_runtime/binding/cpp/ten.h"
#include "aptima_utils/lang/cpp/lib/value.h"
#include "aptima_utils/lib/thread.h"
#include "aptima_utils/lib/time.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/aptima_runtime/smoke/util/binding/cpp/check.h"

namespace {

class test_extension;

void outer_thread_main(test_extension *ext,
                       ten::aptima_env_proxy_t *aptima_env_proxy);

class test_extension : public ten::extension_t {
 public:
  explicit test_extension(const char *name) : ten::extension_t(name) {}

  void on_start(ten::aptima_env_t &aptima_env) override {
    auto *aptima_env_proxy = ten::aptima_env_proxy_t::create(aptima_env);

    // Create a C++ thread to call ten.xxx in it.
    outer_thread = new std::thread(outer_thread_main, this, aptima_env_proxy);

    aptima_env.on_start_done();
  }

  void on_stop(ten::aptima_env_t &aptima_env) override {
    // Reclaim the C++ thread.
    outer_thread->join();
    delete outer_thread;

    aptima_env.on_stop_done();
  }

  void on_cmd(ten::aptima_env_t &aptima_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      // Trigger the C++ thread to call ten.xxx function.
      trigger = true;

      hello_world_cmd = std::move(cmd);
      return;
    }
  }

 private:
  friend void outer_thread_main(test_extension *ext,
                                ten::aptima_env_proxy_t *aptima_env_proxy);

  void extension_on_notify(ten::aptima_env_t &aptima_env) {
    auto cmd_result = ten::cmd_result_t::create(aptima_STATUS_CODE_OK);
    cmd_result->set_property("detail", "hello world, too");
    aptima_env.return_result(std::move(cmd_result), std::move(hello_world_cmd));
  }

  std::thread *outer_thread{nullptr};
  std::atomic<bool> trigger{false};
  std::unique_ptr<ten::cmd_t> hello_world_cmd;
};

void outer_thread_main(test_extension *ext,
                       ten::aptima_env_proxy_t *aptima_env_proxy) {
  while (true) {
    if (ext->trigger) {
      aptima_env_proxy->notify([ext](ten::aptima_env_t &aptima_env) {
        ext->extension_on_notify(aptima_env);
      });

      delete aptima_env_proxy;
      break;
    }

    aptima_sleep(100);
  }
}

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

aptima_CPP_REGISTER_ADDON_AS_EXTENSION(
    notify_test_member_func_basic_in_lambda_3__test_extension, test_extension);

}  // namespace

TEST(NotifyTest, MemberFuncInLambda3) {  // NOLINT
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
               "addon": "notify_test_member_func_basic_in_lambda_3__test_extension",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "basic_extension_group"
             }]
           })");
  auto cmd_result =
      client->send_cmd_and_recv_result(std::move(start_graph_cmd));
  aptima_test::check_status_code(cmd_result, aptima_STATUS_CODE_OK);

  // Send a user-defined 'hello world' command.
  auto hello_world_cmd = ten::cmd_t::create("hello_world");
  hello_world_cmd->set_dest("msgpack://127.0.0.1:8001/", nullptr,
                            "basic_extension_group", "test_extension");
  cmd_result = client->send_cmd_and_recv_result(std::move(hello_world_cmd));
  aptima_test::check_status_code(cmd_result, aptima_STATUS_CODE_OK);
  aptima_test::check_detail_with_string(cmd_result, "hello world, too");

  delete client;

  aptima_thread_join(app_thread, -1);
}
