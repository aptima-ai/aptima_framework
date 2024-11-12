//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "gtest/gtest.h"
#include "include_internal/ten_runtime/binding/cpp/ten.h"
#include "ten_runtime/common/status_code.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/ten_runtime/smoke/extension_test/util/binding/cpp/check.h"

namespace {
class test_predefined_graph : public ten::extension_t {
 public:
  explicit test_predefined_graph(const std::string &name)
      : ten::extension_t(name) {}

  void on_start(ten::ten_env_t &ten_env) override {
    std::string start_graph_json = R"({
             "_ten": {
               "type": "start_graph",
               "seq_id": "222",
               "dest": [{
                 "app": "localhost"
               }],
               "predefined_graph": "graph_1"
             }
          })"_json.dump();

    ten_env.send_json(
        start_graph_json.c_str(),
        [start_graph_json](ten::ten_env_t &ten_env,
                           std::unique_ptr<ten::cmd_result_t> cmd) {
          auto status_code = cmd->get_status_code();
          ASSERT_EQ(status_code, TEN_STATUS_CODE_ERROR);

          auto detail = cmd->get_property_string("detail");
          ASSERT_EQ(detail, "Failed to connect to msgpack://127.0.0.1:8888/");

          // The app will not be closed because it is running in
          // long_running_mode.

          ten_env.on_start_done();
        });
  }

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    nlohmann::json json = nlohmann::json::parse(cmd->to_json());
    if (json["_ten"]["name"] == "test") {
      nlohmann::json detail = {{"id", 1}, {"name", "a"}};

      auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
      cmd_result->set_property_from_json("detail", detail.dump().c_str());
      ten_env.return_result(std::move(cmd_result), std::move(cmd));
    } else {
      TEN_ASSERT(0, "Should not happen.");
    }
  }
};

class test_app_1 : public ten::app_t {
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
                 R"({
                      "_ten": {
                        "uri": "msgpack://127.0.0.1:8001/",
                        "log_level": 2,
                        "long_running_mode": true,
                        "predefined_graphs": [{
                          "name": "default",
                          "auto_start": false,
                          "singleton": true,
                          "nodes": [{
                            "type": "extension",
                            "name": "predefined_graph",
                            "app": "msgpack://127.0.0.1:8001/",
                            "addon": "failed_to_connect_to_remote__predefined_graph_extension",
                            "extension_group": "failed_to_connect_to_remote__predefined_graph_group"
                          }]
                        },{
                          "name": "graph_1",
                          "auto_start": false,
                          "nodes": [{
                            "type": "extension",
                            "name": "normal_extension_1",
                            "app": "msgpack://127.0.0.1:8001/",
                            "addon": "failed_to_connect_to_remote__normal_extension_1",
                            "extension_group": "failed_to_connect_to_remote__normal_extension_group"
                          }, {
                            "type": "extension",
                            "name": "normal_extension_2",
                            "app": "msgpack://127.0.0.1:8888/",
                            "addon": "failed_to_connect_to_remote__normal_extension_2",
                            "extension_group": "failed_to_connect_to_remote__normal_extension_group"
                          }],
                          "connections": [{
                            "app": "msgpack://127.0.0.1:8001/",
                            "extension_group": "failed_to_connect_to_remote__normal_extension_group",
                            "extension": "normal_extension_1",
                            "cmd": [{
                              "name": "hello_world",
                              "dest": [{
                                "app": "msgpack://127.0.0.1:8888/",
                                "extension_group": "failed_to_connect_to_remote__normal_extension_group",
                                "extension": "normal_extension_2"
                              }]
                            }]
                          }]
                        }]
                      }
                    })"
        // clang-format on
    );
    ASSERT_EQ(rc, true);

    ten_env.on_configure_done();
  }
};

void *app_thread_1_main(TEN_UNUSED void *args) {
  auto *app = new test_app_1();
  app->run();
  delete app;

  return nullptr;
}

TEN_CPP_REGISTER_ADDON_AS_EXTENSION(
    failed_to_connect_to_remote__predefined_graph_extension,
    test_predefined_graph);

}  // namespace

TEST(ExtensionTest, FailedToConnectToRemote) {  // NOLINT
  auto *app_1_thread =
      ten_thread_create("app thread 1", app_thread_1_main, nullptr);

  // Create a client and connect to the app.
  auto *client = new ten::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

  // Do not need to send 'start_graph' command first.
  // The 'graph_id' MUST be "default" if we want to send the request to
  // predefined graph.
  nlohmann::json resp = client->send_json_and_recv_resp_in_json(
      R"({
           "_ten": {
             "name": "test",
             "seq_id": "111",
             "dest": [{
               "app": "msgpack://127.0.0.1:8001/",
               "graph": "default",
               "extension_group": "failed_to_connect_to_remote__predefined_graph_group",
               "extension": "predefined_graph"
             }]
           }
         })"_json);
  ten_test::check_result_is(resp, "111", TEN_STATUS_CODE_OK,
                            R"({"id": 1, "name": "a"})");

  delete client;

  // Send a close_app command to close the app as the app is running in
  // long_running_mode.
  ten::msgpack_tcp_client_t::close_app("msgpack://127.0.0.1:8001/");

  ten_thread_join(app_1_thread, -1);
}
