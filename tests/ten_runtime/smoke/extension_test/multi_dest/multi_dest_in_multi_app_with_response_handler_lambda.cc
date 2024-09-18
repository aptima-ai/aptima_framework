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

#define DEST_EXTENSION_MIN_ID 2
#define DEST_EXTENSION_MAX_ID 15

#define DEFINE_EXTENSION(N)                                                   \
  class test_extension_##N : public ten::extension_t {                        \
   public:                                                                    \
    explicit test_extension_##N(const std::string &name)                      \
        : ten::extension_t(name) {}                                           \
                                                                              \
    void on_cmd(ten::ten_env_t &ten_env,                                      \
                std::unique_ptr<ten::cmd_t> cmd) override {                   \
      nlohmann::json json = nlohmann::json::parse(cmd->to_json());            \
      if (json["_ten"]["name"] == "hello_world") {                            \
        auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);      \
        cmd_result->set_property("detail", "hello world from extension " #N); \
        ten_env.return_result(std::move(cmd_result), std::move(cmd));         \
      }                                                                       \
    }                                                                         \
  };

#define DEFINE_APP(N, port)                                                       \
  class test_app_##N : public ten::app_t {                                        \
   public:                                                                        \
    void on_configure(ten::ten_env_t &ten_env) {                                  \
      /* clang-format off */                                                    \
      bool rc = ten_env.init_property_from_json(                          \
                   "{\"_ten\": { \"uri\": \"msgpack://127.0.0.1:" #port "/\",   \
                   \"long_running_mode\": true }                                \
                   }");                                                         \
      ASSERT_EQ(rc, true); \
                                                                                  \
      /* clang-format on */                                                       \
      ten_env.on_configure_done();                                                \
    }                                                                             \
  };                                                                              \
  static void *test_app_##N##_thread_main(TEN_UNUSED void *args) {                \
    auto *app = new test_app_##N();                                               \
    app->run();                                                                   \
    delete app;                                                                   \
                                                                                  \
    return nullptr;                                                               \
  }

#define ADD_EXTENSION(N) \
  extensions.push_back(new test_extension_##N("test extension " #N));

#define DEFINE_EXTENSION_GROUP(N, A, B, C)                                     \
  class test_extension_group_##N : public ten::extension_group_t {             \
   public:                                                                     \
    explicit test_extension_group_##N(const std::string &name)                 \
        : ten::extension_group_t(name) {}                                      \
                                                                               \
    void on_create_extensions(ten::ten_env_t &ten_env) override {              \
      std::vector<ten::extension_t *> extensions;                              \
      ADD_EXTENSION(A);                                                        \
      ADD_EXTENSION(B);                                                        \
      ADD_EXTENSION(C);                                                        \
      ten_env.on_create_extensions_done(extensions);                           \
    }                                                                          \
                                                                               \
    void on_destroy_extensions(                                                \
        ten::ten_env_t &ten_env,                                               \
        const std::vector<ten::extension_t *> &extensions) override {          \
      for (auto iter = extensions.begin(); iter != extensions.end(); ++iter) { \
        delete *iter;                                                          \
      }                                                                        \
      ten_env.on_destroy_extensions_done();                                    \
    }                                                                          \
  };                                                                           \
  TEN_CPP_REGISTER_ADDON_AS_EXTENSION_GROUP(                                   \
      multi_dest_in_multi_app_with_result_handler_lambda__extension_group_##N, \
      test_extension_group_##N);

#define START_APP(N)           \
  auto test_app_##N##_thread = \
      ten_thread_create(nullptr, test_app_##N##_thread_main, nullptr);

#define WAIT_APP_TO_STOP(N) ten_thread_join(test_app_##N##_thread, -1);

namespace {

typedef enum RESPONSE {
  RESPONSE_2,
  RESPONSE_3,
  RESPONSE_4,
  RESPONSE_5,
  RESPONSE_6,
  RESPONSE_7,
  RESPONSE_8,
  RESPONSE_9,
  RESPONSE_10,
  RESPONSE_11,
  RESPONSE_12,
  RESPONSE_13,
  RESPONSE_14,
  RESPONSE_15,
} RESPONSE;

class test_extension_1 : public ten::extension_t {
 public:
  explicit test_extension_1(const std::string &name) : ten::extension_t(name) {}

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    nlohmann::json json = nlohmann::json::parse(cmd->to_json());
    if (json["_ten"]["name"] == "hello_world") {
      ten_env.send_cmd(
          std::move(cmd),
          [this](ten::ten_env_t &ten_env,
                 std::unique_ptr<ten::cmd_result_t> cmd_result) {
            this->pending_resp_num--;
            if (this->pending_resp_num == 0) {
              cmd_result->set_property("detail", "return from extension 1");
              ten_env.return_result_directly(std::move(cmd_result));
            }

            return true;
          });
    }
  }

 private:
  int pending_resp_num{1};
};

DEFINE_EXTENSION(2)
DEFINE_EXTENSION(3)
DEFINE_EXTENSION(4)
DEFINE_EXTENSION(5)
DEFINE_EXTENSION(6)
DEFINE_EXTENSION(7)
DEFINE_EXTENSION(8)
DEFINE_EXTENSION(9)
DEFINE_EXTENSION(10)
DEFINE_EXTENSION(11)
DEFINE_EXTENSION(12)
DEFINE_EXTENSION(13)
DEFINE_EXTENSION(14)
DEFINE_EXTENSION(15)

DEFINE_EXTENSION_GROUP(1, 1, 2, 3)
DEFINE_EXTENSION_GROUP(2, 4, 5, 6)
DEFINE_EXTENSION_GROUP(3, 7, 8, 9)
DEFINE_EXTENSION_GROUP(4, 10, 11, 12)
DEFINE_EXTENSION_GROUP(5, 13, 14, 15)

}  // namespace

DEFINE_APP(1, 8001)
DEFINE_APP(2, 8002)
DEFINE_APP(3, 8003)
DEFINE_APP(4, 8004)
DEFINE_APP(5, 8005)

TEST(ExtensionTest, MultiDestInMultiAppWithResponseHandlerLambda) {  // NOLINT
  // Start app.
  START_APP(1)
  START_APP(2)
  START_APP(3)
  START_APP(4)
  START_APP(5)

  // TODO(Wei): When apps are not started completely, and the client sends the
  // 'start_graph' command to them, apps could not form a complete graph (ex:
  // app 3 is not started completely yet, and app 2 tries to send the
  // 'start_graph' command to it), so we need to add a delay here, or we need to
  // design a mechanism which could tell us that the apps in question are all
  // ready to accept incoming messages.
  ten_sleep(1000);

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
               "seq_id": "55",
               "nodes": [{
                 "type": "extension_group",
                 "name": "multi_dest_in_multi_app_with_result_handler_lambda__extension_group_1",
                 "addon": "multi_dest_in_multi_app_with_result_handler_lambda__extension_group_1",
                 "app": "msgpack://127.0.0.1:8001/"
               },{
                 "type": "extension_group",
                 "name": "multi_dest_in_multi_app_with_result_handler_lambda__extension_group_2",
                 "addon": "multi_dest_in_multi_app_with_result_handler_lambda__extension_group_2",
                 "app": "msgpack://127.0.0.1:8002/"
               },{
                 "type": "extension_group",
                 "name": "multi_dest_in_multi_app_with_result_handler_lambda__extension_group_3",
                 "addon": "multi_dest_in_multi_app_with_result_handler_lambda__extension_group_3",
                 "app": "msgpack://127.0.0.1:8003/"
               },{
                 "type": "extension_group",
                 "name": "multi_dest_in_multi_app_with_result_handler_lambda__extension_group_4",
                 "addon": "multi_dest_in_multi_app_with_result_handler_lambda__extension_group_4",
                 "app": "msgpack://127.0.0.1:8004/"
               },{
                 "type": "extension_group",
                 "name": "multi_dest_in_multi_app_with_result_handler_lambda__extension_group_5",
                 "addon": "multi_dest_in_multi_app_with_result_handler_lambda__extension_group_5",
                 "app": "msgpack://127.0.0.1:8005/"
               }],
               "connections": [{
                 "app": "msgpack://127.0.0.1:8001/",
                 "extension_group": "multi_dest_in_multi_app_with_result_handler_lambda__extension_group_1",
                 "extension": "test extension 1",
                 "cmd": [{
                   "name": "hello_world",
                   "dest": [{
                      "app": "msgpack://127.0.0.1:8001/",
                      "extension_group": "multi_dest_in_multi_app_with_result_handler_lambda__extension_group_1",
                      "extension": "test extension 2"
                   },{
                      "app": "msgpack://127.0.0.1:8001/",
                      "extension_group": "multi_dest_in_multi_app_with_result_handler_lambda__extension_group_1",
                      "extension": "test extension 3"
                   },{
                      "app": "msgpack://127.0.0.1:8002/",
                      "extension_group": "multi_dest_in_multi_app_with_result_handler_lambda__extension_group_2",
                      "extension": "test extension 4"
                   },{
                      "app": "msgpack://127.0.0.1:8002/",
                      "extension_group": "multi_dest_in_multi_app_with_result_handler_lambda__extension_group_2",
                      "extension": "test extension 5"
                   },{
                      "app": "msgpack://127.0.0.1:8002/",
                      "extension_group": "multi_dest_in_multi_app_with_result_handler_lambda__extension_group_2",
                      "extension": "test extension 6"
                   },{
                      "app": "msgpack://127.0.0.1:8003/",
                      "extension_group": "multi_dest_in_multi_app_with_result_handler_lambda__extension_group_3",
                      "extension": "test extension 7"
                   },{
                      "app": "msgpack://127.0.0.1:8003/",
                      "extension_group": "multi_dest_in_multi_app_with_result_handler_lambda__extension_group_3",
                      "extension": "test extension 8"
                   },{
                      "app": "msgpack://127.0.0.1:8003/",
                      "extension_group": "multi_dest_in_multi_app_with_result_handler_lambda__extension_group_3",
                      "extension": "test extension 9"
                   },{
                      "app": "msgpack://127.0.0.1:8004/",
                      "extension_group": "multi_dest_in_multi_app_with_result_handler_lambda__extension_group_4",
                      "extension": "test extension 10"
                   },{
                      "app": "msgpack://127.0.0.1:8004/",
                      "extension_group": "multi_dest_in_multi_app_with_result_handler_lambda__extension_group_4",
                      "extension": "test extension 11"
                   },{
                      "app": "msgpack://127.0.0.1:8004/",
                      "extension_group": "multi_dest_in_multi_app_with_result_handler_lambda__extension_group_4",
                      "extension": "test extension 12"
                   },{
                      "app": "msgpack://127.0.0.1:8005/",
                      "extension_group": "multi_dest_in_multi_app_with_result_handler_lambda__extension_group_5",
                      "extension": "test extension 13"
                   },{
                      "app": "msgpack://127.0.0.1:8005/",
                      "extension_group": "multi_dest_in_multi_app_with_result_handler_lambda__extension_group_5",
                      "extension": "test extension 14"
                   },{
                      "app": "msgpack://127.0.0.1:8005/",
                      "extension_group": "multi_dest_in_multi_app_with_result_handler_lambda__extension_group_5",
                      "extension": "test extension 15"
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

  // Send a user-defined 'hello world' command to 'extension 1'.
  nlohmann::json resp = client->send_json_and_recv_resp_in_json(
      R"({
           "_ten": {
             "name": "hello_world",
             "seq_id": "137",
             "dest":[{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "multi_dest_in_multi_app_with_result_handler_lambda__extension_group_1",
               "extension": "test extension 1"
             }]
           }
         })"_json);
  ten_test::check_result_is(resp, "137", TEN_STATUS_CODE_OK,
                            "return from extension 1");

  delete client;

  ten::msgpack_tcp_client_t::close_app("msgpack://127.0.0.1:8001/");
  ten::msgpack_tcp_client_t::close_app("msgpack://127.0.0.1:8002/");
  ten::msgpack_tcp_client_t::close_app("msgpack://127.0.0.1:8003/");
  ten::msgpack_tcp_client_t::close_app("msgpack://127.0.0.1:8004/");
  ten::msgpack_tcp_client_t::close_app("msgpack://127.0.0.1:8005/");

  WAIT_APP_TO_STOP(1)
  WAIT_APP_TO_STOP(2)
  WAIT_APP_TO_STOP(3)
  WAIT_APP_TO_STOP(4)
  WAIT_APP_TO_STOP(5)
}