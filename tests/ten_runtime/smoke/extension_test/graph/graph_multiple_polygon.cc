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
#include "ten_utils/lib/time.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/common/constant.h"
#include "tests/ten_runtime/smoke/extension_test/util/binding/cpp/check.h"

namespace {

/**
 *                  |--> B --|
 *               |--|        |--> F --|
 *               |  |--> C --|        |
 * client --> A -|                    |--> H
 *               |  |--> D --|        |
 *               |--|        |--> G --|
 *                  |--> E --|
 *
 * App 8001 : A,B,C,D
 * App 8002 : E,G
 * App 8003 : F,H
 */
class test_extension : public ten::extension_t {
 public:
  explicit test_extension(const std::string &name, bool is_leaf)
      : ten::extension_t(name), name_(name), is_leaf_node_(is_leaf) {}

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    nlohmann::json json = nlohmann::json::parse(cmd->to_json());

    if (is_leaf_node_) {
      json["return_from"] = name_;
      auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
      cmd_result->set_property_from_json("detail", json.dump().c_str());
      ten_env.return_result(std::move(cmd_result), std::move(cmd));
      return;
    }

    std::vector<std::string> edges = {"B", "C", "D", "E", "F", "G"};
    if (json["_ten"]["name"] == "send") {
      json["from"] = name_;
      if (std::find(edges.begin(), edges.end(), name_) != edges.end()) {
        json[name_] = name_;
      }

      TEN_UNUSED bool const rc = cmd->from_json(json.dump().c_str());
      TEN_ASSERT(rc, "Should not happen.");

      ten_env.send_cmd(
          std::move(cmd),
          [this, edges](ten::ten_env_t &ten_env,
                        std::unique_ptr<ten::cmd_result_t> status) {
            nlohmann::json json = nlohmann::json::parse(status->to_json());

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

            status->set_property_from_json("detail", detail.dump().c_str());

            ten_env.return_result_directly(std::move(status));
          });
    }
  }

 private:
  const std::string name_;
  const bool is_leaf_node_;
  int received_count = 0;
  int received_success_count = 0;
};

class test_extension_group_1 : public ten::extension_group_t {
 public:
  explicit test_extension_group_1(const std::string &name)
      : ten::extension_group_t(name) {}

  void on_create_extensions(ten::ten_env_t &ten_env) override {
    std::vector<ten::extension_t *> extensions;
    extensions.push_back(new test_extension("A", false));
    extensions.push_back(new test_extension("B", false));
    extensions.push_back(new test_extension("C", false));
    extensions.push_back(new test_extension("D", false));
    ten_env.on_create_extensions_done(extensions);
  }

  void on_destroy_extensions(
      ten::ten_env_t &ten_env,
      const std::vector<ten::extension_t *> &extensions) override {
    for (auto *extension : extensions) {
      delete extension;
    }
    ten_env.on_destroy_extensions_done();
  }
};

class test_extension_group_2 : public ten::extension_group_t {
 public:
  explicit test_extension_group_2(const std::string &name)
      : ten::extension_group_t(name) {}

  void on_create_extensions(ten::ten_env_t &ten_env) override {
    std::vector<ten::extension_t *> extensions;
    extensions.push_back(new test_extension("E", false));
    extensions.push_back(new test_extension("G", false));
    ten_env.on_create_extensions_done(extensions);
  }

  void on_destroy_extensions(
      ten::ten_env_t &ten_env,
      const std::vector<ten::extension_t *> &extensions) override {
    for (auto *extension : extensions) {
      delete extension;
    }
    ten_env.on_destroy_extensions_done();
  }
};

class test_extension_group_3 : public ten::extension_group_t {
 public:
  explicit test_extension_group_3(const std::string &name)
      : ten::extension_group_t(name) {}

  void on_create_extensions(ten::ten_env_t &ten_env) override {
    std::vector<ten::extension_t *> extensions;
    extensions.push_back(new test_extension("F", false));
    extensions.push_back(new test_extension("H", true));
    ten_env.on_create_extensions_done(extensions);
  }

  void on_destroy_extensions(
      ten::ten_env_t &ten_env,
      const std::vector<ten::extension_t *> &extensions) override {
    for (auto *extension : extensions) {
      delete extension;
    }
    ten_env.on_destroy_extensions_done();
  }
};

class test_app_1 : public ten::app_t {
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

class test_app_2 : public ten::app_t {
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

class test_app_3 : public ten::app_t {
 public:
  void on_configure(ten::ten_env_t &ten_env) override {
    bool rc = ten_env.init_property_from_json(
        // clang-format off
                 R"({
                      "_ten": {
                        "uri": "msgpack://127.0.0.1:8003/",
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
  auto *app = new test_app_1();
  app->run();
  delete app;

  return nullptr;
}

void *app_thread_2_main(TEN_UNUSED void *args) {
  auto *app = new test_app_2();
  app->run();
  delete app;

  return nullptr;
}

void *app_thread_3_main(TEN_UNUSED void *args) {
  auto *app = new test_app_3();
  app->run();
  delete app;

  return nullptr;
}

TEN_CPP_REGISTER_ADDON_AS_EXTENSION_GROUP(
    graph_multiple_polygon__extension_group_1, test_extension_group_1);
TEN_CPP_REGISTER_ADDON_AS_EXTENSION_GROUP(
    graph_multiple_polygon__extension_group_2, test_extension_group_2);
TEN_CPP_REGISTER_ADDON_AS_EXTENSION_GROUP(
    graph_multiple_polygon__extension_group_3, test_extension_group_3);

}  // namespace

TEST(ExtensionTest, GraphMultiplePolygon) {  // NOLINT
  // Start app.
  auto *app_thread3 =
      ten_thread_create("app thread 3", app_thread_3_main, nullptr);
  auto *app_thread2 =
      ten_thread_create("app thread 2", app_thread_2_main, nullptr);
  auto *app_thread1 =
      ten_thread_create("app thread 1", app_thread_1_main, nullptr);

  ten_sleep(300);

  // Create a client and connect to the app.
  ten::msgpack_tcp_client_t *client = nullptr;

  for (size_t i = 0; i < MULTIPLE_APP_SCENARIO_GRAPH_CONSTRUCTION_RETRY_TIMES;
       ++i) {
    client = new ten::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

    nlohmann::json resp = client->send_json_and_recv_resp_in_json(
        R"({
             "_ten": {
               "type": "start_graph",
               "seq_id": "55",
               "dest": [{
                 "app": "msgpack://127.0.0.1:8001/"
               }],
               "nodes": [{
                 "type": "extension_group",
                 "name": "graph_multiple_polygon_1",
                 "addon": "graph_multiple_polygon__extension_group_1",
                 "app": "msgpack://127.0.0.1:8001/"
               },{
                 "type": "extension_group",
                 "name": "graph_multiple_polygon_2",
                 "addon": "graph_multiple_polygon__extension_group_2",
                 "app": "msgpack://127.0.0.1:8002/"
               },{
                 "type": "extension_group",
                 "name": "graph_multiple_polygon_3",
                 "addon": "graph_multiple_polygon__extension_group_3",
                 "app": "msgpack://127.0.0.1:8003/"
               }],
               "connections": [{
                 "app": "msgpack://127.0.0.1:8001/",
                 "extension_group": "graph_multiple_polygon_1",
                 "extension": "A",
                 "cmd": [{
                   "name": "send",
                   "dest": [{
                     "app": "msgpack://127.0.0.1:8001/",
                     "extension_group": "graph_multiple_polygon_1",
                     "extension": "B"
                   },{
                     "app": "msgpack://127.0.0.1:8001/",
                     "extension_group": "graph_multiple_polygon_1",
                     "extension": "C"
                   },{
                     "app": "msgpack://127.0.0.1:8001/",
                     "extension_group": "graph_multiple_polygon_1",
                     "extension": "D"
                   },{
                     "app": "msgpack://127.0.0.1:8002/",
                     "extension_group": "graph_multiple_polygon_2",
                     "extension": "E"
                   }]
                 }]
               },{
                 "app": "msgpack://127.0.0.1:8001/",
                 "extension_group": "graph_multiple_polygon_1",
                 "extension": "B",
                 "cmd": [{
                   "name": "send",
                   "dest": [{
                     "app": "msgpack://127.0.0.1:8003/",
                     "extension_group": "graph_multiple_polygon_3",
                     "extension": "F"
                   }]
                 }]
               },{
                 "app": "msgpack://127.0.0.1:8001/",
                 "extension_group": "graph_multiple_polygon_1",
                 "extension": "C",
                 "cmd": [{
                   "name": "send",
                   "dest": [{
                     "app": "msgpack://127.0.0.1:8003/",
                     "extension_group": "graph_multiple_polygon_3",
                     "extension": "F"
                   }]
                 }]
               },{
                 "app": "msgpack://127.0.0.1:8001/",
                 "extension_group": "graph_multiple_polygon_1",
                 "extension": "D",
                 "cmd": [{
                   "name": "send",
                   "dest": [{
                     "app": "msgpack://127.0.0.1:8002/",
                     "extension_group": "graph_multiple_polygon_2",
                     "extension": "G"
                   }]
                 }]
               },{
                 "app": "msgpack://127.0.0.1:8002/",
                 "extension_group": "graph_multiple_polygon_2",
                 "extension": "E",
                 "cmd": [{
                   "name": "send",
                   "dest": [{
                     "app": "msgpack://127.0.0.1:8002/",
                     "extension_group": "graph_multiple_polygon_2",
                     "extension": "G"
                   }]
                 }]
               },{
                 "app": "msgpack://127.0.0.1:8003/",
                 "extension_group": "graph_multiple_polygon_3",
                 "extension": "F",
                 "cmd": [{
                   "name": "send",
                   "dest": [{
                     "app": "msgpack://127.0.0.1:8003/",
                     "extension_group": "graph_multiple_polygon_3",
                     "extension": "H"
                   }]
                 }]
               },{
                 "app": "msgpack://127.0.0.1:8002/",
                 "extension_group": "graph_multiple_polygon_2",
                 "extension": "G",
                 "cmd": [{
                   "name": "send",
                   "dest": [{
                     "app": "msgpack://127.0.0.1:8003/",
                     "extension_group": "graph_multiple_polygon_3",
                     "extension": "H"
                   }]
                 }]
               }]
             }
           })"_json);

    if (!resp.empty()) {
      ten_test::check_status_code_is(resp, TEN_STATUS_CODE_OK);
      break;
    } else {
      delete client;
      client = nullptr;

      // To prevent from busy re-trying.
      ten_sleep(10);
    }
  }

  TEN_ASSERT(client, "Failed to connect to the TEN app.");

  nlohmann::json resp = client->send_json_and_recv_resp_in_json(
      R"({
           "_ten": {
             "name": "send",
             "seq_id": "137",
             "dest": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "graph_multiple_polygon_1",
               "extension": "A"
             }]
           }
         })"_json);

  nlohmann::json detail = resp["detail"];
  EXPECT_EQ(detail["return_from"], "A");
  EXPECT_EQ(detail["success"], true);
  EXPECT_EQ(detail["received_count"], 1);
  EXPECT_EQ(detail["received_success_count"], 1);

  delete client;

  ten::msgpack_tcp_client_t::close_app("msgpack://127.0.0.1:8001/");
  ten::msgpack_tcp_client_t::close_app("msgpack://127.0.0.1:8002/");
  ten::msgpack_tcp_client_t::close_app("msgpack://127.0.0.1:8003/");

  ten_thread_join(app_thread1, -1);
  ten_thread_join(app_thread2, -1);
  ten_thread_join(app_thread3, -1);
}
