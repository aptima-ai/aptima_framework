//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <memory>
#include <nlohmann/json.hpp>
#include <string>

#include "gtest/gtest.h"
#include "include_internal/aptima_runtime/binding/cpp/aptima.h"
#include "aptima_utils/lib/thread.h"
#include "aptima_utils/lib/time.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/common/constant.h"
#include "tests/aptima_runtime/smoke/util/binding/cpp/check.h"

namespace {

class test_extension_1 : public aptima::extension_t {
 public:
  explicit test_extension_1(const char *name) : aptima::extension_t(name) {}

  void on_cmd(aptima::aptima_env_t &aptima_env,
              std::unique_ptr<aptima::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      aptima_env.send_cmd(std::move(cmd));
      return;
    }
  }
};

class test_extension_2 : public aptima::extension_t {
 public:
  explicit test_extension_2(const char *name) : aptima::extension_t(name) {}

  void on_cmd(aptima::aptima_env_t &aptima_env,
              std::unique_ptr<aptima::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      auto cmd_result = aptima::cmd_result_t::create(aptima_STATUS_CODE_OK);
      cmd_result->set_property("detail", "hello world, too");
      aptima_env.return_result(std::move(cmd_result), std::move(cmd));
    }
  }
};

class test_app_1 : public aptima::app_t {
 public:
  void on_configure(aptima::aptima_env_t &aptima_env) override {
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
    bool rc = aptima_env.init_property_from_json(
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

    aptima_env.on_configure_done();
  }
};

class test_app_2 : public aptima::app_t {
 public:
  void on_configure(aptima::aptima_env_t &aptima_env) override {
    bool rc = aptima_env.init_property_from_json(
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

aptima_CPP_REGISTER_ADDON_AS_EXTENSION(group_node_missing_2_apps__extension_1,
                                    test_extension_1);
aptima_CPP_REGISTER_ADDON_AS_EXTENSION(group_node_missing_2_apps__extension_2,
                                    test_extension_2);

}  // namespace

TEST(GraphTest, GroupNodeMissing2Apps) {  // NOLINT
  aptima_thread_t *app1_thread = nullptr;
  aptima_thread_t *app2_thread = nullptr;
  std::unique_ptr<aptima::msgpack_tcp_client_t> client = nullptr;

  try {
    // Start app.
    app2_thread = aptima_thread_create("app thread 2", app_thread_2_main, nullptr);
    app1_thread = aptima_thread_create("app thread 1", app_thread_1_main, nullptr);

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
      client = std::make_unique<aptima::msgpack_tcp_client_t>(
          "msgpack://127.0.0.1:8001/");

      // Send graph.

      auto start_graph_cmd = aptima::cmd_start_graph_t::create();
      start_graph_cmd->set_graph_from_json(R"({
           "nodes": [{
                   "type": "extension",
                   "addon": "group_node_missing_2_apps__extension_1",
                   "name": "test_extension_1",
                   "extension_group": "test_extension_group 1",
                   "app": "msgpack://127.0.0.1:8001/"
                 },{
                   "type": "extension",
                   "addon": "group_node_missing_2_apps__extension_2",
                   "name": "test_extension_2",
                   "extension_group": "test_extension_group 2",
                   "app": "msgpack://127.0.0.1:8002/"
                 }],
                 "connections": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension": "test_extension_1",
                   "cmd": [{
                     "name": "hello_world",
                     "dest": [{
                       "app": "msgpack://127.0.0.1:8002/",
                       "extension_group": "test_extension_group 2",
                       "extension": "test_extension_2"
                     }]
                   }]
                 }]
               })");
      auto cmd_result =
          client->send_cmd_and_recv_result(std::move(start_graph_cmd));

      if (cmd_result) {
        aptima_test::check_status_code(cmd_result, aptima_STATUS_CODE_OK);
        break;
      } else {
        client.reset();

        // To prevent from busy re-trying.
        aptima_sleep(10);
      }
    }

    aptima_ASSERT(client, "Failed to connect to the APTIMA app.");

    // Send a user-defined 'hello world' command.
    auto hello_world_cmd = aptima::cmd_t::create("hello_world");
    hello_world_cmd->set_dest("msgpack://127.0.0.1:8001/", nullptr,
                              "test_extension_group 1", "test_extension_1");

    auto cmd_result =
        client->send_cmd_and_recv_result(std::move(hello_world_cmd));

    aptima_test::check_status_code(cmd_result, aptima_STATUS_CODE_OK);
    aptima_test::check_detail_with_string(cmd_result, "hello world, too");

    // Because the closing of an engine would _not_ cause the closing of the
    // app, so we have to explicitly close the app.
    aptima::msgpack_tcp_client_t::close_app("msgpack://127.0.0.1:8001/");

    // Because the closing of an engine would _not_ cause the closing of the
    // app, so we have to explicitly close the app.
    aptima::msgpack_tcp_client_t::close_app("msgpack://127.0.0.1:8002/");
  } catch (...) {
    // Do nothing.
  }

  aptima_thread_join(app1_thread, -1);
  aptima_thread_join(app2_thread, -1);
}
