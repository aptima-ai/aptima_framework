//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <nlohmann/json.hpp>

#include "gtest/gtest.h"
#include "include_internal/axis_runtime/binding/cpp/ten.h"
#include "axis_utils/lib/thread.h"
#include "axis_utils/lib/time.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/common/constant.h"
#include "tests/axis_runtime/smoke/util/binding/cpp/check.h"

namespace {

class test_extension : public ten::extension_t {
 public:
  explicit test_extension(const char *name)
      : ten::extension_t(name), name_(name) {}

  void on_cmd(ten::axis_env_t &axis_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    nlohmann::json data = nlohmann::json::parse(cmd->get_property_to_json());

    data["send_from"] = name_;

    axis_UNUSED bool const rc =
        cmd->set_property_from_json(nullptr, data.dump().c_str());
    axis_ASSERT(rc, "Should not happen.");

    // extension1(app1) -> extension3(app2) -> extension2(app1) -> return
    if (name_ == "extension2") {
      const nlohmann::json detail = {{"id", 1}, {"name", "aa"}};

      auto cmd_result = ten::cmd_result_t::create(axis_STATUS_CODE_OK);
      cmd_result->set_property_from_json("detail", detail.dump().c_str());

      axis_env.return_result(std::move(cmd_result), std::move(cmd));
    } else {
      axis_env.send_cmd(
          std::move(cmd),
          [](ten::axis_env_t &axis_env, std::unique_ptr<ten::cmd_result_t> result,
             ten::error_t *err) {
            axis_env.return_result_directly(std::move(result));
          });
    }
  }

 private:
  std::string name_;
};

class test_app_1 : public ten::app_t {
 public:
  void on_configure(ten::axis_env_t &axis_env) override {
    bool rc = axis_env.init_property_from_json(
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

    axis_env.on_configure_done();
  }
};

class test_app_2 : public ten::app_t {
 public:
  void on_configure(ten::axis_env_t &axis_env) override {
    bool rc = axis_env.init_property_from_json(
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

    axis_env.on_configure_done();
  }
};

ten::app_t *app1 = nullptr;
ten::app_t *app2 = nullptr;

void *app_thread_1_main(axis_UNUSED void *args) {
  app1 = new test_app_1();
  app1->run(true);
  axis_LOGD("Wait app1 thread.");
  app1->wait();
  delete app1;
  app1 = nullptr;

  return nullptr;
}

void *app_thread_2_main(axis_UNUSED void *args) {
  app2 = new test_app_2();
  app2->run(true);
  axis_LOGD("Wait app2 thread.");
  app2->wait();
  delete app2;
  app2 = nullptr;

  return nullptr;
}

axis_CPP_REGISTER_ADDON_AS_EXTENSION(graph_id_basic__extension, test_extension);

}  // namespace

TEST(ExtensionTest, GraphNameBasic) {  // NOLINT
  auto *app_thread_2 =
      axis_thread_create("app thread 2", app_thread_2_main, nullptr);
  auto *app_thread_1 =
      axis_thread_create("app thread 1", app_thread_1_main, nullptr);

  // extension1(app1) --> extension3(app2) --> extension2(app1) --> return
  ten::msgpack_tcp_client_t *client = nullptr;
  std::string graph_id;

  for (size_t i = 0; i < MULTIPLE_APP_SCENARIO_GRAPH_CONSTRUCTION_RETRY_TIMES;
       ++i) {
    client = new ten::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

    auto start_graph_cmd = ten::cmd_start_graph_t::create();
    start_graph_cmd->set_dest("msgpack://127.0.0.1:8001/", nullptr, nullptr,
                              nullptr);
    start_graph_cmd->set_graph_from_json(R"({
               "nodes": [{
                 "type": "extension",
                 "name": "extension1",
                 "addon": "graph_id_basic__extension",
                 "app": "msgpack://127.0.0.1:8001/",
                 "extension_group": "graph_id_basic__extension_group_1"
               },{
                 "type": "extension",
                 "name": "extension2",
                 "addon": "graph_id_basic__extension",
                 "app": "msgpack://127.0.0.1:8001/",
                 "extension_group": "graph_id_basic__extension_group_1"
               },{
                 "type": "extension",
                 "name": "extension3",
                 "addon": "graph_id_basic__extension",
                 "app": "msgpack://127.0.0.1:8002/",
                 "extension_group": "graph_id_basic__extension_group_2"
               }],
               "connections": [{
                 "app": "msgpack://127.0.0.1:8001/",
                 "extension": "extension1",
                 "cmd": [{
                   "name": "send_message",
                   "dest": [{
                     "app": "msgpack://127.0.0.1:8002/",
                     "extension_group": "graph_id_basic__extension_group_2",
                     "extension": "extension3"
                   }]
                 }]
               },{
                 "app": "msgpack://127.0.0.1:8002/",
                 "extension": "extension3",
                 "cmd": [{
                   "name": "send_message",
                   "dest": [{
                     "app": "msgpack://127.0.0.1:8001/",
                     "extension_group": "graph_id_basic__extension_group_1",
                     "extension": "extension2"
                   }]
                 }]
               }]
           })");

    auto cmd_result =
        client->send_cmd_and_recv_result(std::move(start_graph_cmd));

    if (cmd_result) {
      axis_test::check_status_code(cmd_result, axis_STATUS_CODE_OK);
      graph_id = cmd_result->get_property_string("detail");

      break;
    } else {
      delete client;
      client = nullptr;

      // To prevent from busy re-trying.
      axis_sleep(10);
    }
  }

  axis_ASSERT(client, "Failed to connect to the TEN app.");

  // Send data to extension_1, it will return from extension_2 with json
  // result.
  auto send_message_cmd = ten::cmd_t::create("send_message");
  send_message_cmd->set_dest("msgpack://127.0.0.1:8001/", nullptr,
                             "graph_id_basic__extension_group_1", "extension1");

  auto cmd_result =
      client->send_cmd_and_recv_result(std::move(send_message_cmd));

  axis_test::check_detail_with_json(cmd_result, R"({"id": 1, "name": "aa"})");

  // Send data to extension_3, it will return from extension_2 with json
  // result.
  auto *client2 = new ten::msgpack_tcp_client_t("msgpack://127.0.0.1:8002/");

  send_message_cmd = ten::cmd_t::create("send_message");
  send_message_cmd->set_dest("msgpack://127.0.0.1:8002/", graph_id.c_str(),
                             "graph_id_basic__extension_group_2", "extension3");

  // It must be sent directly to 127.0.0.1:8002, not 127.0.0.1:8001
  cmd_result = client2->send_cmd_and_recv_result(std::move(send_message_cmd));

  axis_test::check_detail_with_json(cmd_result, R"({"id": 1, "name": "aa"})");

  send_message_cmd = ten::cmd_t::create("send_message");
  send_message_cmd->set_dest("msgpack://127.0.0.1:8001/", graph_id.c_str(),
                             "graph_id_basic__extension_group_1", "extension2");

  cmd_result = client->send_cmd_and_recv_result(std::move(send_message_cmd));

  axis_test::check_detail_with_json(cmd_result, R"({"id": 1, "name": "aa"})");

  delete client;
  delete client2;

  if (app1 != nullptr) {
    app1->close();
  }

  if (app2 != nullptr) {
    app2->close();
  }

  axis_thread_join(app_thread_1, -1);
  axis_thread_join(app_thread_2, -1);
}
