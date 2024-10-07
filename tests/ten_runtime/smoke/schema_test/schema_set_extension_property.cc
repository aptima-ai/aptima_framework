//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <cmath>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>

#include "gtest/gtest.h"
#include "include_internal/ten_runtime/binding/cpp/ten.h"
#include "ten_utils/lib/thread.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/ten_runtime/smoke/extension_test/util/binding/cpp/check.h"

namespace {

class test_extension : public ten::extension_t {
 public:
  explicit test_extension(const std::string &name) : ten::extension_t(name) {}

  void on_configure(ten::ten_env_t &ten_env) override {
    ten::ten_env_internal_accessor_t ten_env_internal_accessor(&ten_env);
    bool rc = ten_env_internal_accessor.init_manifest_from_json(
        // clang-format off
                 R"({
                      "type": "extension",
                      "name": "schema_set_extension_prop__test_extension",
                      "version": "0.1.0",
                                          "api": {
                        "property": {
                          "hello": {
                            "type": "string"
                          },
                          "a": {
                            "type": "int8"
                          },
                          "b": {
                            "type": "int16"
                          },
                          "c": {
                            "type": "int32"
                          },
                          "d": {
                            "type": "int64"
                          },
                          "e": {
                            "type": "uint8"
                          },
                          "f": {
                            "type": "uint16"
                          },
                          "g": {
                            "type": "uint32"
                          },
                          "h": {
                            "type": "uint64"
                          },
                          "i": {
                            "type": "float32"
                          },
                          "j": {
                            "type": "float64"
                          }
                        }
                      }
                    })"
        // clang-format on
    );
    ASSERT_EQ(rc, true);

    rc = ten_env.init_property_from_json(
        // clang-format off
                 R"({
                      "hello": "default",
                      "a": 1,
                      "b": 1,
                      "c": 1,
                      "d": 1,
                      "e": 1,
                      "f": 1,
                      "g": 1,
                      "h": 1,
                      "i": 1.0,
                      "j": 1.0
                    })"
        // clang-format on
    );
    ASSERT_EQ(rc, true);

    ten_env.on_configure_done();
  }

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (std::string(cmd->get_name()) == "hello_world") {
      auto number_a = ten_env.get_property_int8("a");
      ASSERT_EQ(static_cast<int8_t>(1), number_a);

      auto number_b = ten_env.get_property_int16("b");
      ASSERT_EQ(static_cast<int16_t>(1), number_b);

      auto number_c = ten_env.get_property_int32("c");
      ASSERT_EQ(static_cast<int32_t>(1), number_c);

      auto number_d = ten_env.get_property_int64("d");
      ASSERT_EQ(static_cast<int64_t>(1), number_d);

      auto number_e = ten_env.get_property_uint8("e");
      ASSERT_EQ(static_cast<uint8_t>(1), number_e);

      auto number_f = ten_env.get_property_uint16("f");
      ASSERT_EQ(static_cast<uint16_t>(1), number_f);

      auto number_g = ten_env.get_property_uint32("g");
      ASSERT_EQ(static_cast<uint32_t>(1), number_g);

      auto number_h = ten_env.get_property_uint64("h");
      ASSERT_EQ(static_cast<uint64_t>(1), number_h);

      auto number_i = ten_env.get_property_float32("i");
      ASSERT_EQ(true, fabs(1.0 - number_i) < 0.01);

      auto number_j = ten_env.get_property_float64("j");
      ASSERT_EQ(true, fabs(1.0 - number_j) < 0.01);

      // The type of `hello` should be string, the `set_property` should be
      // failed.

      bool rc = ten_env.set_property("hello", 1);
      ASSERT_EQ(rc, false);

      auto prop = ten_env.get_property_string("hello");
      ASSERT_EQ("default", prop);

      ten_env.set_property("hello", "world");
      prop = ten_env.get_property_string("hello");

      auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
      cmd_result->set_property("detail", prop);
      ten_env.return_result(std::move(cmd_result), std::move(cmd));
    }
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

TEN_CPP_REGISTER_ADDON_AS_EXTENSION(schema_set_extension_prop__test_extension,
                                    test_extension);

}  // namespace

TEST(SchemaTest, SetExtensionProperty) {  // NOLINT
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
                "name": "test_extension_group",
                "addon": "default_extension_group",
                "app": "msgpack://127.0.0.1:8001/"
             },{
                "type": "extension",
                "name": "test_extension",
                "addon": "schema_set_extension_prop__test_extension",
                "extension_group": "test_extension_group",
                "app": "msgpack://127.0.0.1:8001/"
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
               "extension_group": "test_extension_group",
               "extension": "test_extension"
             }]
           }
         })"_json);
  ten_test::check_result_is(resp, "137", TEN_STATUS_CODE_OK, "world");

  delete client;

  ten_thread_join(app_thread, -1);
}
