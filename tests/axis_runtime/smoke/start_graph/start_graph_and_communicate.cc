//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "gtest/gtest.h"
#include "include_internal/aptima_runtime/binding/cpp/aptima.h"
#include "aptima_runtime/binding/cpp/detail/msg/cmd/stop_graph.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/aptima_runtime/smoke/util/binding/cpp/check.h"

namespace {

class test_normal_extension : public aptima::extension_t {
 public:
  explicit test_normal_extension(const char *name) : aptima::extension_t(name) {}

  void on_cmd(aptima::aptima_env_t &aptima_env,
              std::unique_ptr<aptima::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      auto cmd_result = aptima::cmd_result_t::create(aptima_STATUS_CODE_OK);
      cmd_result->set_property("detail", "hello world, too");
      aptima_env.return_result(std::move(cmd_result), std::move(cmd));
    }
  }
};

class test_predefined_graph : public aptima::extension_t {
 public:
  explicit test_predefined_graph(const char *name) : aptima::extension_t(name) {}

  void on_start(aptima::aptima_env_t &aptima_env) override {
    auto start_graph_cmd = aptima::cmd_start_graph_t::create();
    start_graph_cmd->set_dest("localhost", nullptr, nullptr, nullptr);
    start_graph_cmd->set_graph_from_json(R"({
      "nodes": [{
        "type": "extension",
        "name": "normal_extension",
        "addon": "start_graph_and_communication__normal_extension",
        "app": "msgpack://127.0.0.1:8001/",
        "extension_group": "start_graph_and_communication__normal_extension_group"
      }]
    })"_json.dump()
                                             .c_str());

    aptima_env.send_cmd(
        std::move(start_graph_cmd),
        [this](aptima::aptima_env_t &aptima_env, std::unique_ptr<aptima::cmd_result_t> cmd,
               aptima::error_t *err) {
          auto graph_id = cmd->get_property_string("detail");

          auto hello_world_cmd = aptima::cmd_t::create("hello_world");
          hello_world_cmd->set_dest(
              "msgpack://127.0.0.1:8001/", graph_id.c_str(),
              "start_graph_and_communication__normal_extension_group",
              "normal_extension");
          aptima_env.send_cmd(
              std::move(hello_world_cmd),
              [this, graph_id](aptima::aptima_env_t &aptima_env,
                               std::unique_ptr<aptima::cmd_result_t> cmd,
                               aptima::error_t *err) {
                // Shut down the graph; otherwise, the app won't be able to
                // close because there is still a running engine/graph.
                auto stop_graph_cmd = aptima::cmd_stop_graph_t::create();
                stop_graph_cmd->set_dest("localhost", nullptr, nullptr,
                                         nullptr);
                stop_graph_cmd->set_graph_id(graph_id.c_str());

                aptima_env.send_cmd(
                    std::move(stop_graph_cmd),
                    [this](aptima::aptima_env_t &aptima_env,
                           std::unique_ptr<aptima::cmd_result_t> cmd,
                           aptima::error_t *err) {
                      received_hello_world_resp = true;

                      if (test_cmd != nullptr) {
                        nlohmann::json detail = {{"id", 1}, {"name", "a"}};

                        auto cmd_result =
                            aptima::cmd_result_t::create(aptima_STATUS_CODE_OK);
                        cmd_result->set_property_from_json(
                            "detail", detail.dump().c_str());
                        aptima_env.return_result(std::move(cmd_result),
                                              std::move(test_cmd));
                      }
                    });
              });
        });

    aptima_env.on_start_done();
  }

  void on_cmd(aptima::aptima_env_t &aptima_env,
              std::unique_ptr<aptima::cmd_t> cmd) override {
    if (cmd->get_name() == "test") {
      if (received_hello_world_resp) {
        nlohmann::json detail = {{"id", 1}, {"name", "a"}};

        auto cmd_result = aptima::cmd_result_t::create(aptima_STATUS_CODE_OK);
        cmd_result->set_property_from_json("detail", detail.dump().c_str());
        aptima_env.return_result(std::move(cmd_result), std::move(cmd));
      } else {
        test_cmd = std::move(cmd);
        return;
      }
    } else {
      aptima_ASSERT(0, "Should not happen.");
    }
  }

 private:
  bool received_hello_world_resp{};
  std::unique_ptr<aptima::cmd_t> test_cmd;
};

class test_app : public aptima::app_t {
 public:
  void on_configure(aptima::aptima_env_t &aptima_env) override {
    bool rc = aptima::aptima_env_internal_accessor_t::init_manifest_from_json(
        aptima_env,
        // clang-format off
                 R"({
                      "type": "app",
                      "name": "test_app",
                      "version": "0.1.0"
                    })"
        // clang-format on
    );
    ASSERT_EQ(rc, true);

    rc = aptima_env.init_property_from_json(
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
                            "addon": "start_graph_and_communication__predefined_graph_extension",
                            "extension_group": "start_graph_and_communication__predefined_graph_group"
                          }]
                        }]
                      }
                    })"
        // clang-format on
    );
    ASSERT_EQ(rc, true);

    aptima_env.on_configure_done();
  }
};

void *app_thread_main(aptima_UNUSED void *args) {
  auto *app = new test_app();
  app->run();
  delete app;

  return nullptr;
}

aptima_CPP_REGISTER_ADDON_AS_EXTENSION(
    start_graph_and_communication__predefined_graph_extension,
    test_predefined_graph);
aptima_CPP_REGISTER_ADDON_AS_EXTENSION(
    start_graph_and_communication__normal_extension, test_normal_extension);

}  // namespace

TEST(ExtensionTest, StartGraphAndCommunication) {  // NOLINT
  auto *app_thread = aptima_thread_create("app thread", app_thread_main, nullptr);

  // Create a client and connect to the app.
  auto *client = new aptima::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

  // Do not need to send 'start_graph' command first.
  // The 'graph_id' MUST be "default" (a special string) if we want to send the
  // request to predefined graph.
  auto test_cmd = aptima::cmd_t::create("test");
  test_cmd->set_dest("msgpack://127.0.0.1:8001/", "default",
                     "start_graph_and_communication__predefined_graph_group",
                     "predefined_graph");
  auto cmd_result = client->send_cmd_and_recv_result(std::move(test_cmd));
  aptima_test::check_status_code(cmd_result, aptima_STATUS_CODE_OK);
  aptima_test::check_detail_with_json(cmd_result, R"({"id": 1, "name": "a"})");

  delete client;

  aptima_thread_join(app_thread, -1);
}
