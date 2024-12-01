//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <cstdio>

#include "gtest/gtest.h"
#include "include_internal/ten_runtime/addon/addon_manager.h"
#include "include_internal/ten_runtime/addon/extension/extension.h"
#include "include_internal/ten_utils/lib/env.h"

class GlobalTestEnvironment : public ::testing::Environment {
 public:
  // This method is run before any test cases.
  void SetUp() override {
    if (!ten_env_set("TEN_DISABLE_ADDON_UNREGISTER_AFTER_APP_CLOSE", "true")) {
      perror("Failed to set TEN_DISABLE_ADDON_UNREGISTER_AFTER_APP_CLOSE");
      exit(-1);
    }

    ten_addon_manager_t *manager = ten_addon_manager_get_instance();

    // In the context of smoke testing, there is no need to register
    // `register_ctx`.
    ten_addon_manager_register_all_addons(manager, nullptr);
  }

  // This method is run after all test cases.
  void TearDown() override { ten_addon_unregister_all_extension(); }
};

GTEST_API_ int main(int argc, char **argv) {
  printf("Running main() from %s\n", __FILE__);
  testing::InitGoogleTest(&argc, argv);

  // Add the environment to Google Test.
  ::testing::AddGlobalTestEnvironment(new GlobalTestEnvironment);

  return RUN_ALL_TESTS();
}
