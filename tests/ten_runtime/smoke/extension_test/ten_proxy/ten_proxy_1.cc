//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <condition_variable>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>

#include "gtest/gtest.h"
#include "include_internal/ten_runtime/binding/cpp/ten.h"
#include "ten_utils/lang/cpp/lib/value.h"
#include "ten_utils/lib/thread.h"
#include "ten_utils/lib/time.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/ten_runtime/smoke/extension_test/util/binding/cpp/check.h"

#define TEST_DATA_VALUE 0x34CE87AB478D2DBE

namespace {

class test_extension_1 : public ten::extension_t {
 public:
  explicit test_extension_1(const std::string &name) : ten::extension_t(name) {}

  ~test_extension_1() override {
    delete test_data;

    // Reclaim the C++ thread.
    outer_thread->join();
    delete outer_thread;
  }

  void outer_thread_main(ten::ten_env_proxy_t *ten_env_proxy) {  // NOLINT
    // Wait stop notification.
    std::unique_lock<std::mutex> lock(outer_thread_cv_lock);
    while (!outer_thread_towards_to_close) {
      outer_thread_cv.wait(lock);
    }

    // Wait 2 seconds.
    TEN_LOGD("Waiting 2 seconds...");
    ten_sleep(2000);

    // Before deleting ten_env_proxy, this extension will not be removed, so
    // test_data can be accessed normally.
    EXPECT_EQ(*test_data, TEST_DATA_VALUE);

    // After this line, the whole TEN world will be closed.
    delete ten_env_proxy;
  }

  void on_start(ten::ten_env_t &ten_env) override {
    test_data = new int64_t(TEST_DATA_VALUE);

    // Create a ten_env_proxy, and this will prevent TEN runtime from closing.
    auto *ten_env_proxy = ten::ten_env_proxy_t::create(ten_env);

    // Create a C++ thread.
    outer_thread = new std::thread(&test_extension_1::outer_thread_main, this,
                                   ten_env_proxy);

    ten_env.on_start_done();
  }

  void on_stop(ten::ten_env_t &ten_env) override {
    // Notify the outer thread that we are about to close.
    {
      std::unique_lock<std::mutex> lock(outer_thread_cv_lock);
      outer_thread_towards_to_close = true;
    }
    outer_thread_cv.notify_one();

    ten_env.on_stop_done();
  }

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (std::string(cmd->get_name()) == "hello_world") {
      auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
      cmd_result->set_property("detail", "hello world, too");
      ten_env.return_result(std::move(cmd_result), std::move(cmd));
    }
  }

 private:
  std::thread *outer_thread{nullptr};
  int64_t *test_data{nullptr};

  std::mutex outer_thread_cv_lock;
  std::condition_variable outer_thread_cv;
  bool outer_thread_towards_to_close{false};
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

TEN_CPP_REGISTER_ADDON_AS_EXTENSION(ten_proxy_1__test_extension_1,
                                    test_extension_1);

}  // namespace

TEST(ExtensionTest, TenEnvProxy1) {  // NOLINT
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
               "name": "test_extension_1",
               "addon": "ten_proxy_1__test_extension_1",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "basic_extension_group"
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
               "extension": "test_extension_1"
             }]
           }
         })"_json);
  ten_test::check_result_is(resp, "137", TEN_STATUS_CODE_OK,
                            "hello world, too");

  delete client;

  ten_thread_join(app_thread, -1);
}
