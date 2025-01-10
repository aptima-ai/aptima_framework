//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "gtest/gtest.h"
#include "include_internal/aptima_runtime/binding/cpp/aptima.h"
#include "tests/common/client/cpp/msgpack_tcp.h"
#include "tests/aptima_runtime/smoke/util/binding/cpp/check.h"

namespace {

class test_migration : public aptima::extension_t {
 public:
  explicit test_migration(const char *name) : aptima::extension_t(name) {}

  void on_cmd(aptima::aptima_env_t &aptima_env,
              std::unique_ptr<aptima::cmd_t> cmd) override {
    nlohmann::json const detail = {{"id", 1}, {"name", "a"}};

    auto cmd_result = aptima::cmd_result_t::create(aptima_STATUS_CODE_OK);
    cmd_result->set_property_from_json("detail", detail.dump().c_str());
    aptima_env.return_result(std::move(cmd_result), std::move(cmd));
  }
};

class test_app : public aptima::app_t {
 public:
  void on_configure(aptima::aptima_env_t &aptima_env) override {
    bool rc = aptima::aptima_env_internal_accessor_t::init_manifest_from_json(
        aptima_env,
        // clang-format off
                 R"({
                      "type": "app",
                      "name": "test_app",
                      "version": "0.1.0"
                    })"
        // clang-format on
    );
    ASSERT_EQ(rc, true);

    rc = aptima_env.init_property_from_json(
        // clang-format off
                 R"({
                      "_ten": {
                        "uri": "msgpack://127.0.0.1:8001/",
                        "one_event_loop_per_engine": true,
                        "log_level": 2,
                        "predefined_graphs": [{
                          "name": "default",
                          "auto_start": true,
                          "singleton": true,
                          "nodes": [{
                            "type": "extension",
                            "name": "migration",
                            "addon": "wrong_engine_then_correct_in_migration__extension",
                            "extension_group": "migration_group"
                          }]
                        }]
                      }
                    })"
        // clang-format on
    );
    ASSERT_EQ(rc, true);

    aptima_env.on_configure_done();
  }
};

void *app_thread_main(aptima_UNUSED void *args) {
  auto *app = new test_app();
  app->run();
  delete app;

  return nullptr;
}

aptima_CPP_REGISTER_ADDON_AS_EXTENSION(
    wrong_engine_then_correct_in_migration__extension, test_migration);

}  // namespace

TEST(ExtensionTest, WrongEngineThenCorrectInMigration) {  // NOLINT
  auto *app_thread = aptima_thread_create("app thread", app_thread_main, nullptr);

  // Create a client and connect to the app.
  auto *client = new aptima::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

  // Send a message to the wrong engine, the connection won't be migrated as the
  // engine is not found.
  auto test_cmd = aptima::cmd_t::create("test");
  test_cmd->set_dest("msgpack://127.0.0.1:8001/", "incorrect_graph_id",
                     "migration_group", "migration");

  auto cmd_result = client->send_cmd_and_recv_result(std::move(test_cmd));

  aptima_test::check_status_code(cmd_result, aptima_STATUS_CODE_ERROR);
  aptima_test::check_detail_with_string(cmd_result, "Graph not found.");

  // Send a message to the correct engine, the connection will be migrated, and
  // the belonging thread of the connection should be correct.
  test_cmd = aptima::cmd_t::create("test");
  test_cmd->set_dest("msgpack://127.0.0.1:8001/", "default", "migration_group",
                     "migration");

  cmd_result = client->send_cmd_and_recv_result(std::move(test_cmd));

  aptima_test::check_status_code(cmd_result, aptima_STATUS_CODE_OK);
  aptima_test::check_detail_with_json(cmd_result, R"({"id":1,"name":"a"})");

  // The connection attaches to the remote now as it is migrated. Then send a
  // message to the wrong engine again, the message should be forwarded to the
  // app.
  test_cmd = aptima::cmd_t::create("test");
  test_cmd->set_dest("msgpack://127.0.0.1:8001/", "incorrect_graph_id",
                     "migration_group", "migration");

  cmd_result = client->send_cmd_and_recv_result(std::move(test_cmd));

  aptima_test::check_status_code(cmd_result, aptima_STATUS_CODE_ERROR);
  aptima_test::check_detail_with_string(cmd_result, "Graph not found.");

  delete client;

  aptima_thread_join(app_thread, -1);
}
