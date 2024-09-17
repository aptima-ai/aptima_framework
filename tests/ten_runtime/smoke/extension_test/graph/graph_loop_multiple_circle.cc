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

/**
 *
 * client --> A --> B --> C
 *                  ^     |
 *                  |     V
 *                  <---- D
 *
 */
class test_extension : public ten::extension_t {
 public:
  explicit test_extension(const std::string &name, int value)
      : ten::extension_t(name), name_(name), value_(value) {}

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    nlohmann::json json = nlohmann::json::parse(cmd->to_json());

    if (json["_ten"]["name"] == "sum") {
      if (counter_ == 10) {
        auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
        cmd_result->set_property_from_json("detail", json.dump().c_str());
        ten_env.return_result(std::move(cmd_result), std::move(cmd));
      } else {
        counter_++;

        if (!json.contains("total")) {
          json["total"] = "0";
        }
        int total = std::stoi(json.value("total", ""));
        total += value_;

        json["total"] = std::to_string(total);

        TEN_UNUSED bool const rc = cmd->from_json(json.dump().c_str());
        TEN_ASSERT(rc, "Should not happen.");

        ten_env.send_cmd(std::move(cmd));
        return;
      }
    }
  }

 private:
  const std::string name_;
  const int value_;
  int counter_ = 0;
};

class test_extension_group : public ten::extension_group_t {
 public:
  explicit test_extension_group(const std::string &name)
      : ten::extension_group_t(name) {}

  void on_create_extensions(ten::ten_env_t &ten_env) override {
    std::vector<ten::extension_t *> extensions;
    extensions.push_back(new test_extension("A", 0));
    extensions.push_back(new test_extension("B", 1));
    extensions.push_back(new test_extension("C", 2));
    extensions.push_back(new test_extension("D", 3));
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
    graph_loop_multiple_circle__extension_group, test_extension_group);

}  // namespace

TEST(ExtensionTest, GraphLoopMultipleCircle) {  // NOLINT
  // Start app.
  auto *app_thread =
      ten_thread_create("app thread", test_app_thread_main, nullptr);

  // Create a client and connect to the app.
  auto *client = new ten::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

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
               "name": "graph_loop_multiple_circle__extension_group",
               "addon": "graph_loop_multiple_circle__extension_group",
               "app": "msgpack://127.0.0.1:8001/"
             }],
             "connections": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "graph_loop_multiple_circle__extension_group",
               "extension": "A",
               "cmd": [{
                 "name": "sum",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "graph_loop_multiple_circle__extension_group",
                   "extension": "B"
                 }]
               }]
             },{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "graph_loop_multiple_circle__extension_group",
               "extension": "B",
               "cmd": [{
                 "name": "sum",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "graph_loop_multiple_circle__extension_group",
                   "extension": "C"
                 }]
               }]
             },{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "graph_loop_multiple_circle__extension_group",
               "extension": "C",
               "cmd": [{
                 "name": "sum",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "graph_loop_multiple_circle__extension_group",
                   "extension": "D"
                 }]
               }]
             },{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "graph_loop_multiple_circle__extension_group",
               "extension": "D",
               "cmd": [{
                 "name": "sum",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8001/",
                   "extension_group": "graph_loop_multiple_circle__extension_group",
                   "extension": "B"
                 }]
               }]
             }]
           }
         })"_json);
  ten_test::check_status_code_is(resp, TEN_STATUS_CODE_OK);

  resp = client->send_json_and_recv_resp_in_json(
      R"({
           "_ten": {
             "name": "sum",
             "seq_id": "137",
             "dest": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "graph_loop_multiple_circle__extension_group",
               "extension": "A"
             }]
           }
         })"_json);
  ten_test::check_status_code_is(resp, TEN_STATUS_CODE_OK);

  nlohmann::json detail = resp["detail"];
  EXPECT_EQ((1 + 2 + 3) * 10, std::stoi(detail["total"].get<std::string>()));

  delete client;

  ten_thread_join(app_thread, -1);
}
