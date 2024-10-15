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
#include "ten_utils/lang/cpp/lib/value.h"
#include "ten_utils/lib/thread.h"
#include "ten_utils/macro/macros.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/ten_runtime/smoke/extension_test/util/binding/cpp/check.h"

#define APP_PROP_NAME "test_app_prop"
#define EXT_PROP_NAME "test_extension_prop"

#define APP_PROP_VAL 12345
#define EXT_PROP_VAL 98762

namespace {

class test_extension : public ten::extension_t {
 public:
  explicit test_extension(const std::string &name) : ten::extension_t(name) {}

  void on_configure(ten::ten_env_t &ten_env) override {
    // Define the property.
    ten::ten_env_internal_accessor_t ten_env_internal_accessor(&ten_env);
    bool rc = ten_env_internal_accessor.init_manifest_from_json(
        // clang-format off
                 "{\
                   \"type\": \"extension\",\
                   \"name\": \"property_extension__extension\",\
                   \"version\": \"1.0.0\",\
                   \"api\": {\
                     \"property\": {\
                       \"" EXT_PROP_NAME "\": {\
                         \"type\": \"int32\"\
                       }\
                     }\
                   }\
                 }"
        // clang-format on
    );
    ASSERT_EQ(rc, true);

    // Define the default value of the property.
    rc = ten_env.init_property_from_json("{\"" EXT_PROP_NAME
                                         "\":" TEN_XSTR(EXT_PROP_VAL) "}");
    ASSERT_EQ(rc, true);

    ten_env.on_configure_done();
  }

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    nlohmann::json json = nlohmann::json::parse(cmd->to_json());
    if (json["_ten"]["name"] == "hello_world") {
      auto app_property_value =
          ten_env.get_property_int32("app:" APP_PROP_NAME);
      auto extension_property_value = ten_env.get_property_int32(EXT_PROP_NAME);

      if (app_property_value == APP_PROP_VAL &&
          extension_property_value == EXT_PROP_VAL) {
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
        // clang-format off
                 "{\
                    \"type\": \"app\",\
                    \"name\": \"test_app\",\
                    \"version\": \"1.0.0\",\
                    \"api\": {\
                      \"property\": {\
                        \"" APP_PROP_NAME "\": {\
                          \"type\": \"int32\"\
                        }\
                      }\
                    }\
                  }"
        // clang-format on
    );
    ASSERT_EQ(rc, true);

    rc = ten_env.init_property_from_json(
        "{\
                    \"_ten\": {\
                    \"uri\": \"msgpack://127.0.0.1:8001/\"},\
                    \"" APP_PROP_NAME "\":" TEN_XSTR(APP_PROP_VAL) "}");
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

TEN_CPP_REGISTER_ADDON_AS_EXTENSION(property_extension__extension,
                                    test_extension);

}  // namespace

TEST(ExtensionTest, PropertyExtension) {  // NOLINT
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
               "type": "extension",
               "name": "test_extension",
               "addon": "property_extension__extension",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "property_extension__extension_group"
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
             "dest":[{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "property_extension__extension_group",
               "extension": "test_extension"
             }]
           }
         })"_json);
  ten_test::check_result_is(resp, "137", TEN_STATUS_CODE_OK,
                            "hello world, too");

  // Destroy the client.
  delete client;

  ten_thread_join(app_thread, -1);
}
