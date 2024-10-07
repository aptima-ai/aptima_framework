//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "include_internal/ten_runtime/binding/cpp/ten.h"
#include "ten_utils/lib/thread.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/ten_runtime/smoke/extension_test/util/binding/cpp/check.h"

namespace {

class test_extension_1 : public ten::extension_t {
 public:
  explicit test_extension_1(const std::string &name) : ten::extension_t(name) {}

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    nlohmann::json json = nlohmann::json::parse(cmd->to_json());

    if (json["_ten"]["name"] == "send_data") {
      auto data = ten::data_t::create("aaa");
      data->set_property("prop_bool", true);
      ten_env.send_data(std::move(data));

      auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
      cmd_result->set_property("detail", "data sent");
      ten_env.return_result(std::move(cmd_result), std::move(cmd));
      return;
    }
  }
};

class test_extension_2 : public ten::extension_t {
 public:
  explicit test_extension_2(const std::string &name) : ten::extension_t(name) {}

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    nlohmann::json json = nlohmann::json::parse(cmd->to_json());
    if (json["_ten"]["name"] == "data_received_check") {
      if (data_received) {
        auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
        cmd_result->set_property("detail", "data received");
        ten_env.return_result(std::move(cmd_result), std::move(cmd));
      } else {
        auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_ERROR);
        cmd_result->set_property("detail", "data not received");
        ten_env.return_result(std::move(cmd_result), std::move(cmd));
      }
    }
  }

  void on_data(ten::ten_env_t &ten_env,
               std::unique_ptr<ten::data_t> data) override {
    std::string name = data->get_name();

    auto test_prop = data->get_property_string("test_prop_string");
    auto test_prop_bool_fixed = data->get_property_bool("test_prop_bool_fixed");
    auto test_prop_bool_from_origin =
        data->get_property_bool("test_prop_bool_from_origin");

    if (name == "bbb" && test_prop == "hello" && test_prop_bool_fixed &&
        test_prop_bool_from_origin) {
      data_received = true;
    }
  }

 private:
  bool data_received = false;
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
    ten::ten_env_internal_accessor_t ten_env_internal_accessor(&ten_env);
    bool rc = ten_env_internal_accessor.init_manifest_from_json(
        // clang-format off
                 R"###({
                     "type": "app",
                     "name": "test_app",
                     "version": "0.1.0"
                   })###"
        // clang-format on
    );
    ASSERT_EQ(rc, true);

    rc = ten_env.init_property_from_json(
        // clang-format off
                 R"###({
                      "_ten": {
                        "uri": "msgpack://127.0.0.1:8001/",
                        "log_level": 2,
                        "predefined_graphs": [{
                          "name": "default",
                          "auto_start": false,
                          "nodes": [{
                            "type": "extension_group",
                            "name": "cmd_mapping_graph_extension_1",
                            "addon": "cmd_mapping_data_extension_1__extension_group"
                          }],
                          "connections": [{
                            "app": "msgpack://127.0.0.1:8001/",
                            "extension_group": "cmd_mapping_graph_extension_1",
                            "extension": "test_extension_1",
                            "data": [{
                              "name": "aaa",
                              "dest": [{
                                "app": "msgpack://127.0.0.1:8001/",
                                "extension_group": "cmd_mapping_graph_extension_1",
                                "extension": "test_extension_2",
                                "msg_conversion": {
                                  "type": "per_property",
                                  "rules": [{
                                    "path": "_ten.name",
                                    "conversion_mode": "fixed_value",
                                    "value": "bbb"
                                  },{
                                    "path": "test_prop_string",
                                    "conversion_mode": "fixed_value",
                                    "value": "hello"
                                  },{
                                    "path": "test_prop_bool_fixed",
                                    "conversion_mode": "fixed_value",
                                    "value": true
                                  },{
                                    "path": "test_prop_bool_from_origin",
                                    "conversion_mode": "from_original",
                                    "original_path": "prop_bool"
                                  }]
                                }
                              }]
                            }]
                          }]
                        }]
                      }
                    })###"
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
    cmd_mapping_data_extension_1__extension_group, test_extension_group);

}  // namespace

TEST(CmdConversionTest, CmdConversionData) {  // NOLINT
  // Start app.
  auto *app_thread =
      ten_thread_create("app thread", test_app_thread_main, nullptr);

  // Create a client and connect to the app.
  auto *client = new ten::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

  // Send a user-defined 'send_data' command.
  nlohmann::json resp = client->send_json_and_recv_resp_in_json(
      R"({
           "_ten": {
             "name": "send_data",
             "seq_id": "137",
             "dest": [{
               "app": "msgpack://127.0.0.1:8001/",
               "graph": "default",
               "extension_group": "cmd_mapping_graph_extension_1",
               "extension": "test_extension_1"
             }]
           }
         })"_json);
  ten_test::check_result_is(resp, "137", TEN_STATUS_CODE_OK, "data sent");

  // Send 'data_received_check' command.
  resp = client->send_json_and_recv_resp_in_json(
      R"({
           "_ten": {
             "name": "data_received_check",
             "seq_id": "138",
             "dest": [{
               "app": "msgpack://127.0.0.1:8001/",
               "graph": "default",
               "extension_group": "cmd_mapping_graph_extension_1",
               "extension": "test_extension_2"
             }]
           }
         })"_json);
  ten_test::check_result_is(resp, "138", TEN_STATUS_CODE_OK, "data received");

  delete client;

  ten_thread_join(app_thread, -1);
}
