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

class test_extension_1 : public ten::extension_t {
 public:
  explicit test_extension_1(const std::string &name) : ten::extension_t(name) {}

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    nlohmann::json json = nlohmann::json::parse(cmd->to_json());
    if (json["_ten"]["name"] == "hello_world") {
      ten_env.send_cmd(std::move(cmd));
      return;
    }
  }
};

class test_extension_2 : public ten::extension_t {
 public:
  explicit test_extension_2(const std::string &name) : ten::extension_t(name) {}

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    nlohmann::json json = nlohmann::json::parse(cmd->to_json());
    if (json["_ten"]["name"] == "hello_world") {
      ten_env.send_cmd(std::move(cmd));
      return;
    }
  }
};

class test_extension_3 : public ten::extension_t {
 public:
  explicit test_extension_3(const std::string &name) : ten::extension_t(name) {}

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    nlohmann::json json = nlohmann::json::parse(cmd->to_json());
    if (json["_ten"]["name"] == "hello_world") {
      ten_env.send_cmd(std::move(cmd));
      return;
    }
  }
};

class test_extension_4 : public ten::extension_t {
 public:
  explicit test_extension_4(const std::string &name) : ten::extension_t(name) {}

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    nlohmann::json json = nlohmann::json::parse(cmd->to_json());
    if (json["_ten"]["name"] == "hello_world") {
      auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
      cmd_result->set_property("detail", "hello world, too");
      ten_env.return_result(std::move(cmd_result), std::move(cmd));
    }
  }
};

class test_extension_group_1 : public ten::extension_group_t {
 public:
  explicit test_extension_group_1(const std::string &name)
      : ten::extension_group_t(name) {}

  void on_create_extensions(ten::ten_env_t &ten_env) override {
    std::vector<ten::extension_t *> extensions;
    extensions.push_back(new test_extension_1("test extension 1"));
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
    extensions.push_back(new test_extension_2("test extension 2"));
    extensions.push_back(new test_extension_3("test extension 3"));
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
    extensions.push_back(new test_extension_4("test extension 4"));
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
  void on_init(ten::ten_env_t &ten_env) override {
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

    ten_env.on_init_done();
  }
};

class test_app_2 : public ten::app_t {
 public:
  void on_init(ten::ten_env_t &ten_env) override {
    bool rc = ten_env.init_property_from_json(
        // clang-format off
                 R"({
                      "_ten": {
                        "uri": "msgpack://127.0.0.1:8002/",
                        "one_event_loop_per_engine": true,
                        "long_running_mode": true,
                        "log_level": 2
                      }
                    })"
        // clang-format on
    );
    ASSERT_EQ(rc, true);

    ten_env.on_init_done();
  }
};

class test_app_3 : public ten::app_t {
 public:
  void on_init(ten::ten_env_t &ten_env) override {
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

    ten_env.on_init_done();
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

}  // namespace

TEN_CPP_REGISTER_ADDON_AS_EXTENSION_GROUP(
    graph_y_shape_in_multi_app__extension_group_1, test_extension_group_1);
TEN_CPP_REGISTER_ADDON_AS_EXTENSION_GROUP(
    graph_y_shape_in_multi_app__extension_group_2, test_extension_group_2);
TEN_CPP_REGISTER_ADDON_AS_EXTENSION_GROUP(
    graph_y_shape_in_multi_app__extension_group_3, test_extension_group_3);

TEST(ExtensionTest, GraphYShapeInMultiApp) {  // NOLINT
  // Start app.
  auto *app_thread_3 =
      ten_thread_create("app thread 3", app_thread_3_main, nullptr);
  auto *app_thread_2 =
      ten_thread_create("app thread 2", app_thread_2_main, nullptr);
  auto *app_thread_1 =
      ten_thread_create("app thread 1", app_thread_1_main, nullptr);

  ten_sleep(300);

  // Create a client and connect to the app.
  ten::msgpack_tcp_client_t *client = nullptr;
  std::string graph_name;

  for (size_t i = 0; i < MULTIPLE_APP_SCENARIO_GRAPH_CONSTRUCTION_RETRY_TIMES;
       ++i) {
    client = new ten::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

    // Send graph.
    nlohmann::json resp = client->send_json_and_recv_resp_in_json(
        R"({
             "_ten": {
               "type": "start_graph",
               "seq_id": "55",
               "nodes": [{
                 "type": "extension_group",
                 "name": "graph_y_shape_in_multi_app__extension_group_1",
                 "addon": "graph_y_shape_in_multi_app__extension_group_1",
                 "app": "msgpack://127.0.0.1:8001/"
               },{
                 "type": "extension_group",
                 "name": "graph_y_shape_in_multi_app__extension_group_2",
                 "addon": "graph_y_shape_in_multi_app__extension_group_2",
                 "app": "msgpack://127.0.0.1:8002/"
               },{
                 "type": "extension_group",
                 "name": "graph_y_shape_in_multi_app__extension_group_3",
                 "addon": "graph_y_shape_in_multi_app__extension_group_3",
                 "app": "msgpack://127.0.0.1:8003/"
               }],
               "connections": [{
                 "app": "msgpack://127.0.0.1:8001/",
                 "extension_group": "graph_y_shape_in_multi_app__extension_group_1",
                 "extension": "test extension 1",
                 "cmd": [{
                   "name": "hello_world",
                   "dest": [{
                     "app": "msgpack://127.0.0.1:8002/",
                     "extension_group": "graph_y_shape_in_multi_app__extension_group_2",
                     "extension": "test extension 3"
                   }]
                 }]
               },{
                 "app": "msgpack://127.0.0.1:8002/",
                 "extension_group": "graph_y_shape_in_multi_app__extension_group_2",
                 "extension": "test extension 2",
                 "cmd": [{
                    "name": "hello_world",
                    "dest": [{
                       "app": "msgpack://127.0.0.1:8002/",
                       "extension_group": "graph_y_shape_in_multi_app__extension_group_2",
                       "extension": "test extension 3"
                    }]
                 }]
               },{
                "app": "msgpack://127.0.0.1:8002/",
                "extension_group": "graph_y_shape_in_multi_app__extension_group_2",
                "extension": "test extension 3",
                "cmd": [{
                   "name": "hello_world",
                   "dest": [{
                     "app": "msgpack://127.0.0.1:8003/",
                     "extension_group": "graph_y_shape_in_multi_app__extension_group_3",
                     "extension": "test extension 4"
                   }]
                 }]
               }]
             }
          })"_json);

    if (!resp.empty()) {
      ten_test::check_status_code_is(resp, TEN_STATUS_CODE_OK);
      graph_name = resp["detail"];

      break;
    } else {
      delete client;
      client = nullptr;

      // To prevent from busy re-trying.
      ten_sleep(10);
    }
  }

  TEN_ASSERT(client, "Failed to connect to the TEN app.");

  // Send a user-defined 'hello world' command to 'extension 1'.
  nlohmann::json resp = client->send_json_and_recv_resp_in_json(
      R"({
           "_ten": {
             "name": "hello_world",
             "seq_id": "137",
             "dest":[{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "graph_y_shape_in_multi_app__extension_group_1",
               "extension": "test extension 1"
             }]
           }
         })"_json);
  ten_test::check_result_is(resp, "137", TEN_STATUS_CODE_OK,
                            "hello world, too");

  // Send a user-defined 'hello world' command to 'extension 2'.
  auto *client2 = new ten::msgpack_tcp_client_t("msgpack://127.0.0.1:8002/");

  nlohmann::json request2 =
      R"({
           "_ten": {
             "name": "hello_world",
             "seq_id": "138",
             "dest":[{
               "app": "msgpack://127.0.0.1:8002/",
               "extension_group": "graph_y_shape_in_multi_app__extension_group_2",
               "extension": "test extension 2"
             }]
           }
         })"_json;
  request2["_ten"]["dest"][0]["graph"] = graph_name;
  resp = client2->send_json_and_recv_resp_in_json(request2);

  ten_test::check_result_is(resp, "138", TEN_STATUS_CODE_OK,
                            "hello world, too");

  delete client;
  delete client2;

  ten::msgpack_tcp_client_t::close_app("msgpack://127.0.0.1:8001/");
  ten::msgpack_tcp_client_t::close_app("msgpack://127.0.0.1:8002/");
  ten::msgpack_tcp_client_t::close_app("msgpack://127.0.0.1:8003/");

  ten_thread_join(app_thread_1, -1);
  ten_thread_join(app_thread_2, -1);
  ten_thread_join(app_thread_3, -1);
}
