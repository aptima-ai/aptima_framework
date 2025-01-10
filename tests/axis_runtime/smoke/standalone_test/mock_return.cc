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

      // Send out a command to retrieve the greeting message.
      auto fetch_cmd = aptima::cmd_t::create("fetch_greeting");
      rc = aptima_env.send_cmd(
          std::move(fetch_cmd),
          [](aptima::aptima_env_t &aptima_env, std::unique_ptr<aptima::cmd_result_t> result,
             aptima::error_t *err) {
            if (result->get_status_code() == aptima_STATUS_CODE_OK) {
              auto detail = result->get_property_string("detail");
              EXPECT_EQ(detail, "hola");

              auto data = aptima::data_t::create("greeting");
              data->set_property("text", detail);

              bool rc = aptima_env.send_data(std::move(data));
              EXPECT_EQ(rc, true);
            }
          });
      EXPECT_EQ(rc, true);
    }
  }
};

aptima_CPP_REGISTER_ADDON_AS_EXTENSION(
    standalone_test_mock_return__test_extension_1, test_extension_1);

}  // namespace

namespace {

class extension_tester_1 : public aptima::extension_tester_t {
 protected:
  void on_start(aptima::aptima_env_tester_t &aptima_env) override {
    // Send the first command to the extension.
    auto new_cmd = aptima::cmd_t::create("hello_world");

    aptima_env.send_cmd(
        std::move(new_cmd),
        [](aptima::aptima_env_tester_t &aptima_env,
           std::unique_ptr<aptima::cmd_result_t> result, aptima::error_t *err) {
          if (result->get_status_code() == aptima_STATUS_CODE_OK) {
            auto detail = result->get_property_string("detail");
            EXPECT_EQ(detail, "hello world, too");
          }
        });

    aptima_env.on_start_done();
  }

  void on_cmd(aptima::aptima_env_tester_t &aptima_env,
              std::unique_ptr<aptima::cmd_t> cmd) override {
    if (cmd->get_name() == "fetch_greeting") {
      // Mock the result of the fetch_greeting command.
      auto cmd_result = aptima::cmd_result_t::create(aptima_STATUS_CODE_OK);
      cmd_result->set_property("detail", "hola");
      bool rc = aptima_env.return_result(std::move(cmd_result), std::move(cmd));
      EXPECT_EQ(rc, true);
    }
  }

  void on_data(aptima::aptima_env_tester_t &aptima_env,
               std::unique_ptr<aptima::data_t> data) override {
    if (data->get_name() == "greeting") {
      auto text = data->get_property_string("text");
      EXPECT_EQ(text, "hola");

      aptima_env.stop_test();
    }
  }
};

}  // namespace

TEST(StandaloneTest, MockReturn) {  // NOLINT
  auto *tester = new extension_tester_1();
  tester->set_test_mode_single("standalone_test_mock_return__test_extension_1");

  bool rc = tester->run();
  aptima_ASSERT(rc, "Should not happen.");

  delete tester;
}
