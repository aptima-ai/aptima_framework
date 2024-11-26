//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <cassert>
#include <cstdlib>
#include <iostream>

#include "ten_runtime/binding/cpp/detail/msg/cmd/close_app.h"
#include "ten_runtime/binding/cpp/detail/msg/cmd/start_graph.h"
#include "ten_runtime/binding/cpp/detail/ten_env.h"
#include "ten_runtime/binding/cpp/ten.h"

bool started = false;

class test_extension : public ten::extension_t {
 public:
  explicit test_extension(const std::string &name) : ten::extension_t(name) {}

  void on_init(ten::ten_env_t &ten_env) override { ten_env.on_init_done(); }

  // NOLINTNEXTLINE
  void send_invalid_graph(ten::ten_env_t &ten_env) {
    ten::error_t err;

    auto start_graph_cmd = ten::cmd_start_graph_t::create();
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
                "extension": "default_extension_cpp",
                "extension_group": "default_extension_group",
                "cmd": [
                  {
                    "name": "test",
                    "dest": [
                      {
                        "extension": "default_extension_cpp_2",
                        "extension_group": "default_extension_group_2"
                      }
                    ]
                  }
                ]
              }
            ]
        })",
                                                       &err);
    assert(!result && "The graph should be invalid.");

    // The extension_info is not found, extension_group:
    // default_extension_group_2, extension: default_extension_cpp_2.
    std::string err_msg = err.errmsg();

    // NOLINTNEXTLINE
    assert(err_msg.find("default_extension_group_2") != std::string::npos &&
           "Incorrect msg.");
  }

  void on_start(ten::ten_env_t &ten_env) override {
    ten_env.on_start_done();

    if (!started) {
      started = true;

      send_invalid_graph(ten_env);

      auto start_graph_cmd = ten::cmd_start_graph_t::create();
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

      ten_env.send_cmd(
          std::move(start_graph_cmd),
          [](ten::ten_env_t &env, std::unique_ptr<ten::cmd_result_t> result) {
            // The graph check should be passed.
            if (result->get_status_code() == TEN_STATUS_CODE_OK) {
              auto close_app = ten::cmd_close_app_t::create();
              close_app->set_dest("localhost", nullptr, nullptr, nullptr);
              env.send_cmd(std::move(close_app));
            } else {
              std::cout << "Failed to start graph: "
                        << result->get_property_string("detail") << std::endl;
              std::exit(1);
            }
          });
    }
  }
};

TEN_CPP_REGISTER_ADDON_AS_EXTENSION(default_extension_cpp, test_extension);
