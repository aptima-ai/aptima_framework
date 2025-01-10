//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <nlohmann/json.hpp>
#include <string>

#include "gtest/gtest.h"
#include "include_internal/aptima_runtime/binding/cpp/aptima.h"
#include "aptima_runtime/common/status_code.h"
#include "aptima_utils/lib/thread.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/aptima_runtime/smoke/util/binding/cpp/check.h"

// This is a simple example, used to demonstrate that the flow between
// extensions is _not_ specified by a graph, but is explicitly defined by the
// extensions through coding.
//
// In this example, there are 3 extensions:
//
// * 1 "business" extension
// * 1 "plugin_1" extension
// * 1 "plugin_2" extension
//
// The "business" extension will serve as the central node, connecting the
// "plugin_1" extension and the "plugin_2" extension, to complete the task
// delivered by the client.
//
// client <──> business_extension <──> plugin_1
//                                <──> plugin_2
//
// The process is as follows: When the "business" extension receives the initial
// request, it first asks "plugin_1" to do something. After "plugin_1" has
// finished, it then asks "plugin_2" to do something. After "plugin_2" has also
// finished, it returns the result to the "client".

namespace {

class business_extension : public aptima::extension_t {
 public:
  explicit business_extension(const char *name) : aptima::extension_t(name) {}

  void on_cmd(aptima::aptima_env_t &aptima_env,
              std::unique_ptr<aptima::cmd_t> cmd) override {
    // Check whether the initial request has been received and start processing
    // it.
    if (cmd->get_name() == "initial_request") {
      // Start the handling process of "initial_request".
      handle_initial_request(aptima_env, std::move(cmd));
    }
  }

 private:
  void handle_initial_request(aptima::aptima_env_t &aptima_env,
                              std::unique_ptr<aptima::cmd_t> cmd) {
    // The 1st step is to interact with "plugin_1".
    send_cmd_to_plugin_1(aptima_env, std::move(cmd));
  }

  void send_cmd_to_plugin_1(aptima::aptima_env_t &aptima_env,
                            std::unique_ptr<aptima::cmd_t> cmd) {
    // Construct a command for plugin_1.
    auto cmd_to_plugin_1 = aptima::cmd_t::create("plugin_1_cmd");

    // Explicitly specify to interact with plugin_1.
    cmd_to_plugin_1->set_dest("localhost", "default", "specify_dest_group",
                              "plugin_extension_1");

    // This is the method to pass unique_ptr through the lambda capture list in
    // C++11. If you can use C++14 or even C++17, the syntax can be simpler, but
    // the meaning remains the same.
    auto cmd_shared =
        std::make_shared<std::unique_ptr<aptima::cmd_t>>(std::move(cmd));

    aptima_env.send_cmd(
        std::move(cmd_to_plugin_1),
        [this, cmd_shared](aptima::aptima_env_t &aptima_env,
                           std::unique_ptr<aptima::cmd_result_t> cmd_result,
                           aptima::error_t *err) {
          // Receive the result from plugin_1, and decide the next step based
          // on the content of the result.
          nlohmann::json json =
              nlohmann::json::parse(cmd_result->get_property_to_json());
          if (json["detail"] == "plugin_1_result") {
            // Successfully completed the interaction with plugin_1, the next
            // step is to interact with plugin_2.
            send_cmd_to_plugin_2(aptima_env, std::move(*cmd_shared));
          }
        });
  }

  void send_cmd_to_plugin_2(aptima::aptima_env_t &aptima_env,
                            std::unique_ptr<aptima::cmd_t> cmd) {
    // Construct a command for plugin_2.
    auto cmd_to_plugin_2 = aptima::cmd_t::create("plugin_2_cmd");

    // Explicitly specify to interact with plugin_2.
    cmd_to_plugin_2->set_dest("localhost", "default", "specify_dest_group",
                              "plugin_extension_2");

    // This is the method to pass unique_ptr through the lambda capture list in
    // C++11. If you can use C++14 or even C++17, the syntax can be simpler, but
    // the meaning remains the same.
    auto cmd_shared =
        std::make_shared<std::unique_ptr<aptima::cmd_t>>(std::move(cmd));

    aptima_env.send_cmd(
        std::move(cmd_to_plugin_2),
        [cmd_shared](aptima::aptima_env_t &aptima_env,
                     std::unique_ptr<aptima::cmd_result_t> cmd_result,
                     aptima::error_t *err) {
          // Receive result from plugin_2.
          nlohmann::json json =
              nlohmann::json::parse(cmd_result->get_property_to_json());
          if (json["detail"] == "plugin_2_result") {
            // Successfully completed the interaction with plugin_2,
            // the next step is to return a result to the request
            // submitter (i.e., the client).
            auto cmd_result = aptima::cmd_result_t::create(aptima_STATUS_CODE_OK);
            cmd_result->set_property("detail", "success");
            aptima_env.return_result(std::move(cmd_result),
                                  std::move(*cmd_shared));
          }
        });
  }
};

class plugin_extension_1 : public aptima::extension_t {
 public:
  explicit plugin_extension_1(const char *name) : aptima::extension_t(name) {}

  void on_cmd(aptima::aptima_env_t &aptima_env,
              std::unique_ptr<aptima::cmd_t> cmd) override {
    // Simulate the action of receiving a command, and return a result.
    if (cmd->get_name() == "plugin_1_cmd") {
      auto cmd_result = aptima::cmd_result_t::create(aptima_STATUS_CODE_OK);
      cmd_result->set_property("detail", "plugin_1_result");
      aptima_env.return_result(std::move(cmd_result), std::move(cmd));
    }
  }
};

class plugin_extension_2 : public aptima::extension_t {
 public:
  explicit plugin_extension_2(const char *name) : aptima::extension_t(name) {}

  void on_cmd(aptima::aptima_env_t &aptima_env,
              std::unique_ptr<aptima::cmd_t> cmd) override {
    // Simulate the action of receiving a command, and return a result.
    if (cmd->get_name() == "plugin_2_cmd") {
      auto cmd_result = aptima::cmd_result_t::create(aptima_STATUS_CODE_OK);
      cmd_result->set_property("detail", "plugin_2_result");
      aptima_env.return_result(std::move(cmd_result), std::move(cmd));
    }
  }
};

class business_app : public aptima::app_t {
 public:
  void on_configure(aptima::aptima_env_t &aptima_env) override {
    // Note that the graph is very simple. It does _not_ contain any workflows,
    // just what the extensions contained in the graph.
    //
    // The overall logic contains 3 extensions:
    // * "business" extension
    // * "plugin_1" extension
    // * "plugin_2" extension
    //
    // The graph merely describes which extensions are included, nothing more.
    // It does not contain any logic for interaction; all logic for interaction
    // is written in the code.

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
                          "auto_start": true,
                          "singleton": true,
                          "nodes": [{
                            "type": "extension",
                            "name": "business_extension",
                            "addon": "specify_dest__business_extension",
                            "extension_group": "specify_dest_group"
                          },{
                            "type": "extension",
                            "name": "plugin_extension_1",
                            "addon": "specify_dest__plugin_extension_1",
                            "extension_group": "specify_dest_group"
                          },{
                            "type": "extension",
                            "name": "plugin_extension_2",
                            "addon": "specify_dest__plugin_extension_2",
                            "extension_group": "specify_dest_group"
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

void *business_app_thread_main(aptima_UNUSED void *args) {
  auto *app = new business_app();
  app->run();
  delete app;

  return nullptr;
}

aptima_CPP_REGISTER_ADDON_AS_EXTENSION(specify_dest__business_extension,
                                    business_extension);
aptima_CPP_REGISTER_ADDON_AS_EXTENSION(specify_dest__plugin_extension_1,
                                    plugin_extension_1);
aptima_CPP_REGISTER_ADDON_AS_EXTENSION(specify_dest__plugin_extension_2,
                                    plugin_extension_2);

}  // namespace

TEST(ExtensionTest, SpecifyDest) {  // NOLINT
  // Start the app.
  auto *app_thread = aptima_thread_create("business app thread",
                                       business_app_thread_main, nullptr);

  // Create a client and connect to the app.
  auto *client = new aptima::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

  // Send the "initial_request" to the "business extension".
  auto initial_request_cmd = aptima::cmd_t::create("initial_request");
  initial_request_cmd->set_dest("msgpack://127.0.0.1:8001/", "default",
                                "specify_dest_group", "business_extension");
  auto cmd_result =
      client->send_cmd_and_recv_result(std::move(initial_request_cmd));

  // Check whether the correct result has been received.
  aptima_test::check_status_code(cmd_result, aptima_STATUS_CODE_OK);
  aptima_test::check_detail_with_string(cmd_result, "success");

  delete client;

  aptima_thread_join(app_thread, -1);
}
