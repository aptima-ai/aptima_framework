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

class ExtensionA : public ten::extension_t {
 public:
  ExtensionA() : ten::extension_t("A") {}

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    ten_env.send_cmd(std::move(cmd));
  }
};

class ExtensionB : public ten::extension_t {
 public:
  ExtensionB() : ten::extension_t("B") {}

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    nlohmann::json resp = {{"a", "b"}};

    auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
    cmd_result->set_property_from_json("detail", resp.dump().c_str());
    ten_env.return_result(std::move(cmd_result), std::move(cmd));
  }
};

class ExtensionGroupA : public ten::extension_group_t {
 public:
  explicit ExtensionGroupA(const std::string &name)
      : ten::extension_group_t(name) {}

  void on_create_extensions(ten::ten_env_t &ten_env) override {
    std::vector<ten::extension_t *> extensions;
    extensions.push_back(new ExtensionA());
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

class ExtensionGroupB : public ten::extension_group_t {
 public:
  explicit ExtensionGroupB(const std::string &name)
      : ten::extension_group_t(name) {}

  void on_create_extensions(ten::ten_env_t &ten_env) override {
    std::vector<ten::extension_t *> extensions;
    extensions.push_back(new ExtensionB());
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

class test_app_a : public ten::app_t {
 public:
  void on_init(ten::ten_env_t &ten_env) override {
    bool rc = ten_env.init_property_from_json(
        // clang-format off
                 R"({
                      "_ten": {
                        "uri": "msgpack://127.0.0.1:8001/",
                        "one_event_loop_per_engine": true,
                        "long_running_mode": true,
                        "log_level": 1
                      }
                    })"
        // clang-format on
    );
    ASSERT_EQ(rc, true);

    ten_env.on_init_done();
  }
};

TEN_CPP_REGISTER_ADDON_AS_EXTENSION_GROUP(
    engine_long_running_mode__extension_group_A, ExtensionGroupA);

test_app_a *app_a = nullptr;

void *app_thread_1_main(TEN_UNUSED void *args) {
  app_a = new test_app_a();
  app_a->run();
  delete app_a;

  return nullptr;
}

class test_app_b : public ten::app_t {
 public:
  void on_init(ten::ten_env_t &ten_env) override {
    bool rc = ten_env.init_property_from_json(
        // clang-format off
                 R"({
                      "_ten": {
                        "uri": "msgpack://127.0.0.1:8002/",
                        "long_running_mode": true,
                        "log_level": 1
                      }
                    })"
        // clang-format on
    );
    ASSERT_EQ(rc, true);

    ten_env.on_init_done();
  }
};

TEN_CPP_REGISTER_ADDON_AS_EXTENSION_GROUP(
    engine_long_running_mode__extension_group_B, ExtensionGroupB);

test_app_b *app_b = nullptr;

void *app_thread_2_main(TEN_UNUSED void *args) {
  app_b = new test_app_b();
  app_b->run();
  delete app_b;

  return nullptr;
}

}  // namespace

TEST(ExtensionTest, EngineLongRunningMode) {  // NOLINT
  // Start app.
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
               "long_running_mode": true,
               "seq_id": "55",
               "nodes": [{
                 "type": "extension_group",
                 "name": "engine_long_running_mode__extension_group_A",
                 "addon": "engine_long_running_mode__extension_group_A",
                 "app": "msgpack://127.0.0.1:8001/"
               },{
                 "type": "extension_group",
                 "name": "engine_long_running_mode__extension_group_B",
                 "addon": "engine_long_running_mode__extension_group_B",
                 "app": "msgpack://127.0.0.1:8002/"
               }],
               "connections": [{
                 "app": "msgpack://127.0.0.1:8001/",
                 "extension_group": "engine_long_running_mode__extension_group_A",
                 "extension": "A",
                 "cmd": [{
                   "name": "test",
                   "dest": [{
                     "app": "msgpack://127.0.0.1:8002/",
                     "extension_group": "engine_long_running_mode__extension_group_B",
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

  // now close connection
  delete client;

  // connect again, send request with graph_name directly
  client = new ten::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

  // Send a user-defined 'hello world' command.
  nlohmann::json request =
      R"({
           "_ten": {
             "name": "test",
             "seq_id": "137",
             "dest":[{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "engine_long_running_mode__extension_group_A",
               "extension": "A"
             }]
           }
         })"_json;
  request["_ten"]["dest"][0]["graph"] = graph_name;

  nlohmann::json resp = client->send_json_and_recv_resp_in_json(request);
  ten_test::check_result_is(resp, "137", TEN_STATUS_CODE_OK, R"({"a": "b"})");

  // Destroy the client.
  delete client;

  if (app_a != nullptr) {
    app_a->close();
  }

  if (app_b != nullptr) {
    app_b->close();
  }

  ten_thread_join(app_thread_1, -1);
  ten_thread_join(app_thread_2, -1);
}
