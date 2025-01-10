//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>

#include "gtest/gtest.h"
#include "include_internal/aptima_runtime/binding/cpp/aptima.h"
#include "aptima_utils/lang/cpp/lib/value.h"
#include "aptima_utils/lib/alloc.h"
#include "aptima_utils/lib/thread.h"
#include "aptima_utils/lib/time.h"
#include "aptima_utils/macro/check.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/aptima_runtime/smoke/util/binding/cpp/check.h"

#define TEST_DATA_VALUE 0x34CE87AB478D2DBE

namespace {

class test_extension_1 : public aptima::extension_t {
 public:
  explicit test_extension_1(const char *name)
      : aptima::extension_t(name), trigger(false) {}

  void outer_thread_main(aptima::aptima_env_proxy_t *aptima_env_proxy) {
    while (true) {
      if (trigger) {
        // Create a memory buffer to contain some important data.
        auto *test_data = static_cast<int64_t *>(aptima_MALLOC(sizeof(int64_t)));
        aptima_ASSERT(test_data, "Failed to allocate memory.");

        *test_data = TEST_DATA_VALUE;

        aptima_env_proxy->notify(send_data_from_outer_thread, test_data);

        break;
      }

      aptima_sleep(100);
    }

    delete aptima_env_proxy;

    // Wait stop notification.
    std::unique_lock<std::mutex> lock(outer_thread_cv_lock);
    while (!outer_thread_towards_to_close) {
      outer_thread_cv.wait(lock);
    }
  }

  void on_start(aptima::aptima_env_t &aptima_env) override {
    auto *aptima_env_proxy = aptima::aptima_env_proxy_t::create(aptima_env);

    // Create a C++ thread to call aptima.xxx in it.
    outer_thread = new std::thread(&test_extension_1::outer_thread_main, this,
                                   aptima_env_proxy);

    aptima_env.on_start_done();
  }

  void on_stop(aptima::aptima_env_t &aptima_env) override {
    {
      std::unique_lock<std::mutex> lock(outer_thread_cv_lock);
      outer_thread_towards_to_close = true;
    }
    outer_thread_cv.notify_one();

    // Reclaim the C++ thread.
    outer_thread->join();
    delete outer_thread;

    aptima_env.on_stop_done();
  }

  void on_cmd(aptima::aptima_env_t &aptima_env,
              std::unique_ptr<aptima::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      // Trigger the C++ thread to call aptima.xxx function.
      trigger = true;

      aptima_env.send_cmd(std::move(cmd));
      return;
    }
  }

 private:
  std::thread *outer_thread{nullptr};
  std::atomic<bool> trigger;
  std::mutex outer_thread_cv_lock;
  std::condition_variable outer_thread_cv;
  bool outer_thread_towards_to_close{false};

  static void send_data_from_outer_thread(aptima::aptima_env_t &aptima_env,
                                          void *user_data) {
    // Create a 'aptima::data_t' with the same important data.
    auto aptima_data = aptima::data_t::create("data");
    aptima_data->set_property("test_data", user_data);
    aptima_env.send_data(std::move(aptima_data));
  }
};

class test_extension_2 : public aptima::extension_t {
 public:
  explicit test_extension_2(const char *name) : aptima::extension_t(name) {}

  void on_cmd(aptima::aptima_env_t &aptima_env,
              std::unique_ptr<aptima::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      if (received_data) {
        auto cmd_result = aptima::cmd_result_t::create(aptima_STATUS_CODE_OK);
        cmd_result->set_property("detail", "hello world, too");
        aptima_env.return_result(std::move(cmd_result), std::move(cmd));
      } else {
        hello_world_cmd = std::move(cmd);
        return;
      }
    }
  }

  void on_data(aptima_UNUSED aptima::aptima_env_t &aptima_env,
               std::unique_ptr<aptima::data_t> data) override {
    // Wait 1 second to test if test_extension_2::on_cmd() is called directly
    // by test_extension_1::on_cmd(). If yes, the following checking would be
    // success, otherwise, 'test_data' would be freed by test_extension_1, so
    // that the following checking would be failed.
    aptima_sleep(1000);

    auto *test_data =
        static_cast<int64_t *>(data->get_property_ptr("test_data"));
    aptima_ASSERT(*test_data == TEST_DATA_VALUE, "test_data has been destroyed.");

    // Destroy the important data.
    aptima_FREE(test_data);

    received_data = true;
    if (hello_world_cmd != nullptr) {
      auto cmd_result = aptima::cmd_result_t::create(aptima_STATUS_CODE_OK);
      cmd_result->set_property("detail", "hello world, too");
      aptima_env.return_result(std::move(cmd_result), std::move(hello_world_cmd));
    }
  }

 private:
  std::unique_ptr<aptima::cmd_t> hello_world_cmd;
  bool received_data{false};
};

class test_app : public aptima::app_t {
 public:
  void on_configure(aptima::aptima_env_t &aptima_env) override {
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
    outer_thread_send_data_resp_handler__test_extension_1, test_extension_1);
aptima_CPP_REGISTER_ADDON_AS_EXTENSION(
    outer_thread_send_data_resp_handler__test_extension_2, test_extension_2);

}  // namespace

TEST(ExtensionTest, OuterThreadSendDataRespHandler) {  // NOLINT
  // Start app.
  auto *app_thread =
      aptima_thread_create("app thread", test_app_thread_main, nullptr);

  // Create a client and connect to the app.
  auto *client = new aptima::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

  // Send graph.
  auto start_graph_cmd = aptima::cmd_start_graph_t::create();
  start_graph_cmd->set_graph_from_json(R"({
           "nodes": [{
               "type": "extension",
               "name": "test_extension_1",
               "addon": "outer_thread_send_data_resp_handler__test_extension_1",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "basic_extension_group"
             },{
               "type": "extension",
               "name": "test_extension_2",
               "addon": "outer_thread_send_data_resp_handler__test_extension_2",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "basic_extension_group"
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
               }],
               "data": [{
                 "name": "data",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension": "test_extension_2"
                 }]
               }]
             }]
           })");
  auto cmd_result =
      client->send_cmd_and_recv_result(std::move(start_graph_cmd));
  aptima_test::check_status_code(cmd_result, aptima_STATUS_CODE_OK);

  // Send a user-defined 'hello world' command.
  auto hello_world_cmd = aptima::cmd_t::create("hello_world");
  hello_world_cmd->set_dest("msgpack://127.0.0.1:8001/", nullptr,
                            "basic_extension_group", "test_extension_1");
  cmd_result = client->send_cmd_and_recv_result(std::move(hello_world_cmd));
  aptima_test::check_status_code(cmd_result, aptima_STATUS_CODE_OK);
  aptima_test::check_detail_with_string(cmd_result, "hello world, too");

  delete client;

  aptima_thread_join(app_thread, -1);
}
