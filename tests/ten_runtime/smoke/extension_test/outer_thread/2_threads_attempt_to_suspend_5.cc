//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>

#include "gtest/gtest.h"
#include "include_internal/ten_runtime/binding/cpp/ten.h"
#include "ten_utils/lang/cpp/lib/value.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/thread.h"
#include "ten_utils/macro/check.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/ten_runtime/smoke/extension_test/util/binding/cpp/check.h"

#define TEST_DATA_VALUE 0x34CE87AB478D2DBE

namespace {

class Holder {  // NOLINT
 public:
  explicit Holder(void *ptr) : ptr_(ptr) {}
  ~Holder() { TEN_FREE(ptr_); }

 private:
  void *ptr_{nullptr};
};

class test_extension_1 : public ten::extension_t {
 public:
  explicit test_extension_1(const std::string &name) : ten::extension_t(name) {}

  void outer_thread1_main(ten::ten_env_proxy_t *ten_env_proxy) {  // NOLINT
    // Create a memory buffer to contain some important data.
    auto *test_data = static_cast<int64_t *>(ten_malloc(sizeof(int64_t)));
    TEN_ASSERT(test_data, "Failed to allocate memory.");

    // We want to simulate that the memory will be freed when it goes out of the
    // scope.
    Holder _(test_data);

    *test_data = TEST_DATA_VALUE;

    std::unique_lock<std::mutex> lock(outer_thread_1_cv_lock);

    for (int i = 0; i < 10; ++i) {
      if (!outer_thread_1_towards_to_close) {
        bool rc = ten_env_proxy->acquire_lock_mode();
        TEN_ASSERT(rc, "Failed to acquire lock mode.");

        ten_env_proxy->notify(send_data_from_outer_thread, test_data, true);

        rc = ten_env_proxy->release_lock_mode();
        TEN_ASSERT(rc, "Failed to release lock mode.");
      }
    }

    delete ten_env_proxy;

    while (!outer_thread_1_towards_to_close) {
      outer_thread_1_cv.wait(lock);
    }
  }

  void outer_thread2_main(ten::ten_env_proxy_t *ten_env_proxy) {  // NOLINT
    // Create a memory buffer to contain some important data.
    auto *test_data = static_cast<int64_t *>(TEN_MALLOC(sizeof(int64_t)));
    TEN_ASSERT(test_data, "Failed to allocate memory.");

    // We want to simulate that the memory will be freed when it goes out of the
    // scope.
    Holder _(test_data);

    *test_data = TEST_DATA_VALUE;

    std::unique_lock<std::mutex> lock(outer_thread_2_cv_lock);
    for (int i = 0; i < 10; ++i) {
      if (!outer_thread_2_towards_to_close) {
        bool rc = ten_env_proxy->acquire_lock_mode();
        TEN_ASSERT(rc, "Failed to acquire lock mode.");

        ten_env_proxy->notify(send_data_from_outer_thread, test_data, true);

        rc = ten_env_proxy->release_lock_mode();
        TEN_ASSERT(rc, "Failed to release lock mode.");
      }
    }

    delete ten_env_proxy;

    while (!outer_thread_2_towards_to_close) {
      outer_thread_2_cv.wait(lock);
    }
  }

  void on_start(ten::ten_env_t &ten_env) override {
    auto start_to_send_cmd = ten::cmd_t::create("start_to_send");
    ten_env.send_cmd(
        std::move(start_to_send_cmd),
        [this](ten::ten_env_t &ten_env,
               std::unique_ptr<ten::cmd_result_t> cmd_result) {
          TEN_ASSERT(cmd_result->get_status_code() == TEN_STATUS_CODE_OK,
                     "Failed to send 'start_to_send' command.");

          auto *ten_proxy_1 = ten::ten_env_proxy_t::create(ten_env);
          auto *ten_proxy_2 = ten::ten_env_proxy_t::create(ten_env);

          // Create a C++ thread to call ten.xxx in it.
          outer_thread1 = new std::thread(&test_extension_1::outer_thread1_main,
                                          this, ten_proxy_1);
          outer_thread2 = new std::thread(&test_extension_1::outer_thread2_main,
                                          this, ten_proxy_2);

          return true;
        });

    ten_env.on_start_done();
  }

  void on_stop(ten::ten_env_t &ten_env) override {
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

    ten_env.on_stop_done();
  }

  void on_cmd(ten::ten_env_t &ten_env,
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

  static void send_data_from_outer_thread(ten::ten_env_t &ten_env,
                                          void *user_data) {
    // Create a 'ten::data_t' with the same important data.
    auto ten_data = ten::data_t::create("data");
    ten_data->set_property("test_data", user_data);
    ten_env.send_data(std::move(ten_data));
  }
};

class test_extension_2 : public ten::extension_t {
 public:
  explicit test_extension_2(const std::string &name) : ten::extension_t(name) {}

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (cmd->get_name() == std::string("start_to_send")) {
      auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
      cmd_result->set_property("detail", "ok");
      ten_env.return_result(std::move(cmd_result), std::move(cmd));
      return;
    }

    if (data_received_count == expected_received_count) {
      auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
      cmd_result->set_property("detail", "ok");
      ten_env.return_result(std::move(cmd_result), std::move(cmd));
    } else {
      hello_cmd = std::move(cmd);
    }
  }

  void on_data(TEN_UNUSED ten::ten_env_t &ten_env,
               std::unique_ptr<ten::data_t> data) override {
    auto *test_data =
        static_cast<int64_t *>(data->get_property_ptr("test_data"));
    TEN_ASSERT(*test_data == TEST_DATA_VALUE, "test_data has been destroyed.");

    data_received_count++;

    if (hello_cmd != nullptr &&
        data_received_count == expected_received_count) {
      auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
      cmd_result->set_property("detail", "ok");
      ten_env.return_result(std::move(cmd_result), std::move(hello_cmd));
    }
  }

 private:
  std::unique_ptr<ten::cmd_t> hello_cmd{nullptr};
  int data_received_count{0};
  int expected_received_count{20};
};

class test_app : public ten::app_t {
 public:
  void on_init(ten::ten_env_t &ten_env) override {
    bool rc = ten_env.init_property_from_json(
        // clang-format off
                 R"({
                      "_ten": {
                        "uri": "msgpack://127.0.0.1:8001/",
                        "log_level": 1
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

TEN_CPP_REGISTER_ADDON_AS_EXTENSION(
    two_threads_attempt_to_suspend_5__test_extension_1, test_extension_1);
TEN_CPP_REGISTER_ADDON_AS_EXTENSION(
    two_threads_attempt_to_suspend_5__test_extension_2, test_extension_2);

}  // namespace

TEST(ExtensionTest, TwoThreadsAttemptToSuspend5) {  // NOLINT
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
               "name": "basic_extension_group",
               "addon": "default_extension_group",
               "app": "msgpack://127.0.0.1:8001/"
             },{
               "type": "extension",
               "name": "test extension 1",
               "addon": "two_threads_attempt_to_suspend_5__test_extension_1",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "basic_extension_group"
             },{
               "type": "extension",
               "name": "test extension 2",
               "addon": "two_threads_attempt_to_suspend_5__test_extension_2",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "basic_extension_group"
             }],
             "connections": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "basic_extension_group",
               "extension": "test extension 1",
               "cmd": [{
                 "name": "start_to_send",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "basic_extension_group",
                   "extension": "test extension 2"
                 }]
               }],
               "data": [{
                 "name": "data",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "basic_extension_group",
                   "extension": "test extension 2"
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
               "extension_group": "basic_extension_group",
               "extension": "test extension 2"
             }]
           }
         })"_json);
  ten_test::check_result_is(resp, "137", TEN_STATUS_CODE_OK, "ok");

  delete client;

  ten_thread_join(app_thread, -1);
}
