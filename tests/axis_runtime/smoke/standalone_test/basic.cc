//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "gtest/gtest.h"
#include "include_internal/axis_runtime/binding/cpp/ten.h"
#include "axis_runtime/binding/cpp/detail/extension.h"
#include "axis_runtime/common/status_code.h"
#include "axis_utils/lang/cpp/lib/value.h"
#include "tests/axis_runtime/smoke/util/binding/cpp/check.h"

namespace {

// This part is the extension codes written by the developer, maintained in its
// final release form, and will not change due to testing requirements.

class test_extension_1 : public ten::extension_t {
 public:
  explicit test_extension_1(const char *name) : ten::extension_t(name) {}

  void on_cmd(ten::axis_env_t &axis_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      auto cmd_result = ten::cmd_result_t::create(axis_STATUS_CODE_OK);
      cmd_result->set_property("detail", "hello world, too");
      bool rc = axis_env.return_result(std::move(cmd_result), std::move(cmd));
      EXPECT_EQ(rc, true);
    }
  }
};

axis_CPP_REGISTER_ADDON_AS_EXTENSION(standalone_test_basic__test_extension_1,
                                    test_extension_1);

}  // namespace

namespace {

class extension_tester_1 : public ten::extension_tester_t {
 public:
  void on_start(ten::axis_env_tester_t &axis_env) override {
    // Send the first command to the extension.
    auto new_cmd = ten::cmd_t::create("hello_world");

    axis_env.send_cmd(
        std::move(new_cmd),
        [](ten::axis_env_tester_t &axis_env,
           std::unique_ptr<ten::cmd_result_t> result, ten::error_t *err) {
          if (result->get_status_code() == axis_STATUS_CODE_OK) {
            axis_env.stop_test();
          }
        });

    axis_env.on_start_done();
  }
};

}  // namespace

TEST(StandaloneTest, Basic) {  // NOLINT
  auto *tester = new extension_tester_1();
  tester->set_test_mode_single("standalone_test_basic__test_extension_1");

  bool rc = tester->run();
  axis_ASSERT(rc, "Should not happen.");

  delete tester;
}
