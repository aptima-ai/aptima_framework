//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <nlohmann/json.hpp>
#include <string>

#include "gtest/gtest.h"
#include "include_internal/aptima_runtime/binding/cpp/ten.h"
#include "aptima_utils/lib/thread.h"
#include "aptima_utils/lib/time.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/aptima_runtime/smoke/util/binding/cpp/check.h"

namespace {

class test_extension_1 : public ten::extension_t {
 public:
  explicit test_extension_1(const char *name) : ten::extension_t(name) {}

  void on_cmd(ten::aptima_env_t &aptima_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      aptima_env.send_cmd(std::move(cmd));
      return;
    }
  }
};

class test_extension_2 : public ten::extension_t {
 public:
  explicit test_extension_2(const char *name) : ten::extension_t(name) {}

  void on_cmd(ten::aptima_env_t &aptima_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      auto cmd_result = ten::cmd_result_t::create(aptima_STATUS_CODE_OK);
      cmd_result->set_property("detail", "hello world, too");
      aptima_env.return_result(std::move(cmd_result), std::move(cmd));
    }
  }
};

class test_app_1 : public ten::app_t {
 public:
  void on_configure(ten::aptima_env_t &aptima_env) override {
    bool rc = ten::aptima_env_internal_accessor_t::init_manifest_from_json(
        aptima_env,
        // clang-format off
                 R"({
                      "type": "app",
                      "name": "test_app",
                      "version": "0.1.0"
                    })"
        // clang-format on
    );
    ASSERT_EQ(rc, true);

    rc = aptima_env.init_property_from_json(
        // clang-format off
                 R"({
                      "_ten": {
                        "uri": "msgpack://127.0.0.1:8001/",
                        "log_level": 2,
                        "predefined_graphs": [{
                           "name": "default",
                           "auto_start": true,
                           "singleton": true,
                           "nodes": [{
                              "type": "extension",
                              "app": "msgpack://127.0.0.1:8001/",
                              "extension_group": "predefined_graph_group",
                              "addon": "predefined_graph_multi_app__extension_1",
                              "name": "test_extension_1"
                           }, {
                              "type": "extension",
                              "app": "msgpack://127.0.0.1:8002/",
                              "extension_group": "predefined_graph_group",
                              "addon": "predefined_graph_multi_app__extension_2",
                              "name": "test_extension_2"
                           }],
                           "connections": [{
                             "app": "msgpack://127.0.0.1:8001/",
                             "extension": "test_extension_1",
                             "cmd": [{
                               "name": "hello_world",
                               "dest": [{
                                 "app": "msgpack://127.0.0.1:8002/",
                                 "extension": "test_extension_2"
                               }]
                             }]
                           }]
                         }]
                       }
                     })"
        // clang-format on
    );
    ASSERT_EQ(rc, true);

    aptima_env.on_configure_done();
  }
};

class test_app_2 : public ten::app_t {
 public:
  void on_configure(ten::aptima_env_t &aptima_env) override {
    aptima_env.init_property_from_json(
        R"({
             "_ten": {
               "uri": "msgpack://127.0.0.1:8002/"
             }
           })");
    aptima_env.on_configure_done();
  }
};

void *app_thread_1_main(aptima_UNUSED void *args) {
  auto *app = new test_app_1();
  app->run();
  delete app;

  return nullptr;
}

void *app_thread_2_main(aptima_UNUSED void *args) {
  auto *app = new test_app_2();
  app->run();
  delete app;

  return nullptr;
}

aptima_CPP_REGISTER_ADDON_AS_EXTENSION(predefined_graph_multi_app__extension_1,
                                    test_extension_1);
aptima_CPP_REGISTER_ADDON_AS_EXTENSION(predefined_graph_multi_app__extension_2,
                                    test_extension_2);

}  // namespace

TEST(ExtensionTest, PredefinedGraphMultiApp) {  // NOLINT
  // Start app.
  auto *app_1_thread =
      aptima_thread_create("app thread 1", app_thread_1_main, nullptr);

  // Used to verify the retry mechanism of the protocol.
  aptima_sleep(1000);
  auto *app_2_thread =
      aptima_thread_create("app thread 2", app_thread_2_main, nullptr);

  // Create a client and connect to the app.
  auto *client = new ten::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

  // Send a user-defined 'hello world' command.
  auto hello_world_cmd = ten::cmd_t::create("hello_world");
  hello_world_cmd->set_dest("msgpack://127.0.0.1:8001/", "default",
                            "predefined_graph_group", "test_extension_1");
  auto cmd_result =
      client->send_cmd_and_recv_result(std::move(hello_world_cmd));
  aptima_test::check_status_code(cmd_result, aptima_STATUS_CODE_OK);
  aptima_test::check_detail_with_string(cmd_result, "hello world, too");

  delete client;

  aptima_thread_join(app_1_thread, -1);
  aptima_thread_join(app_2_thread, -1);
}
