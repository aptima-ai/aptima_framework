//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <nlohmann/json.hpp>
#include <string>

#include "gtest/gtest.h"
#include "include_internal/axis_runtime/binding/cpp/ten.h"
#include "axis_utils/lib/thread.h"
#include "tests/axis_runtime/smoke/util/binding/cpp/check.h"

namespace {

class test_app : public ten::app_t {
 public:
  void on_configure(ten::axis_env_t &axis_env) override {
    // The protocol (error://) is not a correct one, so the listening will fail.
    bool rc = axis_env.init_property_from_json(
        // clang-format off
                 R"({
                      "_ten": {
                        "uri": "error://127.0.0.1:8001/",
                        "log_level": 2
                      }
                    })"
        // clang-format on
    );
    ASSERT_EQ(rc, true);

    axis_env.on_configure_done();
  }
};

void *test_app_thread_main(axis_UNUSED void *args) {
  auto *app = new test_app();
  app->run();
  delete app;

  return nullptr;
}

}  // namespace

TEST(ExtensionTest, FailedToListen) {  // NOLINT
  auto *app_thread =
      axis_thread_create("app thread", test_app_thread_main, nullptr);

  // The app would stop automatically after the listening failed.

  axis_thread_join(app_thread, -1);
}
