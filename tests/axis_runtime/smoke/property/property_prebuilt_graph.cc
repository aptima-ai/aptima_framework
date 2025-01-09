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
#include "aptima_utils/lang/cpp/lib/value.h"
#include "aptima_utils/lib/thread.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/aptima_runtime/smoke/util/binding/cpp/check.h"

namespace {

class test_extension : public ten::extension_t {
 public:
  explicit test_extension(const char *name) : ten::extension_t(name) {}

  void on_cmd(ten::aptima_env_t &aptima_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    auto mode = aptima_env.get_property_string("widget_props");
    if (mode.empty()) {
      mode = "default";
    }

    if (cmd->get_name() == "hello_world") {
      auto cmd_result = ten::cmd_result_t::create(aptima_STATUS_CODE_OK);
      cmd_result->set_property("detail", mode);
      aptima_env.return_result(std::move(cmd_result), std::move(cmd));
    }
  }
};

class test_app : public ten::app_t {
 public:
  void on_configure(ten::aptima_env_t &aptima_env) override {
    bool rc = ten::aptima_env_internal_accessor_t::init_manifest_from_json(
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
                 R"###({
                      "_ten": {
                        "uri": "msgpack://127.0.0.1:8001/",
                        "log_level": 2,
                        "predefined_graphs": [{
                           "name": "default",
                           "auto_start": true,
                           "singleton": true,
                           "nodes": [{
                             "type": "extension",
                             "name": "property_predefined_graph",
                             "addon": "property_predefined_graph__extension",
                             "extension_group": "property_predefined_graph",
                             "property": {
                               "widget_props": "1"
                             }
                           },{
                             "type": "extension",
                             "name": "property_predefined_graph_no_prop",
                             "addon": "property_predefined_graph__extension",
                             "extension_group": "property_predefined_graph"
                           }]
                         }]
                        }
                    })###"
        // clang-format on
    );
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

aptima_CPP_REGISTER_ADDON_AS_EXTENSION(property_predefined_graph__extension,
                                    test_extension);

}  // namespace

TEST(PropertyTest, PredefinedGraph) {  // NOLINT
  // Start app.
  auto *app_thread =
      aptima_thread_create("app thread", test_app_thread_main, nullptr);

  // Create a client and connect to the app.
  auto *client = new ten::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

  // Send a user-defined 'hello world' command.
  auto hello_world_cmd = ten::cmd_t::create("hello_world");
  hello_world_cmd->set_dest("msgpack://127.0.0.1:8001/", "default",
                            "property_predefined_graph",
                            "property_predefined_graph");
  auto cmd_result =
      client->send_cmd_and_recv_result(std::move(hello_world_cmd));

  aptima_test::check_status_code(cmd_result, aptima_STATUS_CODE_OK);
  aptima_test::check_detail_with_string(cmd_result, "1");
  hello_world_cmd = ten::cmd_t::create("hello_world");
  hello_world_cmd->set_dest("msgpack://127.0.0.1:8001/", "default",
                            "property_predefined_graph",
                            "property_predefined_graph_no_prop");
  cmd_result = client->send_cmd_and_recv_result(std::move(hello_world_cmd));

  aptima_test::check_status_code(cmd_result, aptima_STATUS_CODE_OK);
  aptima_test::check_detail_with_string(cmd_result, "default");

  // Destroy the client.
  delete client;

  aptima_thread_join(app_thread, -1);
}
