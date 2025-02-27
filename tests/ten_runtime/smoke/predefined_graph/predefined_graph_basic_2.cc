//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "gtest/gtest.h"
#include "include_internal/ten_runtime/binding/cpp/aptima.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/ten_runtime/smoke/util/binding/cpp/check.h"

namespace {

class test_normal_extension : public aptima::extension_t {
 public:
  explicit test_normal_extension(const char *name) : aptima::extension_t(name) {}

  void on_cmd(aptima::ten_env_t &ten_env,
              std::unique_ptr<aptima::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      auto cmd_result = aptima::cmd_result_t::create(TEN_STATUS_CODE_OK);
      cmd_result->set_property("detail", "hello world, too");
      ten_env.return_result(std::move(cmd_result), std::move(cmd));
    }
  }
};

class test_predefined_graph : public aptima::extension_t {
 public:
  explicit test_predefined_graph(const char *name) : aptima::extension_t(name) {}

  void on_start(aptima::ten_env_t &ten_env) override {
    auto start_graph_cmd = aptima::cmd_start_graph_t::create();
    start_graph_cmd->set_dest("msgpack://127.0.0.1:8001/", nullptr, nullptr,
                              nullptr);
    start_graph_cmd->set_graph_from_json(R"({
      "nodes": [{
        "type": "extension",
        "name": "normal_extension",
        "addon": "predefined_graph_basic_2__normal_extension",
        "app": "msgpack://127.0.0.1:8001/",
        "extension_group": "normal_extension_group"
      }]
    })"_json.dump()
                                             .c_str());

    ten_env.send_cmd(
        std::move(start_graph_cmd),
        [&](aptima::ten_env_t &ten_env,
            std::unique_ptr<aptima::cmd_result_t> cmd_result, aptima::error_t *err) {
          nlohmann::json json =
              nlohmann::json::parse(cmd_result->get_property_to_json());
          if (cmd_result->get_status_code() == TEN_STATUS_CODE_OK) {
            auto hello_world_cmd = aptima::cmd_t::create("hello_world");
            hello_world_cmd->set_dest("msgpack://127.0.0.1:8001/",
                                      json["detail"].get<std::string>().c_str(),
                                      "normal_extension_group",
                                      "normal_extension");
            ten_env.send_cmd(
                std::move(hello_world_cmd),
                [&](aptima::ten_env_t &ten_env,
                    std::unique_ptr<aptima::cmd_result_t> cmd_result,
                    aptima::error_t *err) {
                  nlohmann::json json =
                      nlohmann::json::parse(cmd_result->get_property_to_json());
                  if (cmd_result->get_status_code() == TEN_STATUS_CODE_OK) {
                    normal_extension_is_ready = true;

                    if (command_1 != nullptr) {
                      nlohmann::json const detail =
                          R"({"id": 1, "name": "a"})"_json;
                      auto cmd_result =
                          aptima::cmd_result_t::create(TEN_STATUS_CODE_OK);
                      cmd_result->set_property_from_json("detail",
                                                         detail.dump().c_str());
                      ten_env.return_result(std::move(cmd_result),
                                            std::move(command_1));
                    }
                  }
                });
          }
        });

    ten_env.on_start_done();
  }

  void on_cmd(aptima::ten_env_t &ten_env,
              std::unique_ptr<aptima::cmd_t> cmd) override {
    if (cmd->get_name() == "command_1") {
      if (normal_extension_is_ready) {
        auto detail = R"({"id": 1, "name": "a"})"_json;
        auto cmd_result = aptima::cmd_result_t::create(TEN_STATUS_CODE_OK);
        cmd_result->set_property_from_json("detail", detail.dump().c_str());
        ten_env.return_result(std::move(cmd_result), std::move(cmd));
      } else {
        command_1 = std::move(cmd);
        return;
      }
    }
  }

 private:
  bool normal_extension_is_ready{};
  std::unique_ptr<aptima::cmd_t> command_1;
};

class test_app : public aptima::app_t {
 public:
  void on_configure(aptima::ten_env_t &ten_env) override {
    bool rc = aptima::ten_env_internal_accessor_t::init_manifest_from_json(
        ten_env,
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
                            "addon": "predefined_graph_basic_2__predefined_graph",
                            "extension_group": "predefined_graph_group"
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

TEN_CPP_REGISTER_ADDON_AS_EXTENSION(predefined_graph_basic_2__predefined_graph,
                                    test_predefined_graph);
TEN_CPP_REGISTER_ADDON_AS_EXTENSION(predefined_graph_basic_2__normal_extension,
                                    test_normal_extension);

}  // namespace

TEST(ExtensionTest, PredefinedGraphBasic2) {  // NOLINT
  auto *app_thread = ten_thread_create("app thread", app_thread_main, nullptr);

  // Create a client and connect to the app.
  auto *client = new aptima::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

  // Do not need to send 'start_graph' command first.
  // The 'graph_id' MUST be "default" (a special string) if we want to send the
  // request to predefined graph.
  auto command_1_cmd = aptima::cmd_t::create("command_1");
  command_1_cmd->set_dest("msgpack://127.0.0.1:8001/", "default",
                          "predefined_graph_group", "predefined_graph");
  auto cmd_result = client->send_cmd_and_recv_result(std::move(command_1_cmd));
  ten_test::check_status_code(cmd_result, TEN_STATUS_CODE_OK);
  ten_test::check_detail_with_json(cmd_result, R"({"id": 1, "name": "a"})");

  delete client;

  ten_thread_join(app_thread, -1);
}
