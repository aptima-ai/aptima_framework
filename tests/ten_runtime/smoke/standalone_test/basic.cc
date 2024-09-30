//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "gtest/gtest.h"
#include "include_internal/ten_runtime/binding/cpp/ten.h"
#include "include_internal/ten_runtime/test/extension_test.h"
#include "ten_runtime/common/status_code.h"
#include "ten_runtime/msg/cmd/cmd.h"
#include "ten_runtime/msg/cmd_result/cmd_result.h"
#include "ten_utils/lang/cpp/lib/value.h"
#include "ten_utils/lib/event.h"
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
      auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
      cmd_result->set_property("detail", "hello world, too");
      bool rc = ten_env.return_result(std::move(cmd_result), std::move(cmd));
      EXPECT_EQ(rc, true);
    }
  }
};

TEN_CPP_REGISTER_ADDON_AS_EXTENSION(standalone_test_basic__test_extension_1,
                                    test_extension_1);

}  // namespace

static void hello_world_cmd_result_handler(ten_shared_ptr_t *cmd_result,
                                           void *user_data) {
  auto *cmd_success = static_cast<ten_event_t *>(user_data);
  TEN_ASSERT(cmd_success, "Invalid argument.");

  if (ten_cmd_result_get_status_code(cmd_result) == TEN_STATUS_CODE_OK) {
    ten_event_set(cmd_success);
  }
}

TEST(StandaloneTest, Basic) {  // NOLINT
  ten_extension_test_t *test = ten_extension_test_create();
  ten_extension_test_add_addon(test, "standalone_test_basic__test_extension_1");
  ten_extension_test_start(test);

  ten_shared_ptr_t *hello_world_cmd = ten_cmd_create("hello_world", nullptr);
  TEN_ASSERT(hello_world_cmd, "Should not happen.");

  ten_event_t *cmd_success = ten_event_create(0, 0);

  ten_extension_test_send_cmd(test, hello_world_cmd,
                              hello_world_cmd_result_handler, cmd_success);

  ten_event_wait(cmd_success, -1);
  ten_event_destroy(cmd_success);

  ten_extension_test_destroy(test);
}
