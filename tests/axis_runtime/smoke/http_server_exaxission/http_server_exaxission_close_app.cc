//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <libwebsockets.h>

#include <cstddef>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>

#include "gtest/gtest.h"
#include "include_internal/axis_runtime/binding/cpp/ten.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/lib/thread.h"
#include "tests/common/client/http.h"

namespace {

class test_extension : public ten::extension_t {
 public:
  explicit test_extension(const char *name) : ten::extension_t(name) {}

  void on_cmd(ten::axis_env_t &axis_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      auto cmd_result = ten::cmd_result_t::create(axis_STATUS_CODE_OK);
      cmd_result->set_property("detail", "hello world, too");
      axis_env.return_result(std::move(cmd_result), std::move(cmd));
    }
  }
};

axis_CPP_REGISTER_ADDON_AS_EXTENSION(
    http_server_extension_close_app__test_extension, test_extension);

class test_app : public ten::app_t {
 public:
  void on_configure(ten::axis_env_t &axis_env) final {
    bool rc = ten::axis_env_internal_accessor_t::init_manifest_from_json(
        axis_env,
        // clang-format off
                 R"({
                      "type": "app",
                      "name": "test_app",
                      "version": "0.1.0"
                    })"
        // clang-format on
    );
    ASSERT_EQ(rc, true);

    rc = axis_env.init_property_from_json(
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
                            "addon": "http_server_extension_close_app__test_extension",
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

    axis_env.on_configure_done();
  }
};

ten::app_t *test_app_ = nullptr;

void *test_app_thread_main(axis_UNUSED void *args) {
  test_app_ = new test_app();
  test_app_->run(true);
  test_app_->wait();
  delete test_app_;
  test_app_ = nullptr;

  return nullptr;
}

}  // namespace

TEST(ExtensionTest, HttpServerExtensionCloseApp) {  // NOLINT
  auto *app_thread =
      axis_thread_create("app thread", test_app_thread_main, nullptr);

  axis_test_http_client_init();

  axis_string_t resp;
  axis_string_init(&resp);
  axis_test_http_client_post("http://127.0.0.1:8001/",
                            R"({"_ten": {"name": "hello_world"}})", &resp);
  EXPECT_EQ(std::string(axis_string_get_raw_str(&resp)), "\"hello world, too\"");

  axis_string_clear(&resp);
  axis_test_http_client_post("http://127.0.0.1:8001/",
                            R"({"_ten": {"type": "close_app"}})", &resp);
  EXPECT_EQ(std::string(axis_string_get_raw_str(&resp)), "TEN is closed.");

  axis_string_deinit(&resp);
  axis_test_http_client_deinit();

  axis_thread_join(app_thread, -1);
}
