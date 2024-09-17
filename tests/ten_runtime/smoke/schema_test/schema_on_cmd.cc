//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include <nlohmann/json.hpp>
#include <string>

#include "gtest/gtest.h"
#include "include_internal/ten_runtime/binding/cpp/internal/ten_env_internal_accessor.h"
#include "include_internal/ten_runtime/binding/cpp/ten.h"
#include "ten_runtime/common/status_code.h"
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
      bool rc = cmd->set_property("foo", "abc");
      ASSERT_EQ(rc, true);

      rc = cmd->set_property("bar", 1232);
      ASSERT_EQ(rc, true);

      rc = ten_env.send_cmd(std::move(cmd));
      ASSERT_EQ(rc, true);

      return;
    }
  }
};

class test_extension_2 : public ten::extension_t {
 public:
  explicit test_extension_2(const std::string &name) : ten::extension_t(name) {}

  void on_init(ten::ten_env_t &ten_env) override {
    ten::ten_env_internal_accessor_t ten_env_internal_accessor(&ten_env);
    bool rc = ten_env_internal_accessor.init_manifest_from_json(
        "invalid json string");
    ASSERT_EQ(rc, false);

    rc = ten_env_internal_accessor.init_manifest_from_json(
        // clang-format off
                 R"({
                      "type": "extension",
                      "name": "schema_on_cmd__test_extension_2",
                      "version": "0.1.0",
                                          "api": {
                        "cmd_in": [
                          {
                            "name": "hello_world",
                            "property": {
                              "foo": {
                                "type": "string"
                              },
                              "bar": {
                                "type": "int8"
                              }
                            }
                          }
                        ]
                      }
                    })"
        // clang-format on
    );
    ASSERT_EQ(rc, true);

    ten_env.on_init_done();
  }

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    // This function will never be called because the cmd does not match the
    // schema.

    FAIL() << "This control path should never be touched!";

    if (std::string(cmd->get_name()) == "hello_world") {
      auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
      cmd_result->set_property("detail", "hello world, too");
      ten_env.return_result(std::move(cmd_result), std::move(cmd));
    }
  }
};

class test_app : public ten::app_t {
 public:
  void on_init(ten::ten_env_t &ten_env) override {
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

    ten_env.on_init_done();
  }
};

void *test_app_thread_main(TEN_UNUSED void *args) {
  auto *app = new test_app();
  app->run();
  delete app;

  return nullptr;
}

TEN_CPP_REGISTER_ADDON_AS_EXTENSION(schema_on_cmd__test_extension_1,
                                    test_extension_1);
TEN_CPP_REGISTER_ADDON_AS_EXTENSION(schema_on_cmd__test_extension_2,
                                    test_extension_2);

}  // namespace

TEST(SchemaTest, OnCmd) {  // NOLINT
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
               "name": "basic_extension_group",
               "addon": "default_extension_group",
               "app": "msgpack://127.0.0.1:8001/"
             },{
                "type": "extension",
                "name": "test extension 1",
                "addon": "schema_on_cmd__test_extension_1",
                "extension_group": "basic_extension_group",
                "app": "msgpack://127.0.0.1:8001/"
             },{
                "type": "extension",
                "name": "test extension 2",
                "addon": "schema_on_cmd__test_extension_2",
                "extension_group": "basic_extension_group",
                "app": "msgpack://127.0.0.1:8001/"
             }],
             "connections": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "basic_extension_group",
               "extension": "test extension 1",
               "cmd": [{
                 "name": "hello_world",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "basic_extension_group",
                   "extension": "test extension 2"
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
               "extension_group": "basic_extension_group",
               "extension": "test extension 1"
             }]
           }
         })"_json);

  // The cmd does not match the schema defined in `test_extension_2`, so we will
  // receive an error result.
  ten_test::check_status_code_is(resp, TEN_STATUS_CODE_ERROR);

  delete client;

  ten_thread_join(app_thread, -1);
}
