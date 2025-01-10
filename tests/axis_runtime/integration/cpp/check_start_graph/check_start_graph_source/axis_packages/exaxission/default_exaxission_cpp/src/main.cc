//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <cassert>
#include <cstdlib>
#include <iostream>

#include "aptima_runtime/binding/cpp/detail/msg/cmd/close_app.h"
#include "aptima_runtime/binding/cpp/detail/msg/cmd/start_graph.h"
#include "aptima_runtime/binding/cpp/detail/aptima_env.h"
#include "aptima_runtime/binding/cpp/aptima.h"

bool started = false;

class test_extension : public aptima::extension_t {
 public:
  explicit test_extension(const char *name) : aptima::extension_t(name) {}

  void on_init(aptima::aptima_env_t &aptima_env) override { aptima_env.on_init_done(); }

  // NOLINTNEXTLINE
  void send_invalid_graph(aptima::aptima_env_t &aptima_env) {
    aptima::error_t err;

    auto start_graph_cmd = aptima::cmd_start_graph_t::create();
    start_graph_cmd->set_dest("localhost", nullptr, nullptr, nullptr);
    bool result = start_graph_cmd->set_graph_from_json(R"({
            "nodes": [
              {
                "type": "extension",
                "name": "default_extension_cpp",
                "addon": "default_extension_cpp",
                "extension_group": "default_extension_group"
              },
              {
                "type": "extension",
                "name": "default_extension_cpp_2",
                "addon": "default_extension_cpp",
                "extension_group": "default_extension_group"
              }
            ],
            "connections": [
              {
                "extension": "default_extension_cpp_1",
                "cmd": [
                  {
                    "name": "test",
                    "dest": [
                      {
                        "extension": "default_extension_cpp_2"
                      }
                    ]
                  }
                ]
              }
            ]
        })",
                                                       &err);
    assert(!result && "The graph should be invalid.");
  }

  void on_start(aptima::aptima_env_t &aptima_env) override {
    aptima_env.on_start_done();

    if (!started) {
      started = true;

      send_invalid_graph(aptima_env);

      auto start_graph_cmd = aptima::cmd_start_graph_t::create();
      start_graph_cmd->set_dest("localhost", nullptr, nullptr, nullptr);
      start_graph_cmd->set_graph_from_json(R"({
            "nodes": [
              {
                "type": "extension",
                "name": "default_extension_cpp",
                "addon": "default_extension_cpp",
                "extension_group": "default_extension_group"
              }
            ]
        })");

      aptima_env.send_cmd(
          std::move(start_graph_cmd),
          [](aptima::aptima_env_t &env, std::unique_ptr<aptima::cmd_result_t> result,
             aptima::error_t * /*error*/) {
            // The graph check should be passed.
            if (result->get_status_code() == aptima_STATUS_CODE_OK) {
              auto close_app = aptima::cmd_close_app_t::create();
              close_app->set_dest("localhost", nullptr, nullptr, nullptr);
              env.send_cmd(std::move(close_app));
            } else {
              std::cout << "Failed to start graph: "
                        << result->get_property_string("detail") << "\n";

              // NOLINTNEXTLINE(concurrency-mt-unsafe)
              std::exit(EXIT_FAILURE);
            }
          });
    }
  }
};

aptima_CPP_REGISTER_ADDON_AS_EXTENSION(default_extension_cpp, test_extension);
