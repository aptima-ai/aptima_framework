//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "gtest/gtest.h"
#include "aptima_runtime/binding/cpp/ten.h"
#include "aptima_utils/lib/path.h"
#include "aptima_utils/lib/string.h"

namespace {

class extension_tester_1 : public ten::extension_tester_t {
 public:
  void on_start(ten::aptima_env_tester_t &aptima_env) override {
    // Send the first command to the extension.
    auto new_cmd = ten::cmd_t::create("hello_world");

    aptima_env.send_cmd(std::move(new_cmd),
                     [](ten::aptima_env_tester_t &aptima_env,
                        std::unique_ptr<ten::cmd_result_t> result,
                        ten::error_t * /*error*/) {
                       if (result->get_status_code() == aptima_STATUS_CODE_OK) {
                         aptima_env.stop_test();
                       }
                     });

    aptima_env.on_start_done();
  }
};

}  // namespace

TEST(Test, Basic) {  // NOLINT
  auto *tester = new extension_tester_1();

  aptima_string_t *path = aptima_path_get_executable_path();
  aptima_path_join_c_str(path, "../aptima_packages/extension/default_extension_cpp/");

  tester->add_addon_base_dir(aptima_string_get_raw_str(path));

  aptima_string_destroy(path);

  tester->set_test_mode_single("default_extension_cpp");

  tester->run();

  delete tester;
}
