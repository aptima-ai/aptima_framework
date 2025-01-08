//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <string>

#include "gtest/gtest.h"
#include "include_internal/axis_runtime/binding/cpp/ten.h"
#include "axis_runtime/binding/cpp/detail/msg/cmd/start_graph.h"
#include "axis_runtime/msg/cmd/start_graph/cmd.h"
#include "axis_runtime/axis_env/internal/metadata.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/thread.h"
#include "tests/common/client/msgpack_tcp.h"

namespace {

class test_extension_1 : public ten::extension_t {
 public:
  explicit test_extension_1(const char *name) : extension_t(name) {}
};

axis_CPP_REGISTER_ADDON_AS_EXTENSION(error_client_send_json__extension_1,
                                    test_extension_1);

class test_extension_2 : public ten::extension_t {
 public:
  explicit test_extension_2(const char *name) : ten::extension_t(name) {}
};

axis_CPP_REGISTER_ADDON_AS_EXTENSION(error_client_send_json__extension_2,
                                    test_extension_2);

void test_app_on_configure(axis_UNUSED axis_app_t *self, axis_env_t *axis_env) {
  bool result = axis_env_init_property_from_json(axis_env,
                                                "{\
                          \"_ten\": {\
                          \"uri\": \"msgpack://127.0.0.1:8001/\",\
                          \"log_level\": 2\
                          }\
                         }",
                                                nullptr);
  ASSERT_EQ(result, true);

  axis_env_on_configure_done(axis_env, nullptr);
}

void *test_app_thread_main(axis_UNUSED void *args) {
  axis_app_t *app =
      axis_app_create(test_app_on_configure, nullptr, nullptr, nullptr);
  axis_app_run(app, false, nullptr);
  axis_app_wait(app, nullptr);
  axis_app_destroy(app);

  return nullptr;
}

}  // namespace

TEST(ExtensionTest, ErrorClientSendJson) {  // NOLINT
  auto *app_thread =
      axis_thread_create("test_app_thread_main", test_app_thread_main, nullptr);

  // Create a client and connect to the app.
  axis_test_msgpack_tcp_client_t *client =
      axis_test_msgpack_tcp_client_create("msgpack://127.0.0.1:8001/");

  axis_error_t *err = axis_error_create();
  auto *invalid_graph_cmd = axis_cmd_start_graph_create();
  bool success =
      axis_cmd_start_graph_set_graph_from_json_str(invalid_graph_cmd, R"({
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
  EXPECT_STREQ(axis_error_errmsg(err),
               "extension 'extension_1' is associated with different addon "
               "'error_client_send_json__extension_2', "
               "'error_client_send_json__extension_1'");

  axis_error_destroy(err);
  axis_shared_ptr_destroy(invalid_graph_cmd);

  // Strange connection would _not_ cause the TEN app to be closed, so we have
  // to close the TEN app explicitly.
  axis_test_msgpack_tcp_client_close_app(client);

  axis_test_msgpack_tcp_client_destroy(client);

  axis_thread_join(app_thread, -1);
}
