//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <cstdio>

#include "gtest/gtest.h"
#include "include_internal/aptima_runtime/addon/addon.h"
#include "include_internal/aptima_utils/lib/env.h"

class GlobalTestEnvironment : public ::testing::Environment {
 public:
  // This method is run before any test cases.
  void SetUp() override {
    // In a smoke test, the relationship between the app and the process is not
    // one-to-one. Therefore, addons cannot be unloaded when the app ends; they
    // should only be unloaded when the entire process ends. Currently, the
    // following environment variable is used to control whether to perform the
    // addon unload action when the app ends.
    if (!aptima_env_set("aptima_DISABLE_ADDON_UNREGISTER_AFTER_APP_CLOSE", "true")) {
      perror("Failed to set aptima_DISABLE_ADDON_UNREGISTER_AFTER_APP_CLOSE");

      // NOLINTNEXTLINE(concurrency-mt-unsafe)
      exit(EXIT_FAILURE);
    }
  }

  // This method is run after all test cases.
  void TearDown() override { aptima_unregister_all_addons_and_cleanup(); }
};

GTEST_API_ int main(int argc, char **argv) {
  printf("Running main() from %s\n", __FILE__);
  testing::InitGoogleTest(&argc, argv);

  // Add the environment to Google Test.
  ::testing::AddGlobalTestEnvironment(new GlobalTestEnvironment);

  return RUN_ALL_TESTS();
}
