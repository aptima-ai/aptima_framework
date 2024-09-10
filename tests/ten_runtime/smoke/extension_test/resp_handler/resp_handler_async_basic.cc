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
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/ten_runtime/smoke/extension_test/util/binding/cpp/check.h"

namespace {

class test_extension_1 : public ten::extension_t {
 public:
  explicit test_extension_1(const std::string &name) : ten::extension_t(name) {}

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    nlohmann::json json = nlohmann::json::parse(cmd->to_json());
    if (json["_ten"]["name"] == "hello_world_1") {
      auto test_string = std::make_shared<std::string>("test test test");

      ten_env.send_cmd(std::move(cmd),
                       [test_string](ten::ten_env_t &ten_env,
                                     std::unique_ptr<ten::cmd_result_t> cmd) {
                         nlohmann::json json =
                             nlohmann::json::parse(cmd->to_json());
                         if (json.value("detail", "") == "hello world, too" &&
                             *test_string == "test test test") {
                           cmd->set_property("detail", "hello world 1, too");
                           ten_env.return_result_directly(std::move(cmd));
                         }
                       });
    } else if (json["_ten"]["name"] == "hello_world_2") {
      ten_env.send_cmd(
          std::move(cmd), [](ten::ten_env_t &ten_env,
                             std::unique_ptr<ten::cmd_result_t> cmd_result) {
            nlohmann::json json = nlohmann::json::parse(cmd_result->to_json());
            if (json.value("detail", "") == "hello world, too") {
              cmd_result->set_property("detail", "hello world 2, too");
              ten_env.return_result_directly(std::move(cmd_result));
            }

            return true;
          });
    } else if (json["_ten"]["name"] == "hello_world_3") {
      ten_env.send_cmd(
          std::move(cmd), [](ten::ten_env_t &ten_env,
                             std::unique_ptr<ten::cmd_result_t> cmd_result) {
            nlohmann::json json = nlohmann::json::parse(cmd_result->to_json());
            if (json.value("detail", "") == "hello world, too") {
              cmd_result->set_property("detail", "hello world 3, too");
              ten_env.return_result_directly(std::move(cmd_result));
            }
          });
    } else if (json["_ten"]["name"] == "hello_world_4") {
      hello_world_4_cmd = std::move(cmd);
      ten_env.send_json(
          R"({"_ten": { "name": "hello_world_5" }})"_json.dump().c_str(),
          [&](ten::ten_env_t &ten_env, std::unique_ptr<ten::cmd_result_t> cmd) {
            nlohmann::json json = nlohmann::json::parse(cmd->to_json());
            if (json.value("detail", "") == "hello world, too") {
              auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
              cmd_result->set_property("detail", "hello world 4, too");
              ten_env.return_result(std::move(cmd_result),
                                    std::move(hello_world_4_cmd));
            }
          });
      return;
    } else if (json["_ten"]["name"] == "hello_world_5") {
      auto cmd_shared =
          std::make_shared<std::unique_ptr<ten::cmd_t>>(std::move(cmd));

      ten_env.send_json(
          R"({"_ten": { "name": "hello_world_6" }})"_json.dump().c_str(),
          [cmd_shared](ten::ten_env_t &ten_env,
                       std::unique_ptr<ten::cmd_result_t> cmd_result) {
            nlohmann::json json = nlohmann::json::parse(cmd_result->to_json());
            if (json.value("detail", "") == "hello world, too") {
              auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
              cmd_result->set_property("detail", "hello world 5, too");
              ten_env.return_result(std::move(cmd_result),
                                    std::move(*cmd_shared));
            }

            return true;
          });
      return;
    }
  }

 private:
  std::unique_ptr<ten::cmd_t> hello_world_4_cmd;
};

class test_extension_2 : public ten::extension_t {
 public:
  explicit test_extension_2(const std::string &name) : ten::extension_t(name) {}

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    nlohmann::json json = nlohmann::json::parse(cmd->to_json());
    if (json["_ten"]["name"] == "hello_world_1" ||
        json["_ten"]["name"] == "hello_world_2" ||
        json["_ten"]["name"] == "hello_world_3" ||
        json["_ten"]["name"] == "hello_world_5" ||
        json["_ten"]["name"] == "hello_world_6") {
      auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
      cmd_result->set_property("detail", "hello world, too");
      ten_env.return_result(std::move(cmd_result), std::move(cmd));
    }
  }
};

class test_extension_group : public ten::extension_group_t {
 public:
  explicit test_extension_group(const std::string &name)
      : ten::extension_group_t(name) {}

  void on_create_extensions(ten::ten_env_t &ten_env) override {
    std::vector<ten::extension_t *> extensions;
    extensions.push_back(new test_extension_1("test extension 1"));
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

class test_app : public ten::app_t {
 public:
  void on_init(ten::ten_env_t &ten_env) override {
    bool rc = ten_env.init_property_from_json(
        // clang-format off
                 R"({
                      "_ten": {
                        "uri": "msgpack://127.0.0.1:8001/",
                        "log_level": 1
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
    resp_handler_async_basic__extension_group, test_extension_group);

}  // namespace

TEST(ExtensionTest, RespHandlerAsyncBasic) {  // NOLINT
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
               "name": "resp_handler_async_basic__extension_group",
               "addon": "resp_handler_async_basic__extension_group",
               "app": "msgpack://127.0.0.1:8001/"
             }],
             "connections": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "resp_handler_async_basic__extension_group",
               "extension": "test extension 1",
               "cmd": [{
                 "name": "hello_world_1",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "resp_handler_async_basic__extension_group",
                   "extension": "test extension 2"
                 }]
               },{
                 "name": "hello_world_2",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "resp_handler_async_basic__extension_group",
                   "extension": "test extension 2"
                 }]
               },{
                 "name": "hello_world_3",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "resp_handler_async_basic__extension_group",
                   "extension": "test extension 2"
                 }]
               },{
                 "name": "hello_world_5",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "resp_handler_async_basic__extension_group",
                   "extension": "test extension 2"
                 }]
               },{
                 "name": "hello_world_6",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "resp_handler_async_basic__extension_group",
                   "extension": "test extension 2"
                 }]
               }]
             }]
           }
         })"_json);
  ten_test::check_status_code_is(resp, TEN_STATUS_CODE_OK);

  // Send a user-defined 'hello world' command.
  resp = client->send_json_and_recv_resp_in_json(
      R"({
           "_ten": {
             "name": "hello_world_1",
             "seq_id": "137",
             "dest": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "resp_handler_async_basic__extension_group",
               "extension": "test extension 1"
             }]
           }
         })"_json);
  ten_test::check_result_is(resp, "137", TEN_STATUS_CODE_OK,
                            "hello world 1, too");

  resp = client->send_json_and_recv_resp_in_json(
      R"({
           "_ten": {
             "name": "hello_world_2",
             "seq_id": "138",
             "dest": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "resp_handler_async_basic__extension_group",
               "extension": "test extension 1"
             }]
           }
         })"_json);
  ten_test::check_result_is(resp, "138", TEN_STATUS_CODE_OK,
                            "hello world 2, too");

  resp = client->send_json_and_recv_resp_in_json(
      R"({
           "_ten": {
             "name": "hello_world_3",
             "seq_id": "139",
             "dest": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "resp_handler_async_basic__extension_group",
               "extension": "test extension 1"
             }]
           }
         })"_json);
  ten_test::check_result_is(resp, "139", TEN_STATUS_CODE_OK,
                            "hello world 3, too");

  resp = client->send_json_and_recv_resp_in_json(
      R"({
           "_ten": {
             "name": "hello_world_4",
             "seq_id": "140",
             "dest": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "resp_handler_async_basic__extension_group",
               "extension": "test extension 1"
             }]
           }
         })"_json);
  ten_test::check_result_is(resp, "140", TEN_STATUS_CODE_OK,
                            "hello world 4, too");

  resp = client->send_json_and_recv_resp_in_json(
      R"({
           "_ten": {
             "name": "hello_world_5",
             "seq_id": "141",
             "dest": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "resp_handler_async_basic__extension_group",
               "extension": "test extension 1"
             }]
           }
         })"_json);
  ten_test::check_result_is(resp, "141", TEN_STATUS_CODE_OK,
                            "hello world 5, too");

  delete client;

  ten_thread_join(app_thread, -1);
}