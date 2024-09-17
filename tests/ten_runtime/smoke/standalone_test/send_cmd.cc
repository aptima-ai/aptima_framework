//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <nlohmann/json.hpp>
#include <string>

#include "gtest/gtest.h"
#include "include_internal/ten_runtime/binding/cpp/ten.h"
#include "ten_runtime/common/status_code.h"
#include "ten_utils/lang/cpp/lib/error.h"
#include "ten_utils/lang/cpp/lib/value.h"
#include "tests/ten_runtime/smoke/extension_test/util/binding/cpp/check.h"

namespace {

// This part is the extension codes written by the developer, maintained in its
// final release form, and will not change due to testing requirements.

class test_extension_1 : public ten::extension_t {
 public:
  explicit test_extension_1(const std::string &name) : ten::extension_t(name) {}

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (std::string(cmd->get_name()) == "hello_world") {
      ten_env.send_cmd(
          std::move(cmd), [](ten::ten_env_t &ten_env,
                             std::unique_ptr<ten::cmd_result_t> cmd_result) {
            if (cmd_result->get_status_code() == TEN_STATUS_CODE_OK) {
              bool rc = ten_env.return_result_directly(std::move(cmd_result));
              EXPECT_EQ(rc, true);
            }
          });
    }
  }
};

TEN_CPP_REGISTER_ADDON_AS_EXTENSION(standalone_test_send_cmd__test_extension_1,
                                    test_extension_1);

}  // namespace

namespace {

class test_ten_mock : public ten::ten_env_mock_t {
 public:
  bool send_cmd(std::unique_ptr<ten::cmd_t> &&cmd,
                ten::result_handler_func_t &&result_handler,
                ten::error_t *err) override {
    if (test_case_num == 1) {
      if (std::string(cmd->get_name()) == "hello_world") {
        auto status = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
        status->set_property("detail", static_cast<int8_t>(3));

        result_handler(*this, std::move(status));

        return true;
      }
    }

    return false;
  }

  bool return_result_directly(std::unique_ptr<ten::cmd_result_t> &&cmd,
                              ten::error_t *err) override {
    if (test_case_num == 1) {
      if (cmd->get_status_code() == TEN_STATUS_CODE_OK) {
        EXPECT_EQ(cmd->get_property_int8("detail"), 3);
        return true;
      }
    }

    return false;
  }

  int64_t test_case_num{0};
};

bool test_case_1() {
  auto *ten_env_mock = new test_ten_mock();
  return ten_env_mock->addon_create_extension_async(
      "standalone_test_send_cmd__test_extension_1",
      "standalone_test_send_cmd__test_extension_1",
      [ten_env_mock](ten::ten_env_t &ten_env, ten::extension_t &extension) {
        ten_env_mock->test_case_num = 1;

        // Send a command to the extension.
        auto cmd = ten::cmd_t::create("hello_world");
        (static_cast<test_extension_1 *>(&extension))
            ->on_cmd(ten_env, std::move(cmd));

        ten_env_mock->addon_destroy_extension_async(
            &extension,
            [ten_env_mock](ten::ten_env_t &ten_env) { delete ten_env_mock; });
      });
}

}  // namespace

TEST(StandaloneTest, SendCmd) {  // NOLINT
  EXPECT_EQ(test_case_1(), true);
}
