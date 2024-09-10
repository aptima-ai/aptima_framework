//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include <nlohmann/json.hpp>
#include <string>

#include "gtest/gtest.h"
#include "include_internal/ten_runtime/binding/cpp/ten.h"
#include "include_internal/ten_runtime/test/extension_test.h"
#include "ten_runtime/extension/extension.h"
#include "ten_runtime/ten_env/internal/send.h"
#include "ten_runtime/ten_env/ten_env.h"
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

namespace {

class test_ten_mock : public ten::ten_env_mock_t {
 public:
  bool return_result(std::unique_ptr<ten::cmd_result_t> &&cmd,
                     std::unique_ptr<ten::cmd_t> &&target_cmd,
                     ten::error_t *err) override {
    auto code = cmd->get_status_code();
    auto message = cmd->get_property_string("detail");
    if (test_case_num == 1) {
      if (std::string(target_cmd->get_name()) == "hello_world") {
        if (code == TEN_STATUS_CODE_OK) {
          EXPECT_EQ(std::string(message), "hello world, too");
          return true;
        }
      }
    }

    return false;
  }

  int64_t test_case_num{0};
};

void ten_cmd_result_handler(ten_extension_t *extension, ten_env_t *ten_env,
                            ten_shared_ptr_t *cmd_result,
                            void *cmd_result_handler_data) {
  TEN_LOGE("receive result handler.");
}

void test_extension_on_start(ten_extension_t *self, ten_env_t *ten_env) {
  std::unique_ptr<ten::cmd_t> test_cmd = ten::cmd_t::create_from_json(
      R"({
               "_ten": {
                 "type": "cmd",
                 "name": "test"
               }
             })");
  ten_env_send_cmd(ten_env, test_cmd->get_underlying_msg(),
                   ten_cmd_result_handler, nullptr, nullptr);
}

bool test_case_1() {
  auto *ten_env_mock = new test_ten_mock();
  return ten_env_mock->addon_create_extension_async(
      "standalone_test_basic__test_extension_1",
      "standalone_test_basic__test_extension_1",
      [ten_env_mock](ten::ten_env_t &ten_env, ten::extension_t &extension) {
        ten_extension_t *test_extension = ten_extension_create(
            "test_extension", nullptr, test_extension_on_start, nullptr,
            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);

        ten_extension_test_t *test = ten_extension_test_create(
            test_extension, extension.get_c_extension());

        ten_extension_test_wait(test);

        // ten_env_mock->test_case_num = 1;

        // // Send a command to the extension.
        // auto cmd = ten::cmd_t::create("hello_world");
        // (static_cast<test_extension_1 *>(&extension))
        //     ->on_cmd(ten, std::move(cmd));

        ten_env_mock->addon_destroy_extension_async(
            &extension,
            [ten_env_mock](ten::ten_env_t &ten_env) { delete ten_env_mock; });
        ten_extension_destroy(test_extension);

        ten_extension_test_destroy(test);
      });

  return true;
}

}  // namespace

TEST(StandaloneTest, Basic) {  // NOLINT
  EXPECT_EQ(test_case_1(), true);
}
