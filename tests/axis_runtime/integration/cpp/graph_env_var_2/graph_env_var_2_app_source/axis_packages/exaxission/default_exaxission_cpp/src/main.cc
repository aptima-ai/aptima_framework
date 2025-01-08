//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <cassert>
#include <cstdlib>

#include "axis_runtime/binding/cpp/ten.h"

class test_extension : public ten::extension_t {
 public:
  explicit test_extension(const char *name) : ten::extension_t(name) {}

  void on_init(ten::axis_env_t &axis_env) override { axis_env.on_init_done(); }

  void on_start(ten::axis_env_t &axis_env) override {
    // The property.json will be loaded by default during `on_init` phase, so
    // the property `prop` should be available here.
    auto prop = axis_env.get_property_string("prop");
    if (prop != "foobar,foo, bar") {
      assert(0 && "Should not happen.");
    }

    axis_env.on_start_done();
  }

  void on_cmd(ten::axis_env_t &axis_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      auto cmd_result = ten::cmd_result_t::create(axis_STATUS_CODE_OK);
      cmd_result->set_property("detail", "hello world, too");
      axis_env.return_result(std::move(cmd_result), std::move(cmd));
    }
  }
};

axis_CPP_REGISTER_ADDON_AS_EXTENSION(default_extension_cpp, test_extension);
