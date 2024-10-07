//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
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
  void on_configure(ten::ten_env_t &ten_env) override {
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
                        "log_level": 2,
                        "predefined_graphs": [{
                          "name": "default",
                          "auto_start": true,
                          "nodes": [{
                            "type": "extension_group",
                            "name": "migration_group",
                            "addon": "batch_send_msgs_in_migration__migration_group"
                          }]
                        }]
                      }
                    })"
        // clang-format on
    );
    ASSERT_EQ(rc, true);

    ten_env.on_configure_done();
  }
};

void *app_thread_main(TEN_UNUSED void *args) {
  auto *app = new test_app();
  app->run();
  delete app;

  return nullptr;
}

TEN_CPP_REGISTER_ADDON_AS_EXTENSION_GROUP(
    batch_send_msgs_in_migration__migration_group, test_migration_group);

}  // namespace

TEST(ExtensionTest, BatchSendMsgsInMigration) {  // NOLINT
  auto *app_thread = ten_thread_create("app thread", app_thread_main, nullptr);

  // Create a client and connect to the app.
  auto *client = new ten::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

  const int size = 10;

  // Send more than one message asynchronously, the protocol should only
  // transfer one message to the runtime before the connection migration is
  // completed.
  for (int i = 0; i < size; i++) {
    client->send_json(
        R"({
             "_ten": {
               "name": "test",
               "seq_id": "123",
               "dest": [{
                 "app": "msgpack://127.0.0.1:8001/",
                 "graph": "default",
                 "extension_group": "migration_group",
                 "extension": "migration"
               }]
             }
           })"_json);
  }

  int count = 0;
  while (count < size) {
    auto results = client->batch_recv_resp_in_json();

    for (auto &resp : results) {
      ten_test::check_result_is(resp, "123", TEN_STATUS_CODE_OK,
                                R"({"id":1,"name":"a"})");
      count++;
    }
  }

  delete client;

  ten_thread_join(app_thread, -1);
}
