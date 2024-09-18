//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "include_internal/ten_runtime/binding/cpp/ten.h"
#include "nlohmann/json_fwd.hpp"
#include "ten_utils/lib/thread.h"
#include "ten_utils/lib/time.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/common/constant.h"
#include "tests/ten_runtime/smoke/extension_test/util/binding/cpp/check.h"

namespace {

class test_extension_A : public ten::extension_t {
 public:
  explicit test_extension_A(const std::string &name) : ten::extension_t(name) {}

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    ten_env.send_cmd(std::move(cmd));
  }
};

class test_extension_B : public ten::extension_t {
 public:
  explicit test_extension_B(const std::string &name) : ten::extension_t(name) {}

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    auto detail = nlohmann::json({{"a", "b"}});
    auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
    cmd_result->set_property_from_json("detail", detail.dump().c_str());
    ten_env.return_result(std::move(cmd_result), std::move(cmd));
  }
};

class test_app_a : public ten::app_t {
 public:
  void on_configure(ten::ten_env_t &ten_env) override {
    bool rc = ten_env.init_property_from_json(
        // clang-format off
                 R"({
                      "_ten": {
                        "uri": "msgpack://127.0.0.1:8001/",
                        "long_running_mode": true,
                        "log_level": 2
                      }
                    })"
        // clang-format on
    );
    ASSERT_EQ(rc, true);

    ten_env.on_configure_done();
  }
};

class test_app_b : public ten::app_t {
 public:
  void on_configure(ten::ten_env_t &ten_env) override {
    bool rc = ten_env.init_property_from_json(
        // clang-format off
                 R"({
                      "_ten": {
                        "uri": "msgpack://127.0.0.1:8002/",
                        "long_running_mode": true,
                        "log_level": 2
                      }
                    })"
        // clang-format on
    );
    ASSERT_EQ(rc, true);

    ten_env.on_configure_done();
  }
};

void *app_thread_1_main(TEN_UNUSED void *args) {
  auto *app_a = new test_app_a();
  app_a->run();
  delete app_a;

  return nullptr;
}

void *app_thread_2_main(TEN_UNUSED void *args) {
  auto *app_b = new test_app_b();
  app_b->run();
  delete app_b;

  return nullptr;
}

std::string graph_name;

void *client_thread_main(TEN_UNUSED void *args) {
  auto seq_id = (size_t)args;

  TEN_LOGD("Client[%zu]: start.", seq_id);

  auto seq_id_str = std::to_string(seq_id);

  // connect again, send request with graph_name directly
  auto *client = new ten::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

  std::string client_ip;
  uint16_t client_port = 0;
  client->get_info(client_ip, client_port);
  TEN_LOGD("Client[%zu] ip address: %s:%d", seq_id, client_ip.c_str(),
           client_port);

  // Send a user-defined 'hello world' command.
  nlohmann::json request = R"({
                                "_ten": {
                                  "name": "test",
                                  "dest":[{
                                    "app": "msgpack://127.0.0.1:8001/",
                                    "extension_group": "extension_group_A",
                                    "extension": "A"
                                  }]
                                }
                              })"_json;
  request["_ten"]["dest"][0]["graph"] = graph_name;
  request["_ten"]["seq_id"] = seq_id_str;

  nlohmann::json resp = client->send_json_and_recv_resp_in_json(request);
  ten_test::check_result_is(resp, seq_id_str, TEN_STATUS_CODE_OK,
                            R"({"a": "b"})");

  // Destroy the client.
  delete client;

  return nullptr;
}

TEN_CPP_REGISTER_ADDON_AS_EXTENSION(one_engine_concurrent__extension_A,
                                    test_extension_A);

TEN_CPP_REGISTER_ADDON_AS_EXTENSION(one_engine_concurrent__extension_B,
                                    test_extension_B);

}  // namespace

TEST(ExtensionTest, OneEngineConcurrent) {  // NOLINT
  // Start app.
  auto *app_thread_2 =
      ten_thread_create("app thread 2", app_thread_2_main, nullptr);
  auto *app_thread_1 =
      ten_thread_create("app thread 1", app_thread_1_main, nullptr);

  ten_sleep(300);

  // Create a client and connect to the app.
  ten::msgpack_tcp_client_t *client = nullptr;

  for (size_t i = 0; i < MULTIPLE_APP_SCENARIO_GRAPH_CONSTRUCTION_RETRY_TIMES;
       ++i) {
    client = new ten::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

    // Send graph.
    nlohmann::json resp = client->send_json_and_recv_resp_in_json(
        R"({
             "_ten": {
               "type": "start_graph",
               "long_running_mode": true,
               "seq_id": "55",
               "nodes": [{
                 "type": "extension_group",
                 "name": "extension_group_A",
                 "addon": "default_extension_group",
                 "app": "msgpack://127.0.0.1:8001/"
               },{
                 "type": "extension_group",
                 "name": "extension_group_B",
                 "addon": "default_extension_group",
                 "app": "msgpack://127.0.0.1:8002/"
               },{
                 "type": "extension",
                 "name": "A",
                 "addon": "one_engine_concurrent__extension_A",
                 "app": "msgpack://127.0.0.1:8001/",
                 "extension_group": "extension_group_A"
               },{
                 "type": "extension",
                 "name": "B",
                 "addon": "one_engine_concurrent__extension_B",
                 "app": "msgpack://127.0.0.1:8002/",
                 "extension_group": "extension_group_B"
               }],
               "connections": [{
                 "app": "msgpack://127.0.0.1:8001/",
                 "extension_group": "extension_group_A",
                 "extension": "A",
                 "cmd": [{
                   "name": "test",
                   "dest": [{
                     "app": "msgpack://127.0.0.1:8002/",
                     "extension_group": "extension_group_B",
                     "extension": "B"
                   }]
                 }]
               }]
             }
           })"_json);

    if (!resp.empty()) {
      ten_test::check_status_code_is(resp, TEN_STATUS_CODE_OK);
      graph_name = resp.value("detail", "");

      break;
    } else {
      delete client;
      client = nullptr;

      // To prevent from busy re-trying.
      ten_sleep(10);
    }
  }

  TEN_ASSERT(client, "Failed to connect to the TEN app.");

  // Now close connection. The engine will be alive due to 'long_running_mode'.
  delete client;

  std::vector<ten_thread_t *> client_threads;

  for (size_t i = 0; i < ONE_ENGINE_ALL_CLIENT_CONCURRENT_CNT; ++i) {
    auto *client_thread =
        ten_thread_create("client_thread_main", client_thread_main, (void *)i);

    client_threads.push_back(client_thread);
  }

  for (ten_thread_t *client_thread : client_threads) {
    ten_thread_join(client_thread, -1);
  }

  ten::msgpack_tcp_client_t::close_app("msgpack://127.0.0.1:8001/");
  ten::msgpack_tcp_client_t::close_app("msgpack://127.0.0.1:8002/");

  ten_thread_join(app_thread_1, -1);
  ten_thread_join(app_thread_2, -1);
}
