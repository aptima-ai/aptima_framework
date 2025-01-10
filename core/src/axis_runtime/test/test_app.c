//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/test/extension_tester.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_utils/container/list.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/mark.h"

static void test_app_on_configure(axis_UNUSED axis_app_t *app,
                                  axis_env_t *axis_env) {
  // Since the tester will wait for the
  // `test_app_axis_env_proxy_create_completed` event after the app starts, it
  // means the tester is currently in a blocking state and not running, so
  // accessing the tester instance here is safe.
  axis_extension_tester_t *tester = app->user_data;
  axis_ASSERT(tester && axis_extension_tester_check_integrity(tester, false),
             "Should not happen.");

  bool rc = false;

  if (!axis_string_is_empty(&tester->test_app_property_json)) {
    rc = axis_env_init_property_from_json(
        axis_env, axis_string_get_raw_str(&tester->test_app_property_json), NULL);
    axis_ASSERT(rc, "Should not happen.");
  } else {
    // The default property.json content of the test app.
    rc = axis_env_init_property_from_json(axis_env,
                                         "{\
                                               \"_ten\": {\
                                                 \"log_level\": 2\
                                               }\
                                             }",
                                         NULL);
    axis_ASSERT(rc, "Should not happen.");
  }

  rc = axis_env_on_configure_done(axis_env, NULL);
  axis_ASSERT(rc, "Should not happen.");
}

// Store the tester as a property of the app so that the extensions within the
// app can access the tester's pointer through this app property.
static void store_tester_as_app_property(axis_extension_tester_t *tester,
                                         axis_env_t *axis_env) {
  axis_ASSERT(tester && axis_extension_tester_check_integrity(tester, false),
             "Invalid argument.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Invalid argument.");

  // Store the tester as a property of the app so that the extensions within the
  // app can access the tester's pointer through this app property.
  axis_value_t *test_info_ptr_value =
      axis_value_create_ptr(tester, NULL, NULL, NULL);

  bool rc =
      axis_env_set_property(axis_env, "tester_ptr", test_info_ptr_value, NULL);
  axis_ASSERT(rc, "Should not happen.");
}

static void create_axis_env_proxy_for_tester(axis_extension_tester_t *tester,
                                            axis_env_t *axis_env) {
  axis_ASSERT(tester && axis_extension_tester_check_integrity(tester, false),
             "Invalid argument.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Invalid argument.");

  tester->test_app_axis_env_proxy = axis_env_proxy_create(axis_env, 1, NULL);
  axis_ASSERT(tester->test_app_axis_env_proxy, "Should not happen.");

  axis_event_set(tester->test_app_axis_env_proxy_create_completed);
}

static void test_app_on_init(axis_app_t *app, axis_env_t *axis_env) {
  axis_extension_tester_t *tester = app->user_data;

  // Since the tester will wait for the
  // `test_app_axis_env_proxy_create_completed` event after the app starts,
  // using the tester here is thread-safe.
  axis_ASSERT(tester && axis_extension_tester_check_integrity(tester, false),
             "Should not happen.");

  store_tester_as_app_property(tester, axis_env);
  create_axis_env_proxy_for_tester(tester, axis_env);

  axis_env_on_init_done(axis_env, NULL);
}

static void axis_extension_tester_on_test_app_deinit_task(void *self_,
                                                         axis_UNUSED void *arg) {
  axis_extension_tester_t *tester = self_;
  axis_ASSERT(tester && axis_extension_tester_check_integrity(tester, true),
             "Invalid argument.");

  // Since the tester uses the app's `axis_env_proxy` to interact with
  // `test_app`, it is necessary to release the app's `axis_env_proxy` within
  // the tester thread to ensure thread safety.
  //
  // Releasing the app's `axis_env_proxy` within the tester thread also
  // guarantees that `test_app` is still active at that time (As long as the
  // `axis_env_proxy` exists, the app will not be destroyed.), ensuring that all
  // operations using the app's `axis_env_proxy` before the releasing of
  // axis_env_proxy are valid.
  bool rc = axis_env_proxy_release(tester->test_app_axis_env_proxy, NULL);
  axis_ASSERT(rc, "Should not happen.");

  tester->test_app_axis_env_proxy = NULL;

  axis_runloop_stop(tester->tester_runloop);
}

static void test_app_on_deinit(axis_app_t *app, axis_env_t *axis_env) {
  axis_extension_tester_t *tester = app->user_data;
  axis_ASSERT(tester && axis_extension_tester_check_integrity(tester, false),
             "Should not happen.");

  axis_runloop_post_task_tail(tester->tester_runloop,
                             axis_extension_tester_on_test_app_deinit_task,
                             tester, NULL);

  axis_env_on_deinit_done(axis_env, NULL);
}

void *axis_builtin_test_app_thread_main(void *args) {
  axis_error_t err;
  axis_error_init(&err);

  axis_app_t *test_app = axis_app_create(test_app_on_configure, test_app_on_init,
                                       test_app_on_deinit, &err);
  axis_ASSERT(test_app, "Failed to create app.");

  axis_extension_tester_t *tester = args;
  axis_ASSERT(tester && axis_extension_tester_check_integrity(tester, false),
             "Invalid argument.");

  test_app->user_data = tester;

  axis_list_foreach (&tester->addon_base_dirs, iter) {
    axis_string_t *addon_base_dir = axis_str_listnode_get(iter.node);
    axis_ASSERT(addon_base_dir, "Should not happen.");

    axis_app_add_axis_package_base_dir(test_app,
                                     axis_string_get_raw_str(addon_base_dir));
  }

  bool rc = axis_app_run(test_app, false, &err);
  axis_ASSERT(rc, "Should not happen.");

  axis_app_destroy(test_app);

  return NULL;
}
