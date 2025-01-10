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
 *                  |--> B --|
 *               |--|        |--> F --|
 *               |  |--> C --|        |
 * client --> A -|                    |--> H
 *               |  |--> D --|        |
 *               |--|        |--> G --|
 *                  |--> E --|
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

    std::vector<std::string> edges = {"B", "C", "D", "E", "F", "G"};
    if (cmd->get_name() == "send") {
      json["from"] = name_;
      if (std::find(edges.begin(), edges.end(), name_) != edges.end()) {
        json[name_] = name_;
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
            nlohmann::json detail = json["detail"];
            if (detail.type() == nlohmann::json::value_t::string) {
              detail = nlohmann::json::parse(json.value("detail", "{}"));
            }

            if (name_ == "A") {
              received_count++;
              if (detail["success"]) {
                received_success_count++;
              }
            }

            if (name_ == "A" && received_count < 1) {
              return;
            }

            detail["received_count"] = received_count;
            detail["received_success_count"] = received_success_count;

            if (name_ == "B" || name_ == "C") {
              detail["success"] =
                  (name_ == detail[name_] && detail["return_from"] == "F");
            } else if (name_ == "D" || name_ == "E") {
              detail["success"] =
                  (name_ == detail[name_] && detail["return_from"] == "G");
            } else {
              if (std::find(edges.begin(), edges.end(), name_) != edges.end()) {
                detail["success"] = (name_ == detail[name_]);
              }
            }

            detail["return_from"] = name_;

            result->set_property_from_json("detail", detail.dump().c_str());

            aptima_env.return_result_directly(std::move(result));
          });
    }
  }

 private:
  const std::string name_;
  bool is_leaf_node_;
  int received_count = 0;
  int received_success_count = 0;
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

aptima_CPP_REGISTER_ADDON_AS_EXTENSION(graph_multiple_polygon_one_app__extension,
                                    test_extension);

}  // namespace

TEST(ExtensionTest, GraphMultiplePolygonOneApp) {  // NOLINT
  // Start app.
  auto *app_thread =
      aptima_thread_create("app thread", test_app_thread_main, nullptr);

  // Create a client and connect to the app.
  auto *client = new aptima::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");
  auto start_graph_cmd = aptima::cmd_start_graph_t::create();
  start_graph_cmd->set_graph_from_json(R"({
             "nodes": [{
               "type": "extension",
               "name": "A",
               "addon": "graph_multiple_polygon_one_app__extension",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "graph_multiple_polygon_one_app__extension_group",
               "property": {
                 "is_leaf": false
                }
             },{
               "type": "extension",
               "name": "B",
               "addon": "graph_multiple_polygon_one_app__extension",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "graph_multiple_polygon_one_app__extension_group",
               "property": {
                 "is_leaf": false
                }
             },{
               "type": "extension",
               "name": "C",
               "addon": "graph_multiple_polygon_one_app__extension",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "graph_multiple_polygon_one_app__extension_group",
               "property": {
                 "is_leaf": false
                }
             },{
               "type": "extension",
               "name": "D",
               "addon": "graph_multiple_polygon_one_app__extension",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "graph_multiple_polygon_one_app__extension_group",
               "property": {
                 "is_leaf": false
                }
             },{
               "type": "extension",
               "name": "E",
               "addon": "graph_multiple_polygon_one_app__extension",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "graph_multiple_polygon_one_app__extension_group",
               "property": {
                 "is_leaf": false
                }
             },{
               "type": "extension",
               "name": "F",
               "addon": "graph_multiple_polygon_one_app__extension",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "graph_multiple_polygon_one_app__extension_group",
               "property": {
                 "is_leaf": false
                }
             },{
               "type": "extension",
               "name": "G",
               "addon": "graph_multiple_polygon_one_app__extension",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "graph_multiple_polygon_one_app__extension_group",
               "property": {
                 "is_leaf": false
                }
             },{
               "type": "extension",
               "name": "H",
               "addon": "graph_multiple_polygon_one_app__extension",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "graph_multiple_polygon_one_app__extension_group",
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
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension": "D"
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension": "E"
                 }]
               }]
             },{
               "app": "msgpack://127.0.0.1:8001/",
               "extension": "B",
               "cmd": [{
                 "name": "send",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension": "F"
                 }]
               }]
             },{
               "app": "msgpack://127.0.0.1:8001/",
               "extension": "C",
               "cmd": [{
                 "name": "send",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension": "F"
                 }]
               }]
             },{
               "app": "msgpack://127.0.0.1:8001/",
               "extension": "D",
               "cmd": [{
                 "name": "send",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension": "G"
                 }]
               }]
             },{
               "app": "msgpack://127.0.0.1:8001/",
               "extension": "E",
               "cmd": [{
                 "name": "send",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension": "G"
                 }]
               }]
             },{
               "app": "msgpack://127.0.0.1:8001/",
               "extension": "F",
               "cmd": [{
                 "name": "send",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension": "H"
                 }]
               }]
             },{
               "app": "msgpack://127.0.0.1:8001/",
               "extension": "G",
               "cmd": [{
                 "name": "send",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension": "H"
                 }]
               }]
             }]
         })");
  auto cmd_result =
      client->send_cmd_and_recv_result(std::move(start_graph_cmd));
  aptima_test::check_status_code(cmd_result, aptima_STATUS_CODE_OK);
  auto send_cmd = aptima::cmd_t::create("send");
  send_cmd->set_dest("msgpack://127.0.0.1:8001/", nullptr,
                     "graph_multiple_polygon_one_app__extension_group", "A");
  cmd_result = client->send_cmd_and_recv_result(std::move(send_cmd));

  nlohmann::json detail =
      nlohmann::json::parse(cmd_result->get_property_to_json("detail"));
  EXPECT_EQ(detail["return_from"], "A");
  EXPECT_EQ(detail["success"], true);
  EXPECT_EQ(detail["received_count"], 1);
  EXPECT_EQ(detail["received_success_count"], 1);

  delete client;

  aptima_thread_join(app_thread, -1);
}
