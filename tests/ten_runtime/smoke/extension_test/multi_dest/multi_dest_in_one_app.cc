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
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/ten_runtime/smoke/extension_test/util/binding/cpp/check.h"

#define DEST_EXTENSION_MIN_ID 2
#define DEST_EXTENSION_MAX_ID 35

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
  RESPONSE_16,
  RESPONSE_17,
  RESPONSE_18,
  RESPONSE_19,
  RESPONSE_20,
  RESPONSE_21,
  RESPONSE_22,
  RESPONSE_23,
  RESPONSE_24,
  RESPONSE_25,
  RESPONSE_26,
  RESPONSE_27,
  RESPONSE_28,
  RESPONSE_29,
  RESPONSE_30,
  RESPONSE_31,
  RESPONSE_32,
  RESPONSE_33,
  RESPONSE_34,
  RESPONSE_35,
} RESPONSE;

class test_extension_1 : public ten::extension_t {
 public:
  explicit test_extension_1(const std::string &name) : ten::extension_t(name) {}

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    nlohmann::json json = nlohmann::json::parse(cmd->to_json());
    if (json["_ten"]["name"] == "hello_world") {
      ten_env.send_cmd(
          std::move(cmd), [this](ten::ten_env_t &ten_env,
                                 std::unique_ptr<ten::cmd_result_t> status) {
            pending_resp_num--;
            if (pending_resp_num == 0) {
              status->set_property("detail", "return from extension 1");
              ten_env.return_result_directly(std::move(status));
            }
          });
      return;
    }
  }

 private:
  int pending_resp_num{1};
};

#define DEFINE_TEST_EXTENSION(N)                                              \
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

DEFINE_TEST_EXTENSION(2)
DEFINE_TEST_EXTENSION(3)
DEFINE_TEST_EXTENSION(4)
DEFINE_TEST_EXTENSION(5)
DEFINE_TEST_EXTENSION(6)
DEFINE_TEST_EXTENSION(7)
DEFINE_TEST_EXTENSION(8)
DEFINE_TEST_EXTENSION(9)
DEFINE_TEST_EXTENSION(10)
DEFINE_TEST_EXTENSION(11)
DEFINE_TEST_EXTENSION(12)
DEFINE_TEST_EXTENSION(13)
DEFINE_TEST_EXTENSION(14)
DEFINE_TEST_EXTENSION(15)
DEFINE_TEST_EXTENSION(16)
DEFINE_TEST_EXTENSION(17)
DEFINE_TEST_EXTENSION(18)
DEFINE_TEST_EXTENSION(19)
DEFINE_TEST_EXTENSION(20)
DEFINE_TEST_EXTENSION(21)
DEFINE_TEST_EXTENSION(22)
DEFINE_TEST_EXTENSION(23)
DEFINE_TEST_EXTENSION(24)
DEFINE_TEST_EXTENSION(25)
DEFINE_TEST_EXTENSION(26)
DEFINE_TEST_EXTENSION(27)
DEFINE_TEST_EXTENSION(28)
DEFINE_TEST_EXTENSION(29)
DEFINE_TEST_EXTENSION(30)
DEFINE_TEST_EXTENSION(31)
DEFINE_TEST_EXTENSION(32)
DEFINE_TEST_EXTENSION(33)
DEFINE_TEST_EXTENSION(34)
DEFINE_TEST_EXTENSION(35)

#define ADD_TEST_EXTENSION(N) \
  extensions.push_back(new test_extension_##N("test extension " #N));

class test_extension_group : public ten::extension_group_t {
 public:
  explicit test_extension_group(const std::string &name)
      : ten::extension_group_t(name) {}

  void on_create_extensions(ten::ten_env_t &ten_env) override {
    std::vector<ten::extension_t *> extensions;
    ADD_TEST_EXTENSION(1)
    ADD_TEST_EXTENSION(2)
    ADD_TEST_EXTENSION(3)
    ADD_TEST_EXTENSION(4)
    ADD_TEST_EXTENSION(5)
    ADD_TEST_EXTENSION(6)
    ADD_TEST_EXTENSION(7)
    ADD_TEST_EXTENSION(8)
    ADD_TEST_EXTENSION(9)
    ADD_TEST_EXTENSION(10)
    ADD_TEST_EXTENSION(11)
    ADD_TEST_EXTENSION(12)
    ADD_TEST_EXTENSION(13)
    ADD_TEST_EXTENSION(14)
    ADD_TEST_EXTENSION(15)
    ADD_TEST_EXTENSION(16)
    ADD_TEST_EXTENSION(17)
    ADD_TEST_EXTENSION(18)
    ADD_TEST_EXTENSION(19)
    ADD_TEST_EXTENSION(20)
    ADD_TEST_EXTENSION(21)
    ADD_TEST_EXTENSION(22)
    ADD_TEST_EXTENSION(23)
    ADD_TEST_EXTENSION(24)
    ADD_TEST_EXTENSION(25)
    ADD_TEST_EXTENSION(26)
    ADD_TEST_EXTENSION(27)
    ADD_TEST_EXTENSION(28)
    ADD_TEST_EXTENSION(29)
    ADD_TEST_EXTENSION(30)
    ADD_TEST_EXTENSION(31)
    ADD_TEST_EXTENSION(32)
    ADD_TEST_EXTENSION(33)
    ADD_TEST_EXTENSION(34)
    ADD_TEST_EXTENSION(35)
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

class test_app : public ten::app_t {
 public:
  void on_init(ten::ten_env_t &ten_env) override {
    bool rc = ten_env.init_property_from_json(
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

    ten_env.on_init_done();
  }
};

void *test_app_thread_main(TEN_UNUSED void *args) {
  auto *app = new test_app();
  app->run();
  delete app;

  return nullptr;
}

TEN_CPP_REGISTER_ADDON_AS_EXTENSION_GROUP(
    multi_dest_in_one_app__extension_group, test_extension_group);

}  // namespace

TEST(ExtensionTest, MultiDestInOneApp) {  // NOLINT
  // Start app.
  auto *app_thread =
      ten_thread_create("app thread", test_app_thread_main, nullptr);

  // Create a client and connect to the app.
  auto *client = new ten::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

  // Send graph.
  nlohmann::json resp = client->send_json_and_recv_resp_in_json(
      R"({
           "_ten": {
             "type": "start_graph",
             "seq_id": "55",
             "nodes": [{
               "type": "extension_group",
               "name": "multi_dest_in_one_app__extension_group",
               "addon": "multi_dest_in_one_app__extension_group",
               "app": "msgpack://127.0.0.1:8001/"
             }],
             "connections": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "multi_dest_in_one_app__extension_group",
               "extension": "test extension 1",
               "cmd": [{
                 "name": "hello_world",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "multi_dest_in_one_app__extension_group",
                   "extension": "test extension 2"
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "multi_dest_in_one_app__extension_group",
                   "extension": "test extension 3"
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "multi_dest_in_one_app__extension_group",
                   "extension": "test extension 4"
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "multi_dest_in_one_app__extension_group",
                   "extension": "test extension 5"
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "multi_dest_in_one_app__extension_group",
                   "extension": "test extension 6"
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "multi_dest_in_one_app__extension_group",
                   "extension": "test extension 7"
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "multi_dest_in_one_app__extension_group",
                   "extension": "test extension 8"
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "multi_dest_in_one_app__extension_group",
                   "extension": "test extension 9"
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "multi_dest_in_one_app__extension_group",
                   "extension": "test extension 10"
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "multi_dest_in_one_app__extension_group",
                   "extension": "test extension 11"
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "multi_dest_in_one_app__extension_group",
                   "extension": "test extension 12"
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "multi_dest_in_one_app__extension_group",
                   "extension": "test extension 13"
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "multi_dest_in_one_app__extension_group",
                   "extension": "test extension 14"
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "multi_dest_in_one_app__extension_group",
                   "extension": "test extension 15"
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "multi_dest_in_one_app__extension_group",
                   "extension": "test extension 16"
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "multi_dest_in_one_app__extension_group",
                   "extension": "test extension 17"
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "multi_dest_in_one_app__extension_group",
                   "extension": "test extension 18"
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "multi_dest_in_one_app__extension_group",
                   "extension": "test extension 19"
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "multi_dest_in_one_app__extension_group",
                   "extension": "test extension 20"
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "multi_dest_in_one_app__extension_group",
                   "extension": "test extension 21"
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "multi_dest_in_one_app__extension_group",
                   "extension": "test extension 22"
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "multi_dest_in_one_app__extension_group",
                   "extension": "test extension 23"
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "multi_dest_in_one_app__extension_group",
                   "extension": "test extension 24"
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "multi_dest_in_one_app__extension_group",
                   "extension": "test extension 25"
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "multi_dest_in_one_app__extension_group",
                   "extension": "test extension 26"
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "multi_dest_in_one_app__extension_group",
                   "extension": "test extension 27"
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "multi_dest_in_one_app__extension_group",
                   "extension": "test extension 28"
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "multi_dest_in_one_app__extension_group",
                   "extension": "test extension 29"
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "multi_dest_in_one_app__extension_group",
                   "extension": "test extension 30"
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "multi_dest_in_one_app__extension_group",
                   "extension": "test extension 31"
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "multi_dest_in_one_app__extension_group",
                   "extension": "test extension 32"
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "multi_dest_in_one_app__extension_group",
                   "extension": "test extension 33"
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "multi_dest_in_one_app__extension_group",
                   "extension": "test extension 34"
                 },{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "multi_dest_in_one_app__extension_group",
                   "extension": "test extension 35"
                 }]
               }]
             }]
           }
         })"_json);
  ten_test::check_status_code_is(resp, TEN_STATUS_CODE_OK);

  // Send a user-defined 'hello world' command to 'extension 1'.
  resp = client->send_json_and_recv_resp_in_json(
      R"({
           "_ten": {
             "name": "hello_world",
             "seq_id": "137",
             "dest":[{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "multi_dest_in_one_app__extension_group",
               "extension": "test extension 1"
             }]
           }
         })"_json);

  ten_test::check_result_is(resp, "137", TEN_STATUS_CODE_OK,
                            "return from extension 1");

  delete client;

  ten_thread_join(app_thread, -1);
}
