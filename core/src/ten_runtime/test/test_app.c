//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/app/app.h"
#include "include_internal/ten_runtime/test/extension_test.h"
#include "ten_utils/macro/mark.h"

static void test_ten_app_on_configure(TEN_UNUSED ten_app_t *app,
                                      ten_env_t *ten_env) {
  bool rc = ten_env_init_property_from_json(ten_env,
                                            "{\
                                               \"_ten\": {\
                                                 \"log_level\": 2\
                                               }\
                                             }",
                                            NULL);
  TEN_ASSERT(rc, "Should not happen.");

  rc = ten_env_on_configure_done(ten_env, NULL);
  TEN_ASSERT(rc, "Should not happen.");
}

static void test_ten_app_on_init(ten_app_t *app, ten_env_t *ten_env) {
  ten_extension_test_t *test_info = app->user_data;
  TEN_ASSERT(test_info, "Should not happen.");

  ten_value_t *test_info_ptr_value =
      ten_value_create_ptr(test_info, NULL, NULL, NULL);

  bool rc = ten_env_set_property(ten_env, "test_extension_test_info_ptr",
                                 test_info_ptr_value, NULL);
  TEN_ASSERT(rc, "Should not happen.");

  test_info->test_app_ten_env_proxy = ten_env_proxy_create(ten_env, 1, NULL);
  TEN_ASSERT(test_info->test_app_ten_env_proxy, "Should not happen.");

  ten_event_set(test_info->test_app_ten_env_proxy_create_completed);

  ten_env_on_init_done(ten_env, NULL);
}

static void test_ten_app_on_deinit(ten_app_t *app, ten_env_t *ten_env) {
  ten_extension_test_t *test_info = app->user_data;
  TEN_ASSERT(test_info, "Should not happen.");

  bool rc = ten_env_proxy_release(test_info->test_app_ten_env_proxy, NULL);
  TEN_ASSERT(rc, "Should not happen.");

  test_info->test_app_ten_env_proxy = NULL;

  ten_env_on_deinit_done(ten_env, NULL);
}

void *ten_extension_test_app_thread_main(void *args) {
  ten_error_t err;
  ten_error_init(&err);

  ten_extension_test_t *test_info = args;

  ten_app_t *test_app =
      ten_app_create(test_ten_app_on_configure, test_ten_app_on_init,
                     test_ten_app_on_deinit, &err);
  TEN_ASSERT(test_app, "Failed to create app.");

  test_app->user_data = test_info;

  bool rc = ten_app_run(test_app, false, &err);
  TEN_ASSERT(rc, "Should not happen.");

  ten_app_destroy(test_app);

  return NULL;
}
