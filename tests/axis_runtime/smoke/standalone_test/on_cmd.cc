//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "gtest/gtest.h"
#include "include_internal/aptima_runtime/binding/cpp/aptima.h"
#include "aptima_runtime/binding/cpp/detail/extension.h"
#include "aptima_runtime/common/status_code.h"
#include "aptima_utils/lang/cpp/lib/value.h"
#include "tests/aptima_runtime/smoke/util/binding/cpp/check.h"

namespace {

// This part is the extension codes written by the developer, maintained in its
// final release form, and will not change due to testing requirements.

class test_extension_1 : public aptima::extension_t {
 public:
  explicit test_extension_1(const char *name) : aptima::extension_t(name) {}

  void on_cmd(aptima::aptima_env_t &aptima_env,
              std::unique_ptr<aptima::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      auto cmd_result = aptima::cmd_result_t::create(aptima_STATUS_CODE_OK);
      cmd_result->set_property("detail", "hello world, too");
      bool rc = aptima_env.return_result(std::move(cmd_result), std::move(cmd));
      EXPECT_EQ(rc, true);

      // Send out an ack command.
      auto ack_cmd = aptima::cmd_t::create("ack", nullptr);
      rc = aptima_env.send_cmd(std::move(ack_cmd), nullptr);
      EXPECT_EQ(rc, true);
    }
  }
};

aptima_CPP_REGISTER_ADDON_AS_EXTENSION(standalone_test_on_cmd__test_extension_1,
                                    test_extension_1);

}  // namespace

namespace {

class extension_tester_1 : public aptima::extension_tester_t {
 protected:
  void on_start(aptima::aptima_env_tester_t &aptima_env) override {
    // Send the first command to the extension.
    auto new_cmd = aptima::cmd_t::create("hello_world");

    aptima_env.send_cmd(
        std::move(new_cmd),
        [this](aptima::aptima_env_tester_t &aptima_env,
               std::unique_ptr<aptima::cmd_result_t> result, aptima::error_t *err) {
          if (result->get_status_code() == aptima_STATUS_CODE_OK) {
            hello_world_cmd_success = true;
          }
        });

    aptima_env.on_start_done();
  }

  void on_cmd(aptima::aptima_env_tester_t &aptima_env,
              std::unique_ptr<aptima::cmd_t> cmd) override {
    if (cmd->get_name() == "ack") {
      ack_cmd_success = true;
    }

    if (ack_cmd_success && hello_world_cmd_success) {
      aptima_env.stop_test();
    }
  }

 private:
  bool hello_world_cmd_success = false;
  bool ack_cmd_success = false;
};

}  // namespace

TEST(StandaloneTest, OnCmd) {  // NOLINT
  auto *tester = new extension_tester_1();
  tester->set_test_mode_single("standalone_test_on_cmd__test_extension_1");

  bool rc = tester->run();
  aptima_ASSERT(rc, "Should not happen.");

  delete tester;
}
