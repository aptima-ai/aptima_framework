//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <memory>
#include <nlohmann/json.hpp>
#include <string>

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
    if (std::string(cmd->get_name()) == "hello_world") {
      ten_env.send_cmd(std::move(cmd));
      return;
    }
  }
};

class test_extension_2 : public ten::extension_t {
 public:
  explicit test_extension_2(const std::string &name) : ten::extension_t(name) {}

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (std::string(cmd->get_name()) == "hello_mapping") {
      if (cmd->get_property_int64("test_group[3][4].test_property_name_1") ==
              32 &&
          cmd->get_property_string("test_group[2][40].test_property_name_2") ==
              "may the force be with you.") {
        auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
        cmd_result->set_property("detail", "hello world, too");
        ten_env.return_result(std::move(cmd_result), std::move(cmd));
      }
    }
  }
};

class test_app : public ten::app_t {
 public:
  void on_configure(ten::ten_env_t &ten_env) override {
    ten::ten_env_internal_accessor_t ten_env_internal_accessor(&ten_env);
    bool rc = ten_env_internal_accessor.init_manifest_from_json(
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
                          "name": "0",
                          "auto_start": false,
                          "nodes": [{
                            "type": "extension_group",
                            "name": "cmd_mapping_path_array_3__extension_group",
                            "addon": "default_extension_group"
                          },{
                            "type": "extension",
                            "name": "test extension 1",
                            "addon": "cmd_mapping_path_array_3__test_extension_1",
                            "extension_group": "cmd_mapping_path_array_3__extension_group"
                          },{
                            "type": "extension",
                            "name": "test extension 2",
                            "addon": "cmd_mapping_path_array_3__test_extension_2",
                            "extension_group": "cmd_mapping_path_array_3__extension_group"
                          }],
                          "connections": [{
                            "app": "msgpack://127.0.0.1:8001/",
                            "extension_group": "cmd_mapping_path_array_3__extension_group",
                            "extension": "test extension 1",
                            "cmd": [{
                              "name": "hello_world",
                              "dest": [{
                                "app": "msgpack://127.0.0.1:8001/",
                                "extension_group": "cmd_mapping_path_array_3__extension_group",
                                "extension": "test extension 2",
                                "msg_conversion": {
                                  "type": "per_property",
                                  "rules": [{
                                    "path": "_ten.name",
                                    "conversion_mode": "fixed_value",
                                    "value": "hello_mapping"
                                  },{
                                    "path": "test_group[3][4].test_property_name_1",
                                    "conversion_mode": "from_original",
                                    "original_path": "test_property"
                                  },{
                                    "path": "test_group[2][40].test_property_name_2",
                                    "conversion_mode": "fixed_value",
                                    "value": "may the force be with you."
                                  }]
                                }
                              }]
                            }]
                          }]
                        }]
                      }
                    })###"
        // clang-format on
    );
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

TEN_CPP_REGISTER_ADDON_AS_EXTENSION(cmd_mapping_path_array_3__test_extension_1,
                                    test_extension_1);
TEN_CPP_REGISTER_ADDON_AS_EXTENSION(cmd_mapping_path_array_3__test_extension_2,
                                    test_extension_2);

}  // namespace

TEST(CmdConversionTest, CmdConversionPathArray3) {  // NOLINT
  // Start app.
  auto *app_thread =
      ten_thread_create("app thread", test_app_thread_main, nullptr);

  // Create a client and connect to the app.
  auto *client = new ten::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

  // Send a user-defined 'hello world' command.
  nlohmann::json resp = client->send_json_and_recv_resp_in_json(
      R"({
           "_ten": {
             "name": "hello_world",
             "seq_id": "137",
             "dest": [{
               "app": "msgpack://127.0.0.1:8001/",
               "graph": "0",
               "extension_group": "cmd_mapping_path_array_3__extension_group",
               "extension": "test extension 1"
             }]
           },
           "test_property": 32
         })"_json);
  ten_test::check_result_is(resp, "137", TEN_STATUS_CODE_OK,
                            "hello world, too");

  delete client;

  ten_thread_join(app_thread, -1);
}
