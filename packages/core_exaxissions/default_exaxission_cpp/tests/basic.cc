//
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#include "gtest/gtest.h"
#include "axis_runtime/addon/addon.h"
#include "axis_runtime/binding/cpp/aptima.h"
#include "axis_utils/lib/path.h"
#include "axis_utils/lib/string.h"

namespace {

class default_extension_tester : public aptima::extension_tester_t {
 public:
  void on_start(aptima::axis_env_tester_t &axis_env) override {
    // Send the first command to the extension.
    auto new_cmd = aptima::cmd_t::create("foo");

    axis_env.send_cmd(std::move(new_cmd),
                     [](aptima::axis_env_tester_t &axis_env,
                        std::unique_ptr<aptima::cmd_result_t> result,
                        aptima::error_t * /*error*/) {
                       if (result->get_status_code() == axis_STATUS_CODE_OK) {
                         axis_env.stop_test();
                       }
                     });
  }
};

}  // namespace

TEST(Test, Basic) {  // NOLINT
  auto *tester = new default_extension_tester();

  axis_string_t *path = axis_path_get_executable_path();
  axis_path_join_c_str(path, "../axis_packages/extension/default_extension_cpp/");

  tester->add_addon_base_dir(axis_string_get_raw_str(path));

  axis_string_destroy(path);

  tester->set_test_mode_single("default_extension_cpp");

  tester->run();

  delete tester;
}
