//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "include_internal/aptima_runtime/binding/cpp/aptima.h"
#include "aptima_utils/lib/thread.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/aptima_runtime/smoke/util/binding/cpp/check.h"

namespace {

/**
 *               |--> B -|
 * client --> A -|       |--> D
 *               |--> C -|
 */
class test_extension : public aptima::extension_t {
 public:
  explicit test_extension(const char *name)
      : aptima::extension_t(name), name_(name) {}

  void on_init(aptima::aptima_env_t &aptima_env) override {
    is_leaf_node_ = aptima_env.get_property_bool("is_leaf");
    aptima_env.on_init_done();
  }

  void on_cmd(aptima::aptima_env_t &aptima_env,
              std::unique_ptr<aptima::cmd_t> cmd) override {
    nlohmann::json json = nlohmann::json::parse(cmd->get_property_to_json());

    if (is_leaf_node_) {
      json["return_from"] = name_;
      auto cmd_result = aptima::cmd_result_t::create(aptima_STATUS_CODE_OK);
      cmd_result->set_property_from_json("detail", json.dump().c_str());
      aptima_env.return_result(std::move(cmd_result), std::move(cmd));
      return;
    }

    std::vector<std::string> edges = {"B", "C"};
    if (cmd->get_name() == "send") {
      json["from"] = name_;
      if (std::find(edges.begin(), edges.end(), name_) != edges.end()) {
        json["source"] = name_;
      }

      aptima_UNUSED bool const rc =
          cmd->set_property_from_json(nullptr, json.dump().c_str());
      aptima_ASSERT(rc, "Should not happen.");

      aptima_env.send_cmd(
          std::move(cmd),
          [this, edges](aptima::aptima_env_t &aptima_env,
                        std::unique_ptr<aptima::cmd_result_t> result,
                        aptima::error_t *err) {
            nlohmann::json json =
                nlohmann::json::parse(result->get_property_to_json());
            receive_count++;

            nlohmann::json detail = json["detail"];
            if (detail.type() == nlohmann::json::value_t::string) {
              detail = nlohmann::json::parse(json.value("detail", "{}"));
            }

            detail["return_from"] = name_;
            detail["success"] = true;
            detail["receive_count"] = receive_count;

            if (std::find(edges.begin(), edges.end(), name_) != edges.end()) {
              detail["success"] = (name_ == detail["source"]);
            }

            result->set_property_from_json("detail", detail.dump().c_str());

            aptima_env.return_result_directly(std::move(result));
          });
    }
  }

 private:
  const std::string name_;
  bool is_leaf_node_{};
  int receive_count = 0;
};

class test_app : public aptima::app_t {
 public:
  void on_configure(aptima::aptima_env_t &aptima_env) override {
    bool rc = aptima_env.init_property_from_json(
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

    aptima_env.on_configure_done();
  }
};

void *test_app_thread_main(aptima_UNUSED void *args) {
  auto *app = new test_app();
  app->run();
  delete app;

  return nullptr;
}

aptima_CPP_REGISTER_ADDON_AS_EXTENSION(
    graph_polygon_in_one_app_return_all_2__extension, test_extension);

}  // namespace

TEST(ExtensionTest, GraphPolygonInOneAppReturnAll2) {  // NOLINT
  // Start app.
  auto *app_thread =
      aptima_thread_create("app thread", test_app_thread_main, nullptr);

  // Create a client and connect to the app.
  auto *client = new aptima::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

  auto start_graph_cmd = aptima::cmd_start_graph_t::create();
  start_graph_cmd->set_dest("msgpack://127.0.0.1:8001/", nullptr, nullptr,
                            nullptr);
  start_graph_cmd->set_graph_from_json(R"({
             "nodes": [{
               "type": "extension",
               "name": "A",
               "addon": "graph_polygon_in_one_app_return_all_2__extension",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "graph_polygon_in_one_app_return_all_2__extension_group",
               "property": {
                 "is_leaf": false
                }
             },{
               "type": "extension",
               "name": "B",
               "addon": "graph_polygon_in_one_app_return_all_2__extension",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "graph_polygon_in_one_app_return_all_2__extension_group",
               "property": {
                 "is_leaf": false
                }
             },{
               "type": "extension",
               "name": "C",
               "addon": "graph_polygon_in_one_app_return_all_2__extension",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "graph_polygon_in_one_app_return_all_2__extension_group",
               "property": {
                 "is_leaf": false
                }
             },{
               "type": "extension",
               "name": "D",
               "addon": "graph_polygon_in_one_app_return_all_2__extension",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "graph_polygon_in_one_app_return_all_2__extension_group",
               "property": {
                 "is_leaf": true
                }
             }],
             "connections": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension": "A",
               "cmd": [{
                 "name": "send",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension": "B"
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension": "C"
                 }]
               }]
             },{
               "app": "msgpack://127.0.0.1:8001/",
               "extension": "B",
               "cmd": [{
                 "name": "send",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension": "D"
                 }]
               }]
             },{
               "app": "msgpack://127.0.0.1:8001/",
               "extension": "C",
               "cmd": [{
                 "name": "send",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension": "D"
                 }]
               }]
             },{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "graph_polygon_in_one_app_return_all_2__extension_group",
               "extension": "D"
             }]
         })");
  auto cmd_result =
      client->send_cmd_and_recv_result(std::move(start_graph_cmd));
  aptima_test::check_status_code(cmd_result, aptima_STATUS_CODE_OK);
  auto send_cmd = aptima::cmd_t::create("send");
  send_cmd->set_dest("msgpack://127.0.0.1:8001/", nullptr,
                     "graph_polygon_in_one_app_return_all_2__extension_group",
                     "A");
  cmd_result = client->send_cmd_and_recv_result(std::move(send_cmd));
  aptima_test::check_status_code(cmd_result, aptima_STATUS_CODE_OK);
  nlohmann::json detail =
      nlohmann::json::parse(cmd_result->get_property_to_json("detail"));

  EXPECT_EQ(detail["return_from"], "A");
  EXPECT_EQ(detail["success"], true);
  EXPECT_EQ(detail["receive_count"], 1);

  delete client;

  aptima_thread_join(app_thread, -1);
}
