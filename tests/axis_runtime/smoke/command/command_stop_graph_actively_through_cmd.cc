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
      aptima_env.send_cmd(std::move(cmd));
      return;
    }
  }
};

class test_extension_3 : public aptima::extension_t {
 public:
  explicit test_extension_3(const char *name) : aptima::extension_t(name) {}

  void on_cmd(aptima::aptima_env_t &aptima_env,
              std::unique_ptr<aptima::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      aptima_env.send_cmd(std::move(cmd));
      return;
    }
  }
};

class test_extension_4 : public aptima::extension_t {
 public:
  explicit test_extension_4(const char *name) : aptima::extension_t(name) {}

  void on_cmd(aptima::aptima_env_t &aptima_env,
              std::unique_ptr<aptima::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      auto cmd_result = aptima::cmd_result_t::create(aptima_STATUS_CODE_OK);
      cmd_result->set_property("detail",
                               "must return result before close engine");
      aptima_env.return_result(std::move(cmd_result), std::move(cmd));

      auto stop_graph_cmd = aptima::cmd_stop_graph_t::create();
      stop_graph_cmd->set_dest("localhost", nullptr, nullptr, nullptr);
      aptima_env.send_cmd(std::move(stop_graph_cmd));
    }
  }
};

class test_app_1 : public aptima::app_t {
 public:
  void on_configure(aptima::aptima_env_t &aptima_env) override {
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
                        "one_event_loop_per_engine": true,
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

class test_app_3 : public aptima::app_t {
 public:
  void on_configure(aptima::aptima_env_t &aptima_env) override {
    bool rc = aptima_env.init_property_from_json(
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

void *app_thread_3_main(aptima_UNUSED void *args) {
  auto *app = new test_app_3();
  app->run();
  delete app;

  return nullptr;
}

aptima_CPP_REGISTER_ADDON_AS_EXTENSION(
    command_stop_graph_actively_through_cmd__extension_1, test_extension_1);
aptima_CPP_REGISTER_ADDON_AS_EXTENSION(
    command_stop_graph_actively_through_cmd__extension_2, test_extension_2);
aptima_CPP_REGISTER_ADDON_AS_EXTENSION(
    command_stop_graph_actively_through_cmd__extension_3, test_extension_3);
aptima_CPP_REGISTER_ADDON_AS_EXTENSION(
    command_stop_graph_actively_through_cmd__extension_4, test_extension_4);

}  // namespace

TEST(ExtensionTest, CommandStopGraphActivelyThroughCmd) {  // NOLINT
  // Start app.
  auto *app_thread_3 =
      aptima_thread_create("app thread 3", app_thread_3_main, nullptr);
  auto *app_thread_2 =
      aptima_thread_create("app thread 2", app_thread_2_main, nullptr);
  auto *app_thread_1 =
      aptima_thread_create("app thread 1", app_thread_1_main, nullptr);

  // Create a client and connect to the app.
  aptima::msgpack_tcp_client_t *client = nullptr;

  for (size_t i = 0; i < MULTIPLE_APP_SCENARIO_GRAPH_CONSTRUCTION_RETRY_TIMES;
       ++i) {
    client = new aptima::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

    // Send graph.
    auto start_graph_cmd = aptima::cmd_start_graph_t::create();
    start_graph_cmd->set_graph_from_json(R"({
           "nodes": [{
                 "type": "extension",
                 "name": "test_extension_1",
                 "addon": "command_stop_graph_actively_through_cmd__extension_1",
                 "app": "msgpack://127.0.0.1:8001/",
                 "extension_group": "command_stop_graph_actively_through_cmd_1"
               },{
                 "type": "extension",
                 "name": "test_extension_2",
                 "addon": "command_stop_graph_actively_through_cmd__extension_2",
                 "app": "msgpack://127.0.0.1:8002/",
                 "extension_group": "command_stop_graph_actively_through_cmd_2"
               },{
                 "type": "extension",
                 "name": "test_extension_3",
                 "addon": "command_stop_graph_actively_through_cmd__extension_3",
                 "app": "msgpack://127.0.0.1:8002/",
                 "extension_group": "command_stop_graph_actively_through_cmd_2"
               },{
                 "type": "extension",
                 "name": "test_extension_4",
                 "addon": "command_stop_graph_actively_through_cmd__extension_4",
                 "app": "msgpack://127.0.0.1:8003/",
                 "extension_group": "command_stop_graph_actively_through_cmd_3"
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
      aptima_test::check_status_code(cmd_result, aptima_STATUS_CODE_OK);
      break;
    } else {
      delete client;
      client = nullptr;

      // To prevent from busy re-trying.
      aptima_sleep(10);
    }
  }

  aptima_ASSERT(client, "Failed to connect to the APTIMA app.");

  auto hello_world_cmd = aptima::cmd_t::create("hello_world");
  hello_world_cmd->set_dest("msgpack://127.0.0.1:8001/", nullptr,
                            "command_stop_graph_actively_through_cmd_1",
                            "test_extension_1");
  client->send_cmd(std::move(hello_world_cmd));

  delete client;

  aptima::msgpack_tcp_client_t::close_app("msgpack://127.0.0.1:8001/");
  aptima::msgpack_tcp_client_t::close_app("msgpack://127.0.0.1:8002/");
  aptima::msgpack_tcp_client_t::close_app("msgpack://127.0.0.1:8003/");

  aptima_thread_join(app_thread_1, -1);
  aptima_thread_join(app_thread_2, -1);
  aptima_thread_join(app_thread_3, -1);
}
