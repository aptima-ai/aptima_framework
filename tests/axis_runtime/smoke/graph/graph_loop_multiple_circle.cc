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
#include "axis_runtime/binding/cpp/detail/axis_env.h"
#include "axis_utils/lib/thread.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/axis_runtime/smoke/util/binding/cpp/check.h"

#define LOOP_CNT 2

namespace {

/**
 *
 * client --> A --> B --> C
 *                  ^     |
 *                  |     V
 *                  <---- D
 *
 */
class test_extension : public ten::extension_t {
 public:
  explicit test_extension(const char *name)
      : ten::extension_t(name), name_(name) {}

  void on_init(ten::axis_env_t &axis_env) override {
    value_ = axis_env.get_property_int32("value");
    axis_env.on_init_done();
  }

  void on_cmd(ten::axis_env_t &axis_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (cmd->get_name() == "sum") {
      nlohmann::json json = nlohmann::json::parse(cmd->get_property_to_json());

      if (counter_ == LOOP_CNT) {
        auto cmd_result = ten::cmd_result_t::create(axis_STATUS_CODE_OK);
        cmd_result->set_property_from_json("detail", json.dump().c_str());
        axis_env.return_result(std::move(cmd_result), std::move(cmd));
      } else {
        counter_++;

        if (!json.contains("total")) {
          json["total"] = "0";
        }
        int total = std::stoi(json.value("total", ""));
        total += value_;

        json["total"] = std::to_string(total);

        axis_UNUSED bool const rc =
            cmd->set_property_from_json(nullptr, json.dump().c_str());
        axis_ASSERT(rc, "Should not happen.");

        axis_env.send_cmd(std::move(cmd));
        return;
      }
    }
  }

 private:
  const std::string name_;
  int value_;
  int counter_ = 0;
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

axis_CPP_REGISTER_ADDON_AS_EXTENSION(graph_loop_multiple_circle__extension,
                                    test_extension);

}  // namespace

TEST(ExtensionTest, GraphLoopMultipleCircle) {  // NOLINT
  // Start app.
  auto *app_thread =
      axis_thread_create("app thread", test_app_thread_main, nullptr);

  // Create a client and connect to the app.
  auto *client = new ten::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

  auto start_graph_cmd = ten::cmd_start_graph_t::create();
  start_graph_cmd->set_dest("msgpack://127.0.0.1:8001/", nullptr, nullptr,
                            nullptr);
  start_graph_cmd->set_graph_from_json(R"({
             "nodes": [{
               "type": "extension",
               "name": "A",
               "addon": "graph_loop_multiple_circle__extension",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "graph_loop_multiple_circle__extension_group",
               "property": {
                 "value": 0
               }
             },{
               "type": "extension",
               "name": "B",
               "addon": "graph_loop_multiple_circle__extension",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "graph_loop_multiple_circle__extension_group",
               "property": {
                 "value": 1
               }
             },{
               "type": "extension",
               "name": "C",
               "addon": "graph_loop_multiple_circle__extension",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "graph_loop_multiple_circle__extension_group",
               "property": {
                 "value": 2
               }
             },{
               "type": "extension",
               "name": "D",
               "addon": "graph_loop_multiple_circle__extension",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "graph_loop_multiple_circle__extension_group",
               "property": {
                 "value": 3
               }
             }],
             "connections": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension": "A",
               "cmd": [{
                 "name": "sum",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension": "B"
                 }]
               }]
             },{
               "app": "msgpack://127.0.0.1:8001/",
               "extension": "B",
               "cmd": [{
                 "name": "sum",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension": "C"
                 }]
               }]
             },{
               "app": "msgpack://127.0.0.1:8001/",
               "extension": "C",
               "cmd": [{
                 "name": "sum",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension": "D"
                 }]
               }]
             },{
               "app": "msgpack://127.0.0.1:8001/",
               "extension": "D",
               "cmd": [{
                 "name": "sum",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension": "B"
                 }]
               }]
             }]
         })");
  auto cmd_result =
      client->send_cmd_and_recv_result(std::move(start_graph_cmd));
  axis_test::check_status_code(cmd_result, axis_STATUS_CODE_OK);
  auto sum_cmd = ten::cmd_t::create("sum");
  sum_cmd->set_dest("msgpack://127.0.0.1:8001/", nullptr,
                    "graph_loop_multiple_circle__extension_group", "A");
  cmd_result = client->send_cmd_and_recv_result(std::move(sum_cmd));
  axis_test::check_status_code(cmd_result, axis_STATUS_CODE_OK);

  nlohmann::json detail =
      nlohmann::json::parse(cmd_result->get_property_to_json("detail"));
  EXPECT_EQ((1 + 2 + 3) * LOOP_CNT,
            std::stoi(detail["total"].get<std::string>()));

  delete client;

  axis_thread_join(app_thread, -1);
}
