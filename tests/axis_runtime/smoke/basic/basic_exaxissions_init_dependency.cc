//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <chrono>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>

#include "gtest/gtest.h"
#include "include_internal/aptima_runtime/binding/cpp/aptima.h"
#include "aptima_utils/lib/thread.h"
#include "aptima_utils/macro/macros.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/aptima_runtime/smoke/util/binding/cpp/check.h"

using namespace std::placeholders;

#define EXTENSION_PROP_NAME_GREETING "greeting"
#define EXTENSION_PROP_VALUE_GREETING "hello "

namespace {

class test_extension_1 : public aptima::extension_t {
 public:
  explicit test_extension_1(const char *name) : aptima::extension_t(name) {}

  void on_cmd(aptima::aptima_env_t &aptima_env,
              std::unique_ptr<aptima::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      aptima_env.send_cmd(std::move(cmd));
    } else if (cmd->get_name() == "get_name") {
      auto cmd_result = aptima::cmd_result_t::create(aptima_STATUS_CODE_OK);
      cmd_result->set_property("detail", "test_extension_1");
      aptima_env.return_result(std::move(cmd_result), std::move(cmd));
    }
  }
};

class test_extension_2 : public aptima::extension_t {
 public:
  explicit test_extension_2(const char *name) : aptima::extension_t(name) {}

  void on_configure(aptima::aptima_env_t &aptima_env) override {
    bool rc = aptima_env.init_property_from_json(
                           // clang-format off
                           "{\
                              \"" EXTENSION_PROP_NAME_GREETING "\": \
                              " aptima_XSTR(EXTENSION_PROP_VALUE_GREETING) "\
                            }"
                           // clang-format on
    , nullptr);
    ASSERT_EQ(rc, true);

    aptima_env.on_configure_done();
  }

  void on_init(aptima::aptima_env_t &aptima_env) override {
    auto *aptima_env_proxy = aptima::aptima_env_proxy_t::create(aptima_env);

    fetch_property_thread_ = std::thread(
        [this](aptima::aptima_env_proxy_t *aptima_env_proxy) {
          std::this_thread::sleep_for(std::chrono::milliseconds(1000));

          aptima_env_proxy->notify(
              [this](aptima::aptima_env_t &aptima_env) { this->get_property(aptima_env); });

          delete aptima_env_proxy;
        },
        aptima_env_proxy);
  }

  void on_cmd(aptima::aptima_env_t &aptima_env,
              std::unique_ptr<aptima::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      auto cmd_result = aptima::cmd_result_t::create(aptima_STATUS_CODE_OK);
      cmd_result->set_property("detail", greeting_);
      aptima_env.return_result(std::move(cmd_result), std::move(cmd));
    }
  }

  void on_stop(aptima::aptima_env_t &aptima_env) override {
    if (fetch_property_thread_.joinable()) {
      fetch_property_thread_.join();
    }

    aptima_env.on_stop_done();
  }

 private:
  std::string greeting_;
  std::thread fetch_property_thread_;

  void get_property(aptima::aptima_env_t &aptima_env) {
    greeting_ = aptima_env.get_property_string(EXTENSION_PROP_NAME_GREETING);

    auto cmd = aptima::cmd_t::create("get_name");
    aptima_env.send_cmd(
        std::move(cmd),
        [this](aptima::aptima_env_t &aptima_env,
               std::unique_ptr<aptima::cmd_result_t> cmd_result,
               aptima::error_t *err) {
          auto name = cmd_result->get_property_string("detail");
          greeting_ += name;

          aptima_env.on_init_done();
          return true;
        },
        nullptr);
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

aptima_CPP_REGISTER_ADDON_AS_EXTENSION(
    basic_extensions_init_dependency__extension_1, test_extension_1);
aptima_CPP_REGISTER_ADDON_AS_EXTENSION(
    basic_extensions_init_dependency__extension_2, test_extension_2);

}  // namespace

TEST(BasicTest, ExtensionsInitDependency) {  // NOLINT
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
               "addon": "basic_extensions_init_dependency__extension_1",
               "extension_group": "basic_extensions_init_dependency",
               "app": "msgpack://127.0.0.1:8001/"
             },{
               "type": "extension",
               "name": "test_extension_2",
               "addon": "basic_extensions_init_dependency__extension_2",
               "extension_group": "basic_extensions_init_dependency",
               "app": "msgpack://127.0.0.1:8001/"
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
               }]
             },{
               "app": "msgpack://127.0.0.1:8001/",
               "extension": "test_extension_2",
               "cmd": [{
                 "name": "get_name",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension": "test_extension_1"
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
                            "basic_extensions_init_dependency",
                            "test_extension_1");
  cmd_result = client->send_cmd_and_recv_result(std::move(hello_world_cmd));
  aptima_test::check_status_code(cmd_result, aptima_STATUS_CODE_OK);
  aptima_test::check_detail_with_string(
      cmd_result,
      std::string(EXTENSION_PROP_VALUE_GREETING) + "test_extension_1");

  delete client;

  aptima_thread_join(app_thread, -1);
}
