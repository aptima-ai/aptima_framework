//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "include_internal/ten_runtime/binding/cpp/aptima.h"
#include "ten_utils/lib/thread.h"
#include "ten_utils/lib/time.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/common/constant.h"
#include "tests/ten_runtime/smoke/util/binding/cpp/check.h"

namespace {

class test_extension_1 : public aptima::extension_t {
 public:
  explicit test_extension_1(const char *name) : aptima::extension_t(name) {}

  void on_cmd(aptima::ten_env_t &ten_env,
              std::unique_ptr<aptima::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      ten_env.send_cmd(std::move(cmd));
      return;
    }
  }
};

class test_extension_2 : public aptima::extension_t {
 public:
  explicit test_extension_2(const char *name) : aptima::extension_t(name) {}

  void on_cmd(aptima::ten_env_t &ten_env,
              std::unique_ptr<aptima::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      auto cmd_result = aptima::cmd_result_t::create(TEN_STATUS_CODE_OK);
      cmd_result->set_property("detail", "hello world, too");
      ten_env.return_result(std::move(cmd_result), std::move(cmd));
    }
  }
};

class test_app_1 : public aptima::app_t {
 public:
  void on_configure(aptima::ten_env_t &ten_env) override {
    // In a scenario which contains multiple APTIMA app, the construction of a
    // graph might failed because not all APTIMA app has already been launched
    // successfully.
    //
    //     client -> (connect cmd) -> APTIMA app 1 ... APTIMA app 2
    //                                    o             x
    //
    // In this case, the newly constructed engine in the app 1 would be closed,
    // and the client would see that the connection has be dropped. After that,
    // the client could retry to send the 'start_graph' command again to inform
    // the APTIMA app to build the graph again.
    //
    // Therefore, the closing of an engine could _not_ cause the closing of the
    // app, and that's why the following 'long_running_mode' has been set.
    bool rc = ten_env.init_property_from_json(
        // clang-format off
                 R"({
                      "_ten": {
                        "uri": "msgpack://127.0.0.1:8001/",
                        "long_running_mode": true,
                        "log_level": 2
                      }
                    })"
        // clang-format on
    );
    ASSERT_EQ(rc, true);

    ten_env.on_configure_done();
  }
};

class test_app_2 : public aptima::app_t {
 public:
  void on_configure(aptima::ten_env_t &ten_env) override {
    bool rc = ten_env.init_property_from_json(
        // clang-format off
                 R"({
                      "_ten": {
                        "uri": "msgpack://127.0.0.1:8002/",
                        "long_running_mode": true,
                        "log_level": 2
                      }
                    })"
        // clang-format on
    );
    ASSERT_EQ(rc, true);

    ten_env.on_configure_done();
  }
};

void *app_thread_1_main(TEN_UNUSED void *args) {
  auto *app = new test_app_1();
  app->run();
  delete app;

  return nullptr;
}

void *app_thread_2_main(TEN_UNUSED void *args) {
  auto *app = new test_app_2();
  app->run();
  delete app;

  return nullptr;
}

TEN_CPP_REGISTER_ADDON_AS_EXTENSION(multi_app_concurrent__extension_1,
                                    test_extension_1);
TEN_CPP_REGISTER_ADDON_AS_EXTENSION(multi_app_concurrent__extension_2,
                                    test_extension_2);

void *client_thread_main(TEN_UNUSED void *args) {
  aptima::msgpack_tcp_client_t *client = nullptr;

  // In a scenario which contains multiple APTIMA app, the construction of a
  // graph might failed because not all APTIMA app has already been launched
  // successfully.
  //
  //     client -> (connect cmd) -> APTIMA app 1 ... APTIMA app 2
  //                                    o             x
  //
  // In this case, the newly constructed engine in the app 1 would be closed,
  // and the client would see that the connection has be dropped. After that,
  // the client could retry to send the 'start_graph' command again to inform
  // the APTIMA app to build the graph again.
  for (size_t i = 0; i < MULTIPLE_APP_SCENARIO_GRAPH_CONSTRUCTION_RETRY_TIMES;
       ++i) {
    // Create a client and connect to the app.
    client = new aptima::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

    // Send graph.
    auto start_graph_cmd = aptima::cmd_start_graph_t::create();
    start_graph_cmd->set_graph_from_json(R"({
           "nodes": [{
                 "type": "extension",
                 "name": "test_extension_1",
                 "addon": "multi_app_concurrent__extension_1",
                 "app": "msgpack://127.0.0.1:8001/",
                 "extension_group": "test_extension_group 1"
               },{
                 "type": "extension",
                 "name": "test_extension_2",
                 "addon": "multi_app_concurrent__extension_2",
                 "app": "msgpack://127.0.0.1:8002/",
                 "extension_group": "test_extension_group 2"
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
             })");
    auto cmd_result =
        client->send_cmd_and_recv_result(std::move(start_graph_cmd));

    if (cmd_result) {
      ten_test::check_status_code(cmd_result, TEN_STATUS_CODE_OK);
      break;
    } else {
      delete client;
      client = nullptr;

      // To prevent from busy re-trying.
      ten_sleep(10);
    }
  }

  TEN_ASSERT(client, "Failed to connect to the APTIMA app.");

  // Send a user-defined 'hello world' command.
  auto hello_world_cmd = aptima::cmd_t::create("hello_world");
  hello_world_cmd->set_dest("msgpack://127.0.0.1:8001/", nullptr,
                            "test_extension_group 1", "test_extension_1");

  auto cmd_result =
      client->send_cmd_and_recv_result(std::move(hello_world_cmd));

  ten_test::check_status_code(cmd_result, TEN_STATUS_CODE_OK);
  ten_test::check_detail_with_string(cmd_result, "hello world, too");

  delete client;

  return nullptr;
}

}  // namespace

TEST(ExtensionTest, DISABLED_MultiAppConcurrent) {  // NOLINT
  // Start app.
  auto *app_thread_2 =
      ten_thread_create("app thread 2", app_thread_2_main, nullptr);
  auto *app_thread_1 =
      ten_thread_create("app thread 1", app_thread_1_main, nullptr);

  std::vector<ten_thread_t *> client_threads;

  for (size_t i = 0; i < ONE_ENGINE_ONE_CLIENT_CONCURRENT_CNT; ++i) {
    auto *client_thread =
        ten_thread_create("client_thread_main", client_thread_main, nullptr);

    client_threads.push_back(client_thread);
  }

  for (ten_thread_t *client_thread : client_threads) {
    ten_thread_join(client_thread, -1);
  }

  // Because the closing of an engine would _not_ cause the closing of the app,
  // so we have to explicitly close the app.
  aptima::msgpack_tcp_client_t::close_app("msgpack://127.0.0.1:8001/");

  // Because the closing of an engine would _not_ cause the closing of the app,
  // so we have to explicitly close the app.
  aptima::msgpack_tcp_client_t::close_app("msgpack://127.0.0.1:8002/");

  ten_thread_join(app_thread_1, -1);
  ten_thread_join(app_thread_2, -1);
}
