//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include <nlohmann/json.hpp>
#include <vector>

#include "gtest/gtest.h"
#include "include_internal/ten_runtime/binding/cpp/ten.h"
#include "ten_utils/lib/thread.h"
#include "ten_utils/lib/time.h"
#include "ten_utils/log/log.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/common/constant.h"
#include "tests/ten_runtime/smoke/extension_test/util/binding/cpp/check.h"

namespace {

class test_extension : public ten::extension_t {
 public:
  explicit test_extension(const std::string &name)
      : ten::extension_t(name), name_(name) {}

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    nlohmann::json data = nlohmann::json::parse(cmd->to_json());

    data["send_from"] = name_;

    TEN_UNUSED bool const rc = cmd->from_json(data.dump().c_str());
    TEN_ASSERT(rc, "Should not happen.");

    // extension1(app1) -> extension3(app2) -> extension2(app1) -> return
    if (name_ == "extension2") {
      const nlohmann::json resp = {{"id", 1}, {"name", "aa"}};
      auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
      cmd_result->set_property_from_json("detail", resp.dump().c_str());
      ten_env.return_result(std::move(cmd_result), std::move(cmd));
    } else {
      ten_env.send_cmd(std::move(cmd),
                       [](ten::ten_env_t &ten_env,
                          std::unique_ptr<ten::cmd_result_t> status) {
                         ten_env.return_result_directly(std::move(status));
                       });
    }
  }

 private:
  std::string name_;
};

class test_extension_group_1 : public ten::extension_group_t {
 public:
  explicit test_extension_group_1(const std::string &name)
      : ten::extension_group_t(name) {}

  void on_create_extensions(ten::ten_env_t &ten_env) override {
    std::vector<ten::extension_t *> extensions;
    extensions.push_back(new test_extension("extension1"));
    extensions.push_back(new test_extension("extension2"));
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
    extensions.push_back(new test_extension("extension3"));
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
                        "log_level": 1
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
                        "log_level": 1
                      }
                    })"
        // clang-format on
    );
    ASSERT_EQ(rc, true);

    ten_env.on_init_done();
  }
};

ten::app_t *app1 = nullptr;
ten::app_t *app2 = nullptr;

void *app_thread_1_main(TEN_UNUSED void *args) {
  app1 = new test_app_1();
  app1->run(true);
  TEN_LOGD("Wait app1 thread.");
  app1->wait();
  delete app1;
  app1 = nullptr;

  return nullptr;
}

void *app_thread_2_main(TEN_UNUSED void *args) {
  app2 = new test_app_2();
  app2->run(true);
  TEN_LOGD("Wait app2 thread.");
  app2->wait();
  delete app2;
  app2 = nullptr;

  return nullptr;
}

TEN_CPP_REGISTER_ADDON_AS_EXTENSION_GROUP(graph_name_basic__extension_group_1,
                                          test_extension_group_1);
TEN_CPP_REGISTER_ADDON_AS_EXTENSION_GROUP(graph_name_basic__extension_group_2,
                                          test_extension_group_2);

}  // namespace

TEST(ExtensionTest, GraphNameBasic) {  // NOLINT
  auto *app_thread_2 =
      ten_thread_create("app thread 2", app_thread_2_main, nullptr);
  auto *app_thread_1 =
      ten_thread_create("app thread 1", app_thread_1_main, nullptr);

  ten_sleep(300);

  // extension1(app1) --> extension3(app2) --> extension2(app1) --> return
  ten::msgpack_tcp_client_t *client = nullptr;
  std::string graph_name;

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
                 "name": "graph_name_basic__extension_group_1",
                 "addon": "graph_name_basic__extension_group_1",
                 "app": "msgpack://127.0.0.1:8001/"
               },{
                 "type": "extension_group",
                 "name": "graph_name_basic__extension_group_2",
                 "addon": "graph_name_basic__extension_group_2",
                 "app": "msgpack://127.0.0.1:8002/"
               }],
               "connections": [{
                 "app": "msgpack://127.0.0.1:8001/",
                 "extension_group": "graph_name_basic__extension_group_1",
                 "extension": "extension1",
                 "cmd": [{
                   "name": "send_message",
                   "dest": [{
                     "app": "msgpack://127.0.0.1:8002/",
                     "extension_group": "graph_name_basic__extension_group_2",
                     "extension": "extension3"
                   }]
                 }]
               },{
                 "app": "msgpack://127.0.0.1:8002/",
                 "extension_group": "graph_name_basic__extension_group_2",
                 "extension": "extension3",
                 "cmd": [{
                   "name": "send_message",
                   "dest": [{
                     "app": "msgpack://127.0.0.1:8001/",
                     "extension_group": "graph_name_basic__extension_group_1",
                     "extension": "extension2"
                   }]
                 }]
               }]
             }
           })"_json);

    if (!resp.empty()) {
      ten_test::check_status_code_is(resp, TEN_STATUS_CODE_OK);
      graph_name = resp["detail"].get<std::string>();

      break;
    } else {
      delete client;
      client = nullptr;

      // To prevent from busy re-trying.
      ten_sleep(10);
    }
  }

  TEN_ASSERT(client, "Failed to connect to the TEN app.");

  // Send data to extension_1, it will return from extension_2 with json
  // result.
  nlohmann::json resp = client->send_json_and_recv_resp_in_json(
      R"({
           "_ten": {
             "name": "send_message",
             "dest": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "graph_name_basic__extension_group_1",
               "extension": "extension1"
             }]
           }
         })"_json);
  ten_test::check_detail_is(resp, R"({"id": 1, "name": "aa"})");

  // Send data to extension_3, it will return from extension_2 with json
  // result.
  auto *client2 = new ten::msgpack_tcp_client_t("msgpack://127.0.0.1:8002/");

  nlohmann::json command_2 =
      R"({
           "_ten": {
             "name": "send_message",
             "dest": [{
               "app": "msgpack://127.0.0.1:8002/",
               "extension_group": "graph_name_basic__extension_group_2",
               "extension": "extension3"
             }]
           }
         })"_json;
  command_2["_ten"]["dest"][0]["graph"] = graph_name;

  // It must be sent directly to 127.0.0.1:8002, not 127.0.0.1:8001
  resp = client2->send_json_and_recv_resp_in_json(command_2);
  ten_test::check_detail_is(resp, R"({"id": 1, "name": "aa"})");

  // Send data to extension_2 directly.
  nlohmann::json command_3 =
      R"({
           "_ten": {
             "name": "send_message",
             "dest": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "graph_name_basic__extension_group_1",
               "extension": "extension2"
             }]
           }
         })"_json;
  command_3["_ten"]["dest"][0]["graph"] = graph_name;
  resp = client->send_json_and_recv_resp_in_json(command_3);
  ten_test::check_detail_is(resp, R"({"id": 1, "name": "aa"})");

  delete client;
  delete client2;

  if (app1 != nullptr) {
    app1->close();
  }

  if (app2 != nullptr) {
    app2->close();
  }

  ten_thread_join(app_thread_1, -1);
  ten_thread_join(app_thread_2, -1);
}
