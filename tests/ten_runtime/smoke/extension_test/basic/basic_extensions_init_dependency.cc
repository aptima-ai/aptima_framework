//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <chrono>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include <vector>

#include "gtest/gtest.h"
#include "include_internal/ten_runtime/binding/cpp/ten.h"
#include "ten_utils/lib/thread.h"
#include "ten_utils/macro/macros.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/ten_runtime/smoke/extension_test/util/binding/cpp/check.h"

using namespace std::placeholders;

#define EXTENSION_PROP_NAME_GREETING "greeting"
#define EXTENSION_PROP_VALUE_GREETING "hello "

namespace {

class test_extension_1 : public ten::extension_t {
 public:
  explicit test_extension_1(const std::string &name) : ten::extension_t(name) {}

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    nlohmann::json json = nlohmann::json::parse(cmd->to_json());

    if (json["_ten"]["name"] == "hello_world") {
      ten_env.send_cmd(std::move(cmd));
    } else if (json["_ten"]["name"] == "get_name") {
      auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
      cmd_result->set_property("detail", "test_extension_1");
      ten_env.return_result(std::move(cmd_result), std::move(cmd));
    }
  }
};

class test_extension_2 : public ten::extension_t {
 public:
  explicit test_extension_2(const std::string &name) : ten::extension_t(name) {}

  void on_configure(ten::ten_env_t &ten_env) override {
    bool rc = ten_env.init_property_from_json(
                           // clang-format off
                           "{\
                              \"" EXTENSION_PROP_NAME_GREETING "\": \
                              " TEN_XSTR(EXTENSION_PROP_VALUE_GREETING) "\
                            }"
                           // clang-format on
    , nullptr);
    ASSERT_EQ(rc, true);

    ten_env.on_configure_done();
  }

  void on_init(ten::ten_env_t &ten_env) override {
    auto *ten_env_proxy = ten::ten_env_proxy_t::create(ten_env);

    fetch_property_thread_ = std::thread(
        [this](ten::ten_env_proxy_t *ten_env_proxy) {
          std::this_thread::sleep_for(std::chrono::milliseconds(1000));

          ten_env_proxy->notify(
              [this](ten::ten_env_t &ten_env) { this->get_property(ten_env); });

          delete ten_env_proxy;
        },
        ten_env_proxy);
  }

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    nlohmann::json json = nlohmann::json::parse(cmd->to_json());
    if (json["_ten"]["name"] == "hello_world") {
      auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
      cmd_result->set_property("detail", greeting_);
      ten_env.return_result(std::move(cmd_result), std::move(cmd));
    }
  }

  void on_stop(ten::ten_env_t &ten_env) override {
    if (fetch_property_thread_.joinable()) {
      fetch_property_thread_.join();
    }

    ten_env.on_stop_done();
  }

 private:
  std::string greeting_;
  std::thread fetch_property_thread_;

  void get_property(ten::ten_env_t &ten_env) {
    ten_env.get_property_string_async(
        EXTENSION_PROP_NAME_GREETING,
        [this](ten::ten_env_t &ten_env, const std::string &result,
               TEN_UNUSED ten::error_t *err) {
          greeting_ = result;

          auto cmd = ten::cmd_t::create("get_name");
          ten_env.send_cmd(
              std::move(cmd),
              [this](ten::ten_env_t &ten_env,
                     std::unique_ptr<ten::cmd_result_t> cmd_result) {
                auto name = cmd_result->get_property_string("detail");
                greeting_ += name;

                ten_env.on_init_done();
                return true;
              },
              nullptr);
        });
  }
};

class test_extension_group : public ten::extension_group_t {
 public:
  explicit test_extension_group(const std::string &name)
      : ten::extension_group_t(name) {}

  void on_create_extensions(ten::ten_env_t &ten_env) override {
    std::vector<ten::extension_t *> extensions;
    extensions.push_back(new test_extension_1("test_extension_1"));
    extensions.push_back(new test_extension_2("test_extension_2"));
    ten_env.on_create_extensions_done(extensions);
  }

  void on_destroy_extensions(
      ten::ten_env_t &ten_env,
      const std::vector<ten::extension_t *> &extensions) override {
    for (auto *extension : extensions) {
      delete extension;
    }
    ten_env.on_destroy_extensions_done();
  }
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

TEN_CPP_REGISTER_ADDON_AS_EXTENSION_GROUP(
    basic_extensions_init_dependency__extension_group, test_extension_group);

}  // namespace

TEST(ExtensionTest, BasicExtensionsInitDependency) {  // NOLINT
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
               "name": "basic_extensions_init_dependency",
               "addon": "basic_extensions_init_dependency__extension_group",
               "app": "msgpack://127.0.0.1:8001/"
             },{
               "type": "extension",
               "name": "test_extension_1",
               "extension_group": "basic_extensions_init_dependency",
               "app": "msgpack://127.0.0.1:8001/"
             },{
               "type": "extension",
               "name": "test_extension_2",
               "extension_group": "basic_extensions_init_dependency",
               "app": "msgpack://127.0.0.1:8001/"
             }],
             "connections": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "basic_extensions_init_dependency",
               "extension": "test_extension_1",
               "cmd": [{
                 "name": "hello_world",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "basic_extensions_init_dependency",
                   "extension": "test_extension_2"
                 }]
               }]
             },{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "basic_extensions_init_dependency",
               "extension": "test_extension_2",
               "cmd": [{
                 "name": "get_name",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "basic_extensions_init_dependency",
                   "extension": "test_extension_1"
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
               "extension_group": "basic_extensions_init_dependency",
               "extension": "test_extension_1"
             }]
           }
         })"_json);
  ten_test::check_result_is(
      resp, "137", TEN_STATUS_CODE_OK,
      std::string(EXTENSION_PROP_VALUE_GREETING) + "test_extension_1");

  delete client;

  ten_thread_join(app_thread, -1);
}
