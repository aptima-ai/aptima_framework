//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
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
    // In a scenario which contains multiple TEN app, the construction of a
    // graph might failed because not all TEN app has already been launched
    // successfully.
    //
    //     client -> (connect cmd) -> TEN app 1 ... TEN app 2
    //                                    o             x
    //
    // In this case, the newly constructed engine in the app 1 would be closed,
    // and the client would see that the connection has be dropped. After that,
    // the client could retry to send the 'start_graph' command again to inform
    // the TEN app to build the graph again.
    //
    // Therefore, the closing of an engine could _not_ cause the closing of the
    // app, and that's why the following 'long_running_mode' has been set.
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

TEN_CPP_REGISTER_ADDON_AS_EXTENSION_GROUP(
    multi_app_concurrent__extension_group_1, test_extension_group_1);
TEN_CPP_REGISTER_ADDON_AS_EXTENSION_GROUP(
    multi_app_concurrent__extension_group_2, test_extension_group_2);

void *client_thread_main(TEN_UNUSED void *args) {
  ten::msgpack_tcp_client_t *client = nullptr;

  // In a scenario which contains multiple TEN app, the construction of a
  // graph might failed because not all TEN app has already been launched
  // successfully.
  //
  //     client -> (connect cmd) -> TEN app 1 ... TEN app 2
  //                                    o             x
  //
  // In this case, the newly constructed engine in the app 1 would be closed,
  // and the client would see that the connection has be dropped. After that,
  // the client could retry to send the 'start_graph' command again to inform
  // the TEN app to build the graph again.
  for (size_t i = 0; i < MULTIPLE_APP_SCENARIO_GRAPH_CONSTRUCTION_RETRY_TIMES;
       ++i) {
    // Create a client and connect to the app.
    client = new ten::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

    // Send graph.
    nlohmann::json resp = client->send_json_and_recv_resp_in_json(
        R"({
             "_ten": {
               "type": "start_graph",
               "seq_id": "55",
               "nodes": [{
                 "type": "extension_group",
                 "name": "test extension group 1",
                 "addon": "multi_app_concurrent__extension_group_1",
                 "app": "msgpack://127.0.0.1:8001/"
               },{
                 "type": "extension_group",
                 "name": "test extension group 2",
                 "addon": "multi_app_concurrent__extension_group_2",
                 "app": "msgpack://127.0.0.1:8002/"
               }],
               "connections": [{
                 "app": "msgpack://127.0.0.1:8001/",
                 "extension_group": "test extension group 1",
                 "extension": "test extension 1",
                 "cmd": [{
                   "name": "hello_world",
                   "dest": [{
                     "app": "msgpack://127.0.0.1:8002/",
                     "extension_group": "test extension group 2",
                     "extension": "test extension 2"
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

  // Send a user-defined 'hello world' command.
  nlohmann::json resp = client->send_json_and_recv_resp_in_json(
      R"({
           "_ten": {
             "name": "hello_world",
             "seq_id": "137",
             "dest": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "test extension group 1",
               "extension": "test extension 1"
             }]
           }
         })"_json);
  ten_test::check_result_is(resp, "137", TEN_STATUS_CODE_OK,
                            "hello world, too");
  delete client;

  return nullptr;
}

}  // namespace

TEST(ExtensionTest, DISABLED_MultiAppConcurrent) {  // NOLINT
  // Start app.
  auto *app_thread_2 =
      ten_thread_create("app thread 2", app_thread_2_main, nullptr);
  auto *app_thread_1 =
      ten_thread_create("app thread 1", app_thread_1_main, nullptr);

  ten_sleep(300);

  std::vector<ten_thread_t *> client_threads;

  for (size_t i = 0; i < ONE_ENGINE_ONE_CLIENT_CONCURRENT_CNT; ++i) {
    auto *client_thread =
        ten_thread_create("client_thread_main", client_thread_main, nullptr);

    client_threads.push_back(client_thread);
  }

  for (ten_thread_t *client_thread : client_threads) {
    ten_thread_join(client_thread, -1);
  }

  // Because the closing of an engine would _not_ cause the closing of the app,
  // so we have to explicitly close the app.
  ten::msgpack_tcp_client_t::close_app("msgpack://127.0.0.1:8001/");

  // Because the closing of an engine would _not_ cause the closing of the app,
  // so we have to explicitly close the app.
  ten::msgpack_tcp_client_t::close_app("msgpack://127.0.0.1:8002/");

  ten_thread_join(app_thread_1, -1);
  ten_thread_join(app_thread_2, -1);
}
