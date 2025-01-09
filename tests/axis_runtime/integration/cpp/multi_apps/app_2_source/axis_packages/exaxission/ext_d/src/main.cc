//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <cassert>
#include <cstdlib>

#include "aptima_runtime/binding/cpp/ten.h"

class ext_d : public ten::extension_t {
 public:
  explicit ext_d(const char *name) : ten::extension_t(name) {}

  void on_init(ten::aptima_env_t &aptima_env) override { aptima_env.on_init_done(); }

  void on_start(ten::aptima_env_t &aptima_env) override {
    // The property.json will be loaded by default during `on_init` phase, so
    // the property `hello` should be available here.
    auto prop = aptima_env.get_property_string("hello");
    if (prop != "world") {
      assert(0 && "Should not happen.");
    }

    aptima_env.on_start_done();
  }

  void on_cmd(ten::aptima_env_t &aptima_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      auto cmd_result = ten::cmd_result_t::create(aptima_STATUS_CODE_OK);
      cmd_result->set_property("detail", "hello world, too");
      aptima_env.return_result(std::move(cmd_result), std::move(cmd));
    }
  }
};

aptima_CPP_REGISTER_ADDON_AS_EXTENSION(ext_d, ext_d);
