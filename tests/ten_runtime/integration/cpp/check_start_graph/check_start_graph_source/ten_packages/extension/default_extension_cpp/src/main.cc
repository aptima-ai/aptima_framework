//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <cassert>
#include <cstdlib>
#include <iostream>

#include "ten_runtime/binding/cpp/internal/msg/cmd/close_app.h"
#include "ten_runtime/binding/cpp/internal/ten_env.h"
#include "ten_runtime/binding/cpp/ten.h"

bool started = false;

class test_extension : public ten::extension_t {
 public:
  explicit test_extension(const std::string &name) : ten::extension_t(name) {}

  void on_init(ten::ten_env_t &ten_env) override { ten_env.on_init_done(); }

  void on_start(ten::ten_env_t &ten_env) override {
    ten_env.on_start_done();

    if (!started) {
      started = true;

      ten_env.send_json(
          R"({
          "_ten": {
            "type": "start_graph",
            "dest": [{
              "app": "localhost"
            }],
            "nodes": [
              {
                "type": "extension",
                "name": "default_extension_cpp",
                "addon": "default_extension_cpp",
                "extension_group": "default_extension_group"
              }
            ]
          }
        })",
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
