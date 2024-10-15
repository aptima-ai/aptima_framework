//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "include_internal/ten_runtime/binding/cpp/ten.h"
#include "ten_utils/lib/thread.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/ten_runtime/smoke/extension_test/util/binding/cpp/check.h"

#define DATA "hello world"

namespace {

class test_extension : public ten::extension_t {
 public:
  explicit test_extension(const std::string &name) : ten::extension_t(name) {}

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    nlohmann::json json = nlohmann::json::parse(cmd->to_json());
    if (json["_ten"]["name"] == "check_received") {
      if (received) {
        auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
        cmd_result->set_property("detail", "received confirmed");
        ten_env.return_result(std::move(cmd_result), std::move(cmd));
      } else {
        auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_ERROR);
        cmd_result->set_property("detail", "received failed");
        ten_env.return_result(std::move(cmd_result), std::move(cmd));
      }
    }
  }

  void on_data(TEN_UNUSED ten::ten_env_t &ten_env,
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
  void on_configure(ten::ten_env_t &ten_env) override {
    bool rc = ten_env.init_property_from_json(
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

    ten_env.on_configure_done();
  }
};

void *test_app_thread_main(TEN_UNUSED void *args) {
  auto *app = new test_app();
  app->run();
  delete app;

  return nullptr;
}

TEN_CPP_REGISTER_ADDON_AS_EXTENSION(data_basic__extension, test_extension);

}  // namespace

TEST(DataTest, Basic) {  // NOLINT
  // Start app.
  auto *app_thread =
      ten_thread_create("app thread", test_app_thread_main, nullptr);

  // Create a client and connect to the app.
  auto *client = new ten::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

  // Send graph.
  nlohmann::json resp = client->send_json_and_recv_resp_in_json(
      R"({
           "_ten": {
             "type": "start_graph",
             "seq_id": "55",
             "nodes": [{
               "type": "extension",
               "name": "test_extension",
               "addon": "data_basic__extension",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "default_extension_group"
             }]
           }
         })"_json);
  ten_test::check_status_code_is(resp, TEN_STATUS_CODE_OK);

  const char *str = DATA;
  client->send_data("", "default_extension_group", "test_extension",
                    (void *)str, strlen(str) + 1);

  resp = client->send_json_and_recv_resp_in_json(
      R"({
           "_ten": {
             "name": "check_received",
             "seq_id": "137",
             "dest":[{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "default_extension_group",
               "extension": "test_extension"
             }]
           }
         })"_json);
  ten_test::check_result_is(resp, "137", TEN_STATUS_CODE_OK,
                            "received confirmed");

  delete client;

  ten_thread_join(app_thread, -1);
}
