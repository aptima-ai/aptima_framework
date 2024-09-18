//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "gtest/gtest.h"
#include "include_internal/ten_runtime/binding/cpp/ten.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/ten_runtime/smoke/extension_test/util/binding/cpp/check.h"

namespace {

class test_property_access_app_store_async_1 : public ten::extension_t {
 public:
  explicit test_property_access_app_store_async_1(const std::string &name)
      : ten::extension_t(name) {}
};

class test_property_access_app_store_async_2 : public ten::extension_t {
 public:
  explicit test_property_access_app_store_async_2(const std::string &name)
      : ten::extension_t(name) {}

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    auto cmd_shared =
        std::make_shared<std::unique_ptr<ten::cmd_t>>(std::move(cmd));
    ten_env.get_property_int32_async(
        "app:aaa", [cmd_shared](ten::ten_env_t &ten_env, int32_t result,
                                TEN_UNUSED ten::error_t *err) {
          if (result == 3) {
            auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
            cmd_result->set_property("detail", "success");
            ten_env.return_result(std::move(cmd_result),
                                  std::move(*cmd_shared));
          }
        });
  }
};

class test_app : public ten::app_t {
 public:
  void on_configure(ten::ten_env_t &ten_env) override {
    ten::ten_env_internal_accessor_t ten_env_internal_accessor(&ten_env);
    bool rc = ten_env_internal_accessor.init_manifest_from_json(
        // clang-format off
                 R"({
                     "type": "app",
                     "name": "test_app",
                     "version": "0.1.0"
                    })"
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
                            "name": "default_extension_group_1",
                            "addon": "default_extension_group"
                          },{
                            "type": "extension_group",
                            "name": "default_extension_group_2",
                            "addon": "default_extension_group"
                          },{
                            "type": "extension",
                            "name": "test_property_access_app_store_async_1",
                            "addon": "test_property_access_app_store_async_1",
                            "extension_group": "default_extension_group_1"
                          },{
                            "type": "extension",
                            "name": "test_property_access_app_store_async_2",
                            "addon": "test_property_access_app_store_async_2",
                            "extension_group": "default_extension_group_2"
                          }]
                        }]
                      },
                      "aaa": 3
                    })###"
        // clang-format on
    );
    ASSERT_EQ(rc, true);

    ten_env.on_configure_done();
  }
};

void *app_thread_main(TEN_UNUSED void *args) {
  auto *app = new test_app();
  app->run();
  delete app;

  return nullptr;
}

TEN_CPP_REGISTER_ADDON_AS_EXTENSION(test_property_access_app_store_async_1,
                                    test_property_access_app_store_async_1);
TEN_CPP_REGISTER_ADDON_AS_EXTENSION(test_property_access_app_store_async_2,
                                    test_property_access_app_store_async_2);

}  // namespace

TEST(ExtensionTest, PropertyAccessAppStoreAsync) {  // NOLINT
  auto *app_thread = ten_thread_create("app thread", app_thread_main, nullptr);

  // Create a client and connect to the app.
  auto *client = new ten::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

  // Do not need to send 'start_graph' command first.
  // The 'graph_name' MUST be "0" (a special string) if we want to send the
  // request to predefined graph.
  nlohmann::json resp = client->send_json_and_recv_resp_in_json(
      R"({
         "_ten": {
           "name": "test",
           "seq_id": "111",
           "dest": [{
             "app": "msgpack://127.0.0.1:8001/",
             "graph": "0",
             "extension_group": "default_extension_group_2",
             "extension": "test_property_access_app_store_async_2"
           }]
         }
       })"_json);
  ten_test::check_result_is(resp, "111", TEN_STATUS_CODE_OK, "success");

  delete client;

  ten_thread_join(app_thread, -1);
}
