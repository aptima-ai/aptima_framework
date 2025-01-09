//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <string>

#include "gtest/gtest.h"
#include "include_internal/aptima_runtime/binding/cpp/ten.h"
#include "aptima_runtime/binding/cpp/detail/msg/cmd/start_graph.h"
#include "aptima_runtime/msg/cmd/start_graph/cmd.h"
#include "aptima_runtime/aptima_env/internal/metadata.h"
#include "aptima_utils/lib/error.h"
#include "aptima_utils/lib/smart_ptr.h"
#include "aptima_utils/lib/thread.h"
#include "tests/common/client/msgpack_tcp.h"

namespace {

class test_extension_1 : public ten::extension_t {
 public:
  explicit test_extension_1(const char *name) : extension_t(name) {}
};

aptima_CPP_REGISTER_ADDON_AS_EXTENSION(error_client_send_json__extension_1,
                                    test_extension_1);

class test_extension_2 : public ten::extension_t {
 public:
  explicit test_extension_2(const char *name) : ten::extension_t(name) {}
};

aptima_CPP_REGISTER_ADDON_AS_EXTENSION(error_client_send_json__extension_2,
                                    test_extension_2);

void test_app_on_configure(aptima_UNUSED aptima_app_t *self, aptima_env_t *aptima_env) {
  bool result = aptima_env_init_property_from_json(aptima_env,
                                                "{\
                          \"_ten\": {\
                          \"uri\": \"msgpack://127.0.0.1:8001/\",\
                          \"log_level\": 2\
                          }\
                         }",
                                                nullptr);
  ASSERT_EQ(result, true);

  aptima_env_on_configure_done(aptima_env, nullptr);
}

void *test_app_thread_main(aptima_UNUSED void *args) {
  aptima_app_t *app =
      aptima_app_create(test_app_on_configure, nullptr, nullptr, nullptr);
  aptima_app_run(app, false, nullptr);
  aptima_app_wait(app, nullptr);
  aptima_app_destroy(app);

  return nullptr;
}

}  // namespace

TEST(ExtensionTest, ErrorClientSendJson) {  // NOLINT
  auto *app_thread =
      aptima_thread_create("test_app_thread_main", test_app_thread_main, nullptr);

  // Create a client and connect to the app.
  aptima_test_msgpack_tcp_client_t *client =
      aptima_test_msgpack_tcp_client_create("msgpack://127.0.0.1:8001/");

  aptima_error_t *err = aptima_error_create();
  auto *invalid_graph_cmd = aptima_cmd_start_graph_create();
  bool success =
      aptima_cmd_start_graph_set_graph_from_json_str(invalid_graph_cmd, R"({
        "nodes":[
          {
            "type": "extension",
            "name": "extension_1",
            "addon": "error_client_send_json__extension_1",
            "app": "msgpack://127.0.0.1:8001/",
            "extension_group": "extension_group"
          },
          {
            "type": "extension",
            "name": "extension_1",
            "addon": "error_client_send_json__extension_2",
            "app": "msgpack://127.0.0.1:8001/",
            "extension_group": "extension_group"
          }
        ]
      })",
                                                  err);
  EXPECT_EQ(success, false);
  EXPECT_STREQ(aptima_error_errmsg(err),
               "extension 'extension_1' is associated with different addon "
               "'error_client_send_json__extension_2', "
               "'error_client_send_json__extension_1'");

  aptima_error_destroy(err);
  aptima_shared_ptr_destroy(invalid_graph_cmd);

  // Strange connection would _not_ cause the TEN app to be closed, so we have
  // to close the TEN app explicitly.
  aptima_test_msgpack_tcp_client_close_app(client);

  aptima_test_msgpack_tcp_client_destroy(client);

  aptima_thread_join(app_thread, -1);
}
