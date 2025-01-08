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
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/axis_runtime/smoke/util/binding/cpp/check.h"

#define DATA "hello world"

namespace {

class test_extension : public ten::extension_t {
 public:
  explicit test_extension(const char *name) : ten::extension_t(name) {}

  void on_cmd(ten::axis_env_t &axis_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (cmd->get_name() == "check_received") {
      if (received) {
        auto cmd_result = ten::cmd_result_t::create(axis_STATUS_CODE_OK);
        cmd_result->set_property("detail", "received confirmed");
        axis_env.return_result(std::move(cmd_result), std::move(cmd));
      } else {
        auto cmd_result = ten::cmd_result_t::create(axis_STATUS_CODE_ERROR);
        cmd_result->set_property("detail", "received failed");
        axis_env.return_result(std::move(cmd_result), std::move(cmd));
      }
    }
  }

  void on_data(axis_UNUSED ten::axis_env_t &axis_env,
               std::unique_ptr<ten::data_t> data) override {
    ten::buf_t buf = data->get_buf();
    if (memcmp(buf.data(), DATA, buf.size()) == 0) {
      received = true;
    }
  }

 private:
  bool received{};
};

class test_app : public ten::app_t {
 public:
  void on_configure(ten::axis_env_t &axis_env) override {
    bool rc = axis_env.init_property_from_json(
        // clang-format off
                 R"({
                      "_ten": {
                        "uri": "msgpack://127.0.0.1:8001/",
                        "log_level": 2
                      }
                    })"
        // clang-format on
        ,
        nullptr);
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

axis_CPP_REGISTER_ADDON_AS_EXTENSION(data_basic__extension, test_extension);

}  // namespace

TEST(DataTest, Basic) {  // NOLINT
  // Start app.
  auto *app_thread =
      axis_thread_create("app thread", test_app_thread_main, nullptr);

  // Create a client and connect to the app.
  auto *client = new ten::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

  // Send graph.
  auto start_graph_cmd = ten::cmd_start_graph_t::create();
  start_graph_cmd->set_graph_from_json(R"({
           "nodes": [{
               "type": "extension",
               "name": "test_extension",
               "addon": "data_basic__extension",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "default_extension_group"
             }]
           })");
  auto cmd_result =
      client->send_cmd_and_recv_result(std::move(start_graph_cmd));
  axis_test::check_status_code(cmd_result, axis_STATUS_CODE_OK);

  const char *str = DATA;
  client->send_data("", "default_extension_group", "test_extension",
                    (void *)str, strlen(str) + 1);
  auto check_received_cmd = ten::cmd_t::create("check_received");
  check_received_cmd->set_dest("msgpack://127.0.0.1:8001/", nullptr,
                               "default_extension_group", "test_extension");
  cmd_result = client->send_cmd_and_recv_result(std::move(check_received_cmd));
  axis_test::check_status_code(cmd_result, axis_STATUS_CODE_OK);
  axis_test::check_detail_with_string(cmd_result, "received confirmed");

  delete client;

  axis_thread_join(app_thread, -1);
}
