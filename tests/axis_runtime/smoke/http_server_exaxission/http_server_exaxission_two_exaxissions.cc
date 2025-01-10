//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <libwebsockets.h>

#include <cstddef>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>

#include "gtest/gtest.h"
#include "include_internal/aptima_runtime/binding/cpp/aptima.h"
#include "aptima_utils/lib/thread.h"
#include "tests/common/client/http.h"

namespace {

class test_extension : public aptima::extension_t {
 public:
  explicit test_extension(const char *name) : aptima::extension_t(name) {}

  void on_cmd(aptima::aptima_env_t &aptima_env,
              std::unique_ptr<aptima::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      auto cmd_result = aptima::cmd_result_t::create(aptima_STATUS_CODE_OK);
      cmd_result->set_property("detail", "hello world, too");
      aptima_env.return_result(std::move(cmd_result), std::move(cmd));
    }
  }
};

aptima_CPP_REGISTER_ADDON_AS_EXTENSION(
    http_server_extension_two_extensions__test_extension, test_extension);

class test_app : public aptima::app_t {
 public:
  void on_configure(aptima::aptima_env_t &aptima_env) final {
    bool rc = aptima::aptima_env_internal_accessor_t::init_manifest_from_json(
        aptima_env,
        // clang-format off
                 R"({
                      "type": "app",
                      "name": "test_app",
                      "version": "0.1.0"
                    })"
        // clang-format on
    );
    ASSERT_EQ(rc, true);

    rc = aptima_env.init_property_from_json(
        // clang-format off
                 R"({
                      "_ten": {
                        "log_level": 2,
                        "predefined_graphs": [{
                          "name": "default",
                          "auto_start": true,
                          "nodes": [{
                            "type": "extension",
                            "name": "simple_http_server_cpp",
                            "addon": "simple_http_server_cpp",
                            "extension_group": "test_extension_group"
                          },{
                            "type": "extension",
                            "name": "test_extension",
                            "addon": "http_server_extension_two_extensions__test_extension",
                            "extension_group": "test_extension_group"
                          }],
                          "connections": [{
                            "extension": "simple_http_server_cpp",
                            "cmd": [{
                              "name": "hello_world",
                              "dest": [{
                                "extension": "test_extension"
                              }]
                            }]
                          }]
                        }]
                      }
                    })"
        // clang-format on
    );
    ASSERT_EQ(rc, true);

    aptima_env.on_configure_done();
  }
};

aptima::app_t *test_app_ = nullptr;

void *test_app_thread_main(aptima_UNUSED void *args) {
  test_app_ = new test_app();
  test_app_->run(true);
  test_app_->wait();
  delete test_app_;
  test_app_ = nullptr;

  return nullptr;
}

}  // namespace

TEST(ExtensionTest, HttpServerExtensionTwoExtensions) {  // NOLINT
  auto *app_thread =
      aptima_thread_create("app thread", test_app_thread_main, nullptr);

  aptima_test_http_client_init();

  aptima_string_t resp;
  aptima_string_init(&resp);
  aptima_test_http_client_post("http://127.0.0.1:8001/",
                            R"({"_ten": {"name": "hello_world"}})", &resp);
  EXPECT_EQ(std::string(aptima_string_get_raw_str(&resp)), "\"hello world, too\"");

  aptima_string_deinit(&resp);
  aptima_test_http_client_deinit();

  if (test_app_ != nullptr) {
    test_app_->close();
  }

  aptima_thread_join(app_thread, -1);
}
