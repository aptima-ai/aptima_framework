//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "gtest/gtest.h"
#include "include_internal/ten_runtime/binding/cpp/ten.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/ten_runtime/smoke/extension_test/util/binding/cpp/check.h"

namespace {

class test_migration : public ten::extension_t {
 public:
  explicit test_migration(const std::string &name) : ten::extension_t(name) {}

  void on_cmd(ten::ten_env_t &ten_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    nlohmann::json const detail = {{"id", 1}, {"name", "a"}};

    auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
    cmd_result->set_property_from_json("detail", detail.dump().c_str());
    ten_env.return_result(std::move(cmd_result), std::move(cmd));
  }
};

class test_migration_group : public ten::extension_group_t {
 public:
  explicit test_migration_group(const std::string &name)
      : ten::extension_group_t(name) {}

  void on_create_extensions(ten::ten_env_t &ten_env) override {
    std::vector<ten::extension_t *> extensions;
    extensions.push_back(new test_migration("migration"));
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
    ten::ten_env_internal_accessor_t ten_env_internal_accessor(&ten_env);
    bool rc = ten_env_internal_accessor.init_manifest_from_json(
        // clang-format off
                 R"({
                      "type": "app",
                      "name": "test_app",
                      "version": "0.1.0"
                    })"
        // clang-format on
    );
    ASSERT_EQ(rc, true);

    rc = ten_env.init_property_from_json(
        // clang-format off
                 R"({
                      "_ten": {
                        "uri": "msgpack://127.0.0.1:8001/",
                        "one_event_loop_per_engine": true,
                        "log_level": 1,
                        "predefined_graphs": [{
                          "name": "0",
                          "auto_start": true,
                          "nodes": [{
                            "type": "extension_group",
                            "name": "migration_group",
                            "addon": "wrong_engine_then_correct_in_migration__migration_group"
                          }]
                        }]
                      }
                    })"
        // clang-format on
    );
    ASSERT_EQ(rc, true);

    ten_env.on_init_done();
  }
};

void *app_thread_main(TEN_UNUSED void *args) {
  auto *app = new test_app();
  app->run();
  delete app;

  return nullptr;
}

TEN_CPP_REGISTER_ADDON_AS_EXTENSION_GROUP(
    wrong_engine_then_correct_in_migration__migration_group,
    test_migration_group);

}  // namespace

TEST(ExtensionTest, WrongEngineThenCorrectInMigration) {  // NOLINT
  auto *app_thread = ten_thread_create("app thread", app_thread_main, nullptr);

  // Create a client and connect to the app.
  auto *client = new ten::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

  // Send a message to the wrong engine, the connection won't be migrated as the
  // engine is not found.
  auto resp = client->send_json_and_recv_resp_in_json(
      R"({
           "_ten": {
             "name": "test",
             "seq_id": "1",
             "dest": [{
               "app": "msgpack://127.0.0.1:8001/",
               "graph": "incorrect_graph_name",
               "extension_group": "migration_group",
               "extension": "migration"
             }]
           }
         })"_json);
  ten_test::check_result_is(resp, "1", TEN_STATUS_CODE_ERROR,
                            "Graph not found.");

  // Send a message to the correct engine, the connection will be migrated, and
  // the belonging thread of the connection should be correct.
  resp = client->send_json_and_recv_resp_in_json(
      R"({
           "_ten": {
             "name": "test",
             "seq_id": "2",
             "dest": [{
               "app": "msgpack://127.0.0.1:8001/",
               "graph": "0",
               "extension_group": "migration_group",
               "extension": "migration"
             }]
           }
         })"_json);
  ten_test::check_result_is(resp, "2", TEN_STATUS_CODE_OK,
                            R"({"id":1,"name":"a"})");

  // The connection attaches to the remote now as it is migrated. Then send a
  // message to the wrong engine again, the message should be forwarded to the
  // app.
  resp = client->send_json_and_recv_resp_in_json(
      R"({
           "_ten": {
             "name": "test",
             "seq_id": "3",
             "dest": [{
               "app": "msgpack://127.0.0.1:8001/",
               "graph": "incorrect_graph_name",
               "extension_group": "migration_group",
               "extension": "migration"
             }]
           }
         })"_json);
  ten_test::check_result_is(resp, "3", TEN_STATUS_CODE_ERROR,
                            "Graph not found.");

  delete client;

  ten_thread_join(app_thread, -1);
}
