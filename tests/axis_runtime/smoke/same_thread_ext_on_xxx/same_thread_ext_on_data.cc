//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <nlohmann/json.hpp>
#include <string>

#include "gtest/gtest.h"
#include "include_internal/aptima_runtime/binding/cpp/aptima.h"
#include "aptima_utils/lang/cpp/lib/value.h"
#include "aptima_utils/lib/alloc.h"
#include "aptima_utils/lib/thread.h"
#include "aptima_utils/lib/time.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/aptima_runtime/smoke/util/binding/cpp/check.h"

#define TEST_DATA_VALUE 0x34CE87AB478D2DBE

namespace {

class test_extension_1 : public aptima::extension_t {
 public:
  explicit test_extension_1(const char *name) : aptima::extension_t(name) {}

  void on_cmd(aptima::aptima_env_t &aptima_env,
              std::unique_ptr<aptima::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      // Create a memory buffer to contain some important data.
      auto *test_data = static_cast<int64_t *>(aptima_malloc(sizeof(int64_t)));
      aptima_ASSERT(test_data, "Failed to allocate memory.");

      *test_data = TEST_DATA_VALUE;

      // Same aptima_data with the important data.
      auto aptima_data = aptima::data_t::create("data");
      aptima_data->set_property("test_data", test_data);
      aptima_env.send_data(std::move(aptima_data));

      auto cmd_result = aptima::cmd_result_t::create(aptima_STATUS_CODE_OK);
      cmd_result->set_property("detail", "hello world, too");
      aptima_env.return_result(std::move(cmd_result), std::move(cmd));
    }
  }
};

class test_extension_2 : public aptima::extension_t {
 public:
  explicit test_extension_2(const char *name) : aptima::extension_t(name) {}

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

    // Destroy the important data. Because 'test_data' is used in
    // 'test_extension_2', we need to destroy 'test_data' here rather than in
    // 'test_extension_1'.
    aptima_free(test_data);
  }
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

aptima_CPP_REGISTER_ADDON_AS_EXTENSION(same_thread_ext_on_data__test_extension_1,
                                    test_extension_1);
aptima_CPP_REGISTER_ADDON_AS_EXTENSION(same_thread_ext_on_data__test_extension_2,
                                    test_extension_2);

}  // namespace

TEST(ExtensionTest, SameThreadExtOnData) {  // NOLINT
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
               "addon": "same_thread_ext_on_data__test_extension_1",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "basic_extension_group"
             },{
               "type": "extension",
               "name": "test_extension_2",
               "addon": "same_thread_ext_on_data__test_extension_2",
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
