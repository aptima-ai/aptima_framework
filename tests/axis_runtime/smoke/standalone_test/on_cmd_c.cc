//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "gtest/gtest.h"
#include "include_internal/aptima_runtime/binding/cpp/ten.h"
#include "include_internal/aptima_runtime/test/env_tester.h"
#include "include_internal/aptima_runtime/test/extension_tester.h"
#include "aptima_runtime/common/status_code.h"
#include "aptima_runtime/msg/cmd/cmd.h"
#include "aptima_runtime/msg/cmd_result/cmd_result.h"
#include "aptima_runtime/test/env_tester.h"
#include "aptima_utils/lang/cpp/lib/value.h"
#include "aptima_utils/lib/smart_ptr.h"
#include "tests/aptima_runtime/smoke/util/binding/cpp/check.h"

namespace {

// This part is the extension codes written by the developer, maintained in its
// final release form, and will not change due to testing requirements.

class test_extension_1 : public ten::extension_t {
 public:
  explicit test_extension_1(const char *name) : ten::extension_t(name) {}

  void on_cmd(ten::aptima_env_t &aptima_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      auto cmd_result = ten::cmd_result_t::create(aptima_STATUS_CODE_OK);
      cmd_result->set_property("detail", "hello world, too");
      bool rc = aptima_env.return_result(std::move(cmd_result), std::move(cmd));
      EXPECT_EQ(rc, true);

      // Send out an ack command.
      auto ack_cmd = ten::cmd_t::create("ack", nullptr);
      rc = aptima_env.send_cmd(std::move(ack_cmd), nullptr);
      EXPECT_EQ(rc, true);
    }
  }
};

aptima_CPP_REGISTER_ADDON_AS_EXTENSION(standalone_test_on_cmd_c__test_extension_1,
                                    test_extension_1);

}  // namespace

namespace {

typedef struct test_info_t {
  bool hello_world_cmd_success;
  bool ack_cmd_success;
} test_info_t;

void hello_world_cmd_result_handler(aptima_env_tester_t *aptima_env,
                                    aptima_shared_ptr_t *cmd_result,
                                    void *user_data,
                                    aptima_UNUSED aptima_error_t *err) {
  if (aptima_cmd_result_get_status_code(cmd_result) == aptima_STATUS_CODE_OK) {
    auto *test_info = static_cast<test_info_t *>(user_data);
    test_info->hello_world_cmd_success = true;
  }
}

void aptima_extension_tester_on_start(aptima_UNUSED aptima_extension_tester_t *tester,
                                   aptima_env_tester_t *aptima_env) {
  auto *test_info = static_cast<test_info_t *>(aptima_MALLOC(sizeof(test_info_t)));
  aptima_ASSERT(test_info, "Failed to allocate memory.");

  test_info->hello_world_cmd_success = false;
  test_info->ack_cmd_success = false;

  tester->user_data = test_info;

  aptima_shared_ptr_t *hello_world_cmd = aptima_cmd_create("hello_world", nullptr);
  aptima_ASSERT(hello_world_cmd, "Should not happen.");

  bool rc = aptima_env_tester_send_cmd(aptima_env, hello_world_cmd,
                                    hello_world_cmd_result_handler, test_info,
                                    nullptr);

  if (rc) {
    aptima_shared_ptr_destroy(hello_world_cmd);
  }

  aptima_env_tester_on_start_done(aptima_env, nullptr);
}

void aptima_extension_tester_on_cmd(aptima_UNUSED aptima_extension_tester_t *tester,
                                 aptima_env_tester_t *aptima_env,
                                 aptima_shared_ptr_t *cmd) {
  auto *test_info = static_cast<test_info_t *>(tester->user_data);

  if (std::string(aptima_msg_get_name(cmd)) == "ack") {
    test_info->ack_cmd_success = true;
  }

  if (test_info->ack_cmd_success && test_info->hello_world_cmd_success) {
    aptima_FREE(test_info);
    bool rc = aptima_env_tester_stop_test(aptima_env, nullptr);
    aptima_ASSERT(rc, "Should not happen.");
  }
}

}  // namespace

TEST(StandaloneTest, OnCmdC) {  // NOLINT
  aptima_extension_tester_t *tester = aptima_extension_tester_create(
      aptima_extension_tester_on_start, aptima_extension_tester_on_cmd, nullptr,
      nullptr, nullptr);
  aptima_extension_tester_set_test_mode_single(
      tester, "standalone_test_on_cmd_c__test_extension_1", nullptr);

  bool rc = aptima_extension_tester_run(tester);
  aptima_ASSERT(rc, "Should not happen.");

  aptima_extension_tester_destroy(tester);
}
