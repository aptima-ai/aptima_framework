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

class test_normal_extension : public ten::extension_t {
 public:
  explicit test_normal_extension(const std::string &name)
      : ten::extension_t(name) {}

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    nlohmann::json json = nlohmann::json::parse(cmd->to_json());
    if (json["_ten"]["name"] == "hello_world") {
      auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
      cmd_result->set_property("detail", "hello world, too");
      ten_env.return_result(std::move(cmd_result), std::move(cmd));
    }
  }
};

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
        [this, start_graph_json](ten::ten_env_t &ten_env,
                                 std::unique_ptr<ten::cmd_result_t> cmd) {
          auto status_code = cmd->get_status_code();
          ASSERT_EQ(status_code, TEN_STATUS_CODE_OK);

          auto graph_id = cmd->get_property_string("detail");

          nlohmann::json hello_cmd =
              R"({
                   "_ten": {
                     "name": "hello_world",
                     "seq_id": "137",
                     "dest":[{
                       "app": "msgpack://127.0.0.1:8001/",
                       "extension_group": "start_predefined_graph_from_extension__normal_extension_group",
                       "extension": "normal_extension"
                     }]
                   }
                 })"_json;
          hello_cmd["_ten"]["dest"][0]["graph"] = graph_id;

          ten_env.send_json(
              hello_cmd.dump().c_str(),
              [this](ten::ten_env_t &ten_env,
                     std::unique_ptr<ten::cmd_result_t> cmd) {
                received_hello_world_resp = true;

                if (test_cmd != nullptr) {
                  nlohmann::json detail = {{"id", 1}, {"name", "a"}};

                  auto cmd_result =
                      ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
                  cmd_result->set_property_from_json("detail",
                                                     detail.dump().c_str());
                  ten_env.return_result(std::move(cmd_result),
                                        std::move(test_cmd));
                }
              });
        });

    ten_env.on_start_done();
  }

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    nlohmann::json json = nlohmann::json::parse(cmd->to_json());
    if (json["_ten"]["name"] == "test") {
      if (received_hello_world_resp) {
        nlohmann::json detail = {{"id", 1}, {"name", "a"}};

        auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
        cmd_result->set_property_from_json("detail", detail.dump().c_str());
        ten_env.return_result(std::move(cmd_result), std::move(cmd));
      } else {
        test_cmd = std::move(cmd);
        return;
      }
    } else {
      TEN_ASSERT(0, "Should not happen.");
    }
  }

 private:
  bool received_hello_world_resp{};
  std::unique_ptr<ten::cmd_t> test_cmd;
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
                 R"({
                      "_ten": {
                        "uri": "msgpack://127.0.0.1:8001/",
                        "log_level": 2,
                        "predefined_graphs": [{
                          "name": "default",
                          "auto_start": false,
                          "singleton": true,
                          "nodes": [{
                            "type": "extension",
                            "name": "predefined_graph",
                            "addon": "start_predefined_graph_from_extension__predefined_graph_extension",
                            "extension_group": "start_predefined_graph_from_extension__predefined_graph_group"
                          }]
                        },{
                          "name": "graph_1",
                          "auto_start": false,
                          "nodes": [{
                            "type": "extension",
                            "name": "normal_extension",
                            "addon": "start_predefined_graph_from_extension__normal_extension",
                            "extension_group": "start_predefined_graph_from_extension__normal_extension_group"
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

void *app_thread_main(TEN_UNUSED void *args) {
  auto *app = new test_app();
  app->run();
  delete app;

  return nullptr;
}

TEN_CPP_REGISTER_ADDON_AS_EXTENSION(
    start_predefined_graph_from_extension__predefined_graph_extension,
    test_predefined_graph);
TEN_CPP_REGISTER_ADDON_AS_EXTENSION(
    start_predefined_graph_from_extension__normal_extension,
    test_normal_extension);

}  // namespace

TEST(ExtensionTest, StartPredefinedGraphFromExtension) {  // NOLINT
  auto *app_thread = ten_thread_create("app thread", app_thread_main, nullptr);

  // Create a client and connect to the app.
  auto *client = new ten::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

  // Do not need to send 'start_graph' command first.
  // The 'graph_id' MUST be "default" (a special string) if we want to send the
  // request to predefined graph.
  nlohmann::json resp = client->send_json_and_recv_resp_in_json(
      R"({
           "_ten": {
             "name": "test",
             "seq_id": "111",
             "dest": [{
               "app": "msgpack://127.0.0.1:8001/",
               "graph": "default",
               "extension_group": "start_predefined_graph_from_extension__predefined_graph_group",
               "extension": "predefined_graph"
             }]
           }
         })"_json);
  ten_test::check_result_is(resp, "111", TEN_STATUS_CODE_OK,
                            R"({"id": 1, "name": "a"})");

  delete client;

  ten_thread_join(app_thread, -1);
}
