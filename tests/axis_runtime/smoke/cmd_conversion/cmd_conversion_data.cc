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
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/aptima_runtime/smoke/util/binding/cpp/check.h"

namespace {

class test_extension_1 : public ten::extension_t {
 public:
  explicit test_extension_1(const char *name) : ten::extension_t(name) {}

  void on_cmd(ten::aptima_env_t &aptima_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (cmd->get_name() == "send_data") {
      auto data = ten::data_t::create("aaa");
      data->set_property("prop_bool", true);
      aptima_env.send_data(std::move(data));

      auto cmd_result = ten::cmd_result_t::create(aptima_STATUS_CODE_OK);
      cmd_result->set_property("detail", "data sent");
      aptima_env.return_result(std::move(cmd_result), std::move(cmd));
      return;
    }
  }
};

class test_extension_2 : public ten::extension_t {
 public:
  explicit test_extension_2(const char *name) : ten::extension_t(name) {}

  void on_cmd(ten::aptima_env_t &aptima_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (cmd->get_name() == "data_received_check") {
      if (data_received) {
        auto cmd_result = ten::cmd_result_t::create(aptima_STATUS_CODE_OK);
        cmd_result->set_property("detail", "data received");
        aptima_env.return_result(std::move(cmd_result), std::move(cmd));
      } else {
        auto cmd_result = ten::cmd_result_t::create(aptima_STATUS_CODE_ERROR);
        cmd_result->set_property("detail", "data not received");
        aptima_env.return_result(std::move(cmd_result), std::move(cmd));
      }
    }
  }

  void on_data(ten::aptima_env_t &aptima_env,
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

class test_app : public ten::app_t {
 public:
  void on_configure(ten::aptima_env_t &aptima_env) override {
    bool rc = ten::aptima_env_internal_accessor_t::init_manifest_from_json(
        aptima_env,
        // clang-format off
                 R"###({
                     "type": "app",
                     "name": "test_app",
                     "version": "0.1.0"
                   })###"
        // clang-format on
    );
    ASSERT_EQ(rc, true);

    rc = aptima_env.init_property_from_json(
        // clang-format off
                 R"###({
                      "_ten": {
                        "uri": "msgpack://127.0.0.1:8001/",
                        "log_level": 2,
                        "predefined_graphs": [{
                          "name": "default",
                          "auto_start": false,
                          "singleton": true,
                          "nodes": [{
                            "app": "msgpack://127.0.0.1:8001/",
                            "type": "extension",
                            "name": "test_extension_1",
                            "addon": "cmd_mapping_data_extension_1",
                            "extension_group": "cmd_mapping_data_extension_group"
                          },{
                            "app": "msgpack://127.0.0.1:8001/",
                            "type": "extension",
                            "name": "test_extension_2",
                            "addon": "cmd_mapping_data_extension_2",
                            "extension_group": "cmd_mapping_data_extension_group"
                          }],
                          "connections": [{
                            "app": "msgpack://127.0.0.1:8001/",
                            "extension": "test_extension_1",
                            "data": [{
                              "name": "aaa",
                              "dest": [{
                                "app": "msgpack://127.0.0.1:8001/",
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

    aptima_env.on_configure_done();
  }
};

void *test_app_thread_main(aptima_UNUSED void *args) {
  auto *app = new test_app();
  app->run();
  delete app;

  return nullptr;
}

aptima_CPP_REGISTER_ADDON_AS_EXTENSION(cmd_mapping_data_extension_1,
                                    test_extension_1);
aptima_CPP_REGISTER_ADDON_AS_EXTENSION(cmd_mapping_data_extension_2,
                                    test_extension_2);

}  // namespace

TEST(CmdConversionTest, CmdConversionData) {  // NOLINT
  // Start app.
  auto *app_thread =
      aptima_thread_create("app thread", test_app_thread_main, nullptr);

  // Create a client and connect to the app.
  auto *client = new ten::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

  // Send a user-defined 'send_data' command.
  auto send_data_cmd = ten::cmd_t::create("send_data");
  send_data_cmd->set_dest("msgpack://127.0.0.1:8001/", "default",
                          "cmd_mapping_data_extension_group",
                          "test_extension_1");
  auto cmd_result = client->send_cmd_and_recv_result(std::move(send_data_cmd));
  aptima_test::check_status_code(cmd_result, aptima_STATUS_CODE_OK);
  aptima_test::check_detail_with_string(cmd_result, "data sent");

  // Send 'data_received_check' command.
  auto data_received_check_cmd = ten::cmd_t::create("data_received_check");
  data_received_check_cmd->set_dest("msgpack://127.0.0.1:8001/", "default",
                                    "cmd_mapping_data_extension_group",
                                    "test_extension_2");
  cmd_result =
      client->send_cmd_and_recv_result(std::move(data_received_check_cmd));
  aptima_test::check_status_code(cmd_result, aptima_STATUS_CODE_OK);
  aptima_test::check_detail_with_string(cmd_result, "data received");

  delete client;

  aptima_thread_join(app_thread, -1);
}
