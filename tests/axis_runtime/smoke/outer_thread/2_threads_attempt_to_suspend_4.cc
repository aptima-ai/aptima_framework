//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>

#include "gtest/gtest.h"
#include "include_internal/aptima_runtime/binding/cpp/ten.h"
#include "aptima_utils/lang/cpp/lib/value.h"
#include "aptima_utils/lib/alloc.h"
#include "aptima_utils/lib/thread.h"
#include "aptima_utils/macro/check.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/aptima_runtime/smoke/util/binding/cpp/check.h"

#define TEST_DATA_VALUE 0x34CE87AB478D2DBE

namespace {

class Holder {  // NOLINT
 public:
  explicit Holder(void *ptr) : ptr_(ptr) {}
  ~Holder() { aptima_FREE(ptr_); }

 private:
  void *ptr_{nullptr};
};

class test_extension_1 : public ten::extension_t {
 public:
  explicit test_extension_1(const char *name) : ten::extension_t(name) {}

  void outer_thread1_main(ten::aptima_env_proxy_t *aptima_env_proxy) {  // NOLINT
    // Create a memory buffer to contain some important data.
    auto *test_data = static_cast<int64_t *>(aptima_malloc(sizeof(int64_t)));
    aptima_ASSERT(test_data, "Failed to allocate memory.");

    // We want to simulate that the memory will be freed when it goes out of the
    // scope.
    Holder _(test_data);

    *test_data = TEST_DATA_VALUE;

    std::unique_lock<std::mutex> lock(outer_thread_1_cv_lock);
    if (!outer_thread_1_towards_to_close) {
      bool rc = aptima_env_proxy->acquire_lock_mode();
      aptima_ASSERT(rc, "Failed to acquire lock mode.");

      aptima_env_proxy->notify(send_data_from_outer_thread, test_data, true);

      rc = aptima_env_proxy->release_lock_mode();
      aptima_ASSERT(rc, "Failed to release lock mode.");
    }

    delete aptima_env_proxy;

    while (!outer_thread_1_towards_to_close) {
      outer_thread_1_cv.wait(lock);
    }
  }

  void outer_thread2_main(ten::aptima_env_proxy_t *aptima_env_proxy) {  // NOLINT
    // Create a memory buffer to contain some important data.
    auto *test_data = static_cast<int64_t *>(aptima_MALLOC(sizeof(int64_t)));
    aptima_ASSERT(test_data, "Failed to allocate memory.");

    // We want to simulate that the memory will be freed when it goes out of the
    // scope.
    Holder _(test_data);

    *test_data = TEST_DATA_VALUE;

    std::unique_lock<std::mutex> lock(outer_thread_2_cv_lock);
    if (!outer_thread_2_towards_to_close) {
      bool rc = aptima_env_proxy->acquire_lock_mode();
      aptima_ASSERT(rc, "Failed to acquire lock mode.");

      aptima_env_proxy->notify(send_data_from_outer_thread, test_data, true);

      rc = aptima_env_proxy->release_lock_mode();
      aptima_ASSERT(rc, "Failed to release lock mode.");
    }

    delete aptima_env_proxy;

    while (!outer_thread_2_towards_to_close) {
      outer_thread_2_cv.wait(lock);
    }
  }

  void on_start(ten::aptima_env_t &aptima_env) override {
    auto start_to_send_cmd = ten::cmd_t::create("start_to_send");
    aptima_env.send_cmd(
        std::move(start_to_send_cmd),
        [this](ten::aptima_env_t &aptima_env,
               std::unique_ptr<ten::cmd_result_t> cmd_result,
               ten::error_t *err) {
          aptima_ASSERT(cmd_result->get_status_code() == aptima_STATUS_CODE_OK,
                     "Failed to send 'start_to_send' command.");

          auto *aptima_proxy_1 = ten::aptima_env_proxy_t::create(aptima_env);
          auto *aptima_proxy_2 = ten::aptima_env_proxy_t::create(aptima_env);

          // Create a C++ thread to call ten.xxx in it.
          outer_thread1 = new std::thread(&test_extension_1::outer_thread1_main,
                                          this, aptima_proxy_1);
          outer_thread2 = new std::thread(&test_extension_1::outer_thread2_main,
                                          this, aptima_proxy_2);

          return true;
        });

    aptima_env.on_start_done();
  }

  void on_stop(ten::aptima_env_t &aptima_env) override {
    {
      std::unique_lock<std::mutex> lock(outer_thread_1_cv_lock);
      outer_thread_1_towards_to_close = true;
    }
    outer_thread_1_cv.notify_one();

    {
      std::unique_lock<std::mutex> lock(outer_thread_2_cv_lock);
      outer_thread_2_towards_to_close = true;
    }
    outer_thread_2_cv.notify_one();

    // Reclaim the C++ thread.
    outer_thread1->join();
    delete outer_thread1;

    outer_thread2->join();
    delete outer_thread2;

    aptima_env.on_stop_done();
  }

  void on_cmd(ten::aptima_env_t &aptima_env,
              std::unique_ptr<ten::cmd_t> cmd) override {}

 private:
  std::thread *outer_thread1{nullptr};
  std::thread *outer_thread2{nullptr};

  std::mutex outer_thread_1_cv_lock;
  std::condition_variable outer_thread_1_cv;
  bool outer_thread_1_towards_to_close{false};

  std::mutex outer_thread_2_cv_lock;
  std::condition_variable outer_thread_2_cv;
  bool outer_thread_2_towards_to_close{false};

  static void send_data_from_outer_thread(ten::aptima_env_t &aptima_env,
                                          void *user_data) {
    // Create a 'ten::data_t' with the same important data.
    auto aptima_data = ten::data_t::create("data");
    aptima_data->set_property("test_data", user_data);
    aptima_env.send_data(std::move(aptima_data));
  }
};

class test_extension_2 : public ten::extension_t {
 public:
  explicit test_extension_2(const char *name) : ten::extension_t(name) {}

  void on_cmd(ten::aptima_env_t &aptima_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (cmd->get_name() == std::string("start_to_send")) {
      auto cmd_result = ten::cmd_result_t::create(aptima_STATUS_CODE_OK);
      cmd_result->set_property("detail", "ok");
      aptima_env.return_result(std::move(cmd_result), std::move(cmd));
      return;
    }

    if (data_received_count == expected_received_count) {
      auto cmd_result = ten::cmd_result_t::create(aptima_STATUS_CODE_OK);
      cmd_result->set_property("detail", "ok");
      aptima_env.return_result(std::move(cmd_result), std::move(cmd));
    } else {
      hello_cmd = std::move(cmd);
    }
  }

  void on_data(aptima_UNUSED ten::aptima_env_t &aptima_env,
               std::unique_ptr<ten::data_t> data) override {
    auto *test_data =
        static_cast<int64_t *>(data->get_property_ptr("test_data"));
    aptima_ASSERT(*test_data == TEST_DATA_VALUE, "test_data has been destroyed.");

    data_received_count++;

    if (hello_cmd != nullptr &&
        data_received_count == expected_received_count) {
      auto cmd_result = ten::cmd_result_t::create(aptima_STATUS_CODE_OK);
      cmd_result->set_property("detail", "ok");
      aptima_env.return_result(std::move(cmd_result), std::move(hello_cmd));
    }
  }

 private:
  std::unique_ptr<ten::cmd_t> hello_cmd{nullptr};
  int data_received_count{0};
  int expected_received_count{2};
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

aptima_CPP_REGISTER_ADDON_AS_EXTENSION(
    two_threads_attempt_to_suspend_4__test_extension_1, test_extension_1);
aptima_CPP_REGISTER_ADDON_AS_EXTENSION(
    two_threads_attempt_to_suspend_4__test_extension_2, test_extension_2);

}  // namespace

TEST(ExtensionTest, TwoThreadsAttemptToSuspend4) {  // NOLINT
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
               "addon": "two_threads_attempt_to_suspend_4__test_extension_1",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "basic_extension_group"
             },{
               "type": "extension",
               "name": "test_extension_2",
               "addon": "two_threads_attempt_to_suspend_4__test_extension_2",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "basic_extension_group"
             }],
             "connections": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension": "test_extension_1",
               "cmd": [{
                 "name": "start_to_send",
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
  auto hello_world_cmd = ten::cmd_t::create("hello_world");
  hello_world_cmd->set_dest("msgpack://127.0.0.1:8001/", nullptr,
                            "basic_extension_group", "test_extension_2");
  cmd_result = client->send_cmd_and_recv_result(std::move(hello_world_cmd));
  aptima_test::check_status_code(cmd_result, aptima_STATUS_CODE_OK);
  aptima_test::check_detail_with_string(cmd_result, "ok");

  delete client;

  aptima_thread_join(app_thread, -1);
}
