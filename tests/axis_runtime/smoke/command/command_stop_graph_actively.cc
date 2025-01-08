//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <nlohmann/json.hpp>
#include <string>

#include "gtest/gtest.h"
#include "include_internal/axis_runtime/binding/cpp/ten.h"
#include "axis_runtime/binding/cpp/detail/msg/cmd/stop_graph.h"
#include "axis_utils/lib/thread.h"
#include "axis_utils/lib/time.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/common/constant.h"
#include "tests/axis_runtime/smoke/util/binding/cpp/check.h"

namespace {

class test_extension_1 : public ten::extension_t {
 public:
  explicit test_extension_1(const char *name) : ten::extension_t(name) {}

  void on_cmd(ten::axis_env_t &axis_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      axis_env.send_cmd(std::move(cmd));
      return;
    }
  }
};

class test_extension_2 : public ten::extension_t {
 public:
  explicit test_extension_2(const char *name) : ten::extension_t(name) {}

  void on_cmd(ten::axis_env_t &axis_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      axis_env.send_cmd(std::move(cmd));
      return;
    }
  }
};

class test_extension_3 : public ten::extension_t {
 public:
  explicit test_extension_3(const char *name) : ten::extension_t(name) {}

  void on_cmd(ten::axis_env_t &axis_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      axis_env.send_cmd(std::move(cmd));
      return;
    }
  }
};

class test_extension_4 : public ten::extension_t {
 public:
  explicit test_extension_4(const char *name) : ten::extension_t(name) {}

  void on_cmd(ten::axis_env_t &axis_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      auto cmd_result = ten::cmd_result_t::create(axis_STATUS_CODE_OK);
      cmd_result->set_property("detail",
                               "must return result before close engine");
      axis_env.return_result(std::move(cmd_result), std::move(cmd));

      auto stop_graph_cmd = ten::cmd_stop_graph_t::create();
      stop_graph_cmd->set_dest("localhost", nullptr, nullptr, nullptr);
      axis_env.send_cmd(std::move(stop_graph_cmd));
    }
  }
};

class test_app_1 : public ten::app_t {
 public:
  void on_configure(ten::axis_env_t &axis_env) override {
    bool rc = axis_env.init_property_from_json(
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

    axis_env.on_configure_done();
  }
};

class test_app_2 : public ten::app_t {
 public:
  void on_configure(ten::axis_env_t &axis_env) override {
    bool rc = axis_env.init_property_from_json(
        // clang-format off
                 R"({
                      "_ten": {
                        "uri": "msgpack://127.0.0.1:8002/",
                        "one_event_loop_per_engine": true,
                        "long_running_mode": true,
                        "log_level": 2
                      }
                    })"
        // clang-format on
    );
    ASSERT_EQ(rc, true);

    axis_env.on_configure_done();
  }
};

class test_app_3 : public ten::app_t {
 public:
  void on_configure(ten::axis_env_t &axis_env) override {
    bool rc = axis_env.init_property_from_json(
        // clang-format off
                 R"({
                      "_ten": {
                        "uri": "msgpack://127.0.0.1:8003/",
                        "long_running_mode": true,
                        "log_level": 2
                      }
                    })"
        // clang-format on
    );
    ASSERT_EQ(rc, true);

    axis_env.on_configure_done();
  }
};

void *app_thread_1_main(axis_UNUSED void *args) {
  auto *app = new test_app_1();
  app->run();
  delete app;

  return nullptr;
}

void *app_thread_2_main(axis_UNUSED void *args) {
  auto *app = new test_app_2();
  app->run();
  delete app;

  return nullptr;
}

void *app_thread_3_main(axis_UNUSED void *args) {
  auto *app = new test_app_3();
  app->run();
  delete app;

  return nullptr;
}

axis_CPP_REGISTER_ADDON_AS_EXTENSION(command_stop_graph_actively__extension_1,
                                    test_extension_1);
axis_CPP_REGISTER_ADDON_AS_EXTENSION(command_stop_graph_actively__extension_2,
                                    test_extension_2);
axis_CPP_REGISTER_ADDON_AS_EXTENSION(command_stop_graph_actively__extension_3,
                                    test_extension_3);
axis_CPP_REGISTER_ADDON_AS_EXTENSION(command_stop_graph_actively__extension_4,
                                    test_extension_4);

}  // namespace

TEST(ExtensionTest, CommandStopGraphActively) {  // NOLINT
  // Start app.
  auto *app_thread_3 =
      axis_thread_create("app thread 3", app_thread_3_main, nullptr);
  auto *app_thread_2 =
      axis_thread_create("app thread 2", app_thread_2_main, nullptr);
  auto *app_thread_1 =
      axis_thread_create("app thread 1", app_thread_1_main, nullptr);

  // Create a client and connect to the app.
  ten::msgpack_tcp_client_t *client = nullptr;

  for (size_t i = 0; i < MULTIPLE_APP_SCENARIO_GRAPH_CONSTRUCTION_RETRY_TIMES;
       ++i) {
    client = new ten::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

    // Send graph.
    auto start_graph_cmd = ten::cmd_start_graph_t::create();
    start_graph_cmd->set_graph_from_json(R"({
           "nodes": [{
                 "type": "extension",
                 "name": "test_extension_1",
                 "addon": "command_stop_graph_actively__extension_1",
                 "app": "msgpack://127.0.0.1:8001/",
                 "extension_group": "command_stop_graph_actively_1"
               },{
                 "type": "extension",
                 "name": "test_extension_2",
                 "addon": "command_stop_graph_actively__extension_2",
                 "app": "msgpack://127.0.0.1:8002/",
                 "extension_group": "command_stop_graph_actively_2"
               },{
                 "type": "extension",
                 "name": "test_extension_3",
                 "addon": "command_stop_graph_actively__extension_3",
                 "app": "msgpack://127.0.0.1:8002/",
                 "extension_group": "command_stop_graph_actively_2"
               },{
                 "type": "extension",
                 "name": "test_extension_4",
                 "addon": "command_stop_graph_actively__extension_4",
                 "app": "msgpack://127.0.0.1:8003/",
                 "extension_group": "command_stop_graph_actively_3"
               }],
               "connections": [{
                 "app": "msgpack://127.0.0.1:8001/",
                 "extension": "test_extension_1",
                 "cmd": [{
                   "name": "hello_world",
                   "dest": [{
                     "app": "msgpack://127.0.0.1:8002/",
                     "extension": "test_extension_3"
                   }]
                 }]
               },{
                "app": "msgpack://127.0.0.1:8002/",
                "extension": "test_extension_2",
                "cmd": [{
                   "name": "hello_world",
                   "dest": [{
                     "app": "msgpack://127.0.0.1:8002/",
                     "extension": "test_extension_3"
                   }]
                 }]
               },{
                "app": "msgpack://127.0.0.1:8002/",
                "extension": "test_extension_3",
                "cmd": [{
                   "name": "hello_world",
                   "dest": [{
                     "app": "msgpack://127.0.0.1:8003/",
                     "extension": "test_extension_4"
                   }]
                 }]
               }]
             })");

    auto cmd_result =
        client->send_cmd_and_recv_result(std::move(start_graph_cmd));

    if (cmd_result) {
      axis_test::check_status_code(cmd_result, axis_STATUS_CODE_OK);
      break;
    } else {
      delete client;
      client = nullptr;

      // To prevent from busy re-trying.
      axis_sleep(10);
    }
  }

  axis_ASSERT(client, "Failed to connect to the TEN app.");

  auto hello_world_cmd = ten::cmd_t::create("hello_world");
  hello_world_cmd->set_dest("msgpack://127.0.0.1:8001/", nullptr,
                            "command_stop_graph_actively_1",
                            "test_extension_1");
  client->send_cmd(std::move(hello_world_cmd));

  delete client;

  ten::msgpack_tcp_client_t::close_app("msgpack://127.0.0.1:8001/");
  ten::msgpack_tcp_client_t::close_app("msgpack://127.0.0.1:8002/");
  ten::msgpack_tcp_client_t::close_app("msgpack://127.0.0.1:8003/");

  axis_thread_join(app_thread_1, -1);
  axis_thread_join(app_thread_2, -1);
  axis_thread_join(app_thread_3, -1);
}
