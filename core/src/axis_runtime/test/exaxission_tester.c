//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/test/extension_tester.h"

#include <inttypes.h>
#include <string.h>

#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/extension_group/builtin/builtin_extension_group.h"
#include "include_internal/axis_runtime/extension_group/extension_group.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "include_internal/axis_runtime/test/env_tester.h"
#include "include_internal/axis_runtime/test/test_app.h"
#include "include_internal/axis_runtime/test/test_extension.h"
#include "axis_runtime/app/app.h"
#include "axis_runtime/extension/extension.h"
#include "axis_runtime/msg/cmd/start_graph/cmd.h"
#include "axis_runtime/msg/msg.h"
#include "axis_runtime/axis_env/internal/metadata.h"
#include "axis_runtime/axis_env/internal/on_xxx_done.h"
#include "axis_runtime/axis_env_proxy/axis_env_proxy.h"
#include "axis_utils/container/list.h"
#include "axis_utils/container/list_str.h"
#include "axis_utils/io/runloop.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/event.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/lib/thread.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/memory.h"

bool axis_extension_tester_check_integrity(axis_extension_tester_t *self,
                                          bool check_thread) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_EXTENSION_TESTER_SIGNATURE) {
    axis_ASSERT(0,
               "Failed to pass extension_thread signature checking: %" PRId64,
               self->signature);
    return false;
  }

  if (check_thread &&
      !axis_sanitizer_thread_check_do_check(&self->thread_check)) {
    return false;
  }

  return true;
}

axis_extension_tester_t *axis_extension_tester_create(
    axis_extension_tester_on_start_func_t on_start,
    axis_extension_tester_on_cmd_func_t on_cmd,
    axis_extension_tester_on_data_func_t on_data,
    axis_extension_tester_on_audio_frame_func_t on_audio_frame,
    axis_extension_tester_on_video_frame_func_t on_video_frame) {
  axis_extension_tester_t *self = axis_MALLOC(sizeof(axis_extension_tester_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  self->binding_handle.me_in_target_lang = self;

  axis_signature_set(&self->signature, axis_EXTENSION_TESTER_SIGNATURE);
  axis_sanitizer_thread_check_init_with_current_thread(&self->thread_check);

  axis_list_init(&self->addon_base_dirs);

  self->on_start = on_start;
  self->on_cmd = on_cmd;
  self->on_data = on_data;
  self->on_audio_frame = on_audio_frame;
  self->on_video_frame = on_video_frame;

  self->axis_env_tester = axis_env_tester_create(self);
  self->tester_runloop = axis_runloop_create(NULL);

  self->test_extension_axis_env_proxy = NULL;
  self->test_extension_axis_env_proxy_create_completed = axis_event_create(0, 1);

  self->test_app_axis_env_proxy = NULL;
  self->test_app_axis_env_proxy_create_completed = axis_event_create(0, 1);
  axis_string_init(&self->test_app_property_json);

  self->test_app_thread = NULL;
  self->user_data = NULL;

  self->test_mode = axis_EXTENSION_TESTER_TEST_MODE_INVALID;

  return self;
}

void axis_extension_tester_set_test_mode_single(axis_extension_tester_t *self,
                                               const char *addon_name,
                                               const char *property_json_str) {
  axis_ASSERT(self && axis_extension_tester_check_integrity(self, true),
             "Invalid argument.");
  axis_ASSERT(addon_name, "Invalid argument.");

  self->test_mode = axis_EXTENSION_TESTER_TEST_MODE_SINGLE;
  axis_string_init_from_c_str(&self->test_target.addon.addon_name, addon_name,
                             strlen(addon_name));

  if (property_json_str && strlen(property_json_str) > 0) {
    axis_error_t err;
    axis_error_init(&err);

    axis_json_t *json = axis_json_from_string(property_json_str, &err);
    if (json) {
      axis_json_destroy(json);
    } else {
      axis_ASSERT(0, "Failed to parse property json: %s",
                 axis_error_errmsg(&err));
    }

    axis_error_deinit(&err);

    axis_string_init_from_c_str(&self->test_target.addon.property_json,
                               property_json_str, strlen(property_json_str));
  } else {
    const char *empty_json = "{}";
    axis_string_init_from_c_str(&self->test_target.addon.property_json,
                               empty_json, strlen(empty_json));
  }
}

void axis_extension_tester_set_test_mode_graph(axis_extension_tester_t *self,
                                              const char *graph_json) {
  axis_ASSERT(self && axis_extension_tester_check_integrity(self, true),
             "Invalid argument.");
  axis_ASSERT(graph_json, "Invalid argument.");

  self->test_mode = axis_EXTENSION_TESTER_TEST_MODE_GRAPH;
  axis_string_init_from_c_str(&self->test_target.graph.graph_json, graph_json,
                             strlen(graph_json));
}

void axis_extension_tester_init_test_app_property_from_json(
    axis_extension_tester_t *self, const char *property_json_str) {
  axis_ASSERT(self && axis_extension_tester_check_integrity(self, true),
             "Invalid argument.");
  axis_ASSERT(property_json_str, "Invalid argument.");

  axis_string_set_formatted(&self->test_app_property_json, "%s",
                           property_json_str);
}

void axis_extension_tester_add_addon_base_dir(axis_extension_tester_t *self,
                                             const char *addon_base_dir) {
  axis_ASSERT(self && axis_extension_tester_check_integrity(self, true),
             "Invalid argument.");
  axis_ASSERT(addon_base_dir, "Invalid argument.");

  axis_list_push_str_back(&self->addon_base_dirs, addon_base_dir);
}

void test_app_axis_env_send_cmd(axis_env_t *axis_env, void *user_data) {
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_shared_ptr_t *cmd = user_data;
  axis_ASSERT(cmd && axis_msg_check_integrity(cmd), "Should not happen.");

  bool rc = axis_env_send_cmd(axis_env, cmd, NULL, NULL, NULL);
  axis_ASSERT(rc, "Should not happen.");
}

static void axis_extension_tester_destroy_test_target(
    axis_extension_tester_t *self) {
  axis_ASSERT(self && axis_extension_tester_check_integrity(self, true),
             "Invalid argument.");

  if (self->test_mode == axis_EXTENSION_TESTER_TEST_MODE_SINGLE) {
    axis_string_deinit(&self->test_target.addon.addon_name);
    axis_string_deinit(&self->test_target.addon.property_json);
  } else if (self->test_mode == axis_EXTENSION_TESTER_TEST_MODE_GRAPH) {
    axis_string_deinit(&self->test_target.graph.graph_json);
  }
}

void axis_extension_tester_destroy(axis_extension_tester_t *self) {
  axis_ASSERT(self && axis_extension_tester_check_integrity(self, true),
             "Invalid argument.");

  axis_ASSERT(self->test_app_axis_env_proxy == NULL,
             "The `axis_env_proxy` of `test_app` should be released in the "
             "tester task triggered by the `deinit` of `test_app`.");
  if (self->test_app_axis_env_proxy_create_completed) {
    axis_event_destroy(self->test_app_axis_env_proxy_create_completed);
  }

  axis_ASSERT(
      self->test_extension_axis_env_proxy == NULL,
      "The `axis_env_proxy` of `test_extension` should be released in the "
      "tester task triggered by the `deinit` of `test_extension`.");
  if (self->test_extension_axis_env_proxy_create_completed) {
    axis_event_destroy(self->test_extension_axis_env_proxy_create_completed);
  }

  axis_thread_join(self->test_app_thread, -1);

  axis_extension_tester_destroy_test_target(self);
  axis_list_clear(&self->addon_base_dirs);

  axis_string_deinit(&self->test_app_property_json);

  axis_env_tester_destroy(self->axis_env_tester);
  axis_sanitizer_thread_check_deinit(&self->thread_check);

  axis_runloop_destroy(self->tester_runloop);
  self->tester_runloop = NULL;

  axis_FREE(self);
}

static void test_app_axis_env_send_start_graph_cmd(axis_env_t *axis_env,
                                                  void *user_data) {
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_app_t *app = axis_env->attached_target.app;
  axis_ASSERT(app && axis_app_check_integrity(app, true), "Should not happen.");

  axis_shared_ptr_t *cmd = user_data;
  axis_ASSERT(cmd && axis_msg_check_integrity(cmd), "Should not happen.");

  bool rc = axis_msg_clear_and_set_dest(cmd, axis_app_get_uri(app), NULL, NULL,
                                       NULL, NULL);
  axis_ASSERT(rc, "Should not happen.");

  rc = axis_env_send_cmd(axis_env, cmd, NULL, NULL, NULL);
  axis_ASSERT(rc, "Should not happen.");
}

static void axis_extension_tester_create_and_start_graph(
    axis_extension_tester_t *self) {
  axis_ASSERT(self && axis_extension_tester_check_integrity(self, true),
             "Invalid argument.");
  axis_ASSERT(self->test_mode != axis_EXTENSION_TESTER_TEST_MODE_INVALID,
             "Invalid test mode.");
  axis_ASSERT(self->test_app_axis_env_proxy, "Invalid test app axis_env_proxy.");

  axis_shared_ptr_t *start_graph_cmd = axis_cmd_start_graph_create();
  axis_ASSERT(start_graph_cmd, "Should not happen.");

  bool rc = false;

  if (self->test_mode == axis_EXTENSION_TESTER_TEST_MODE_SINGLE) {
    axis_ASSERT(axis_string_check_integrity(&self->test_target.addon.addon_name),
               "Invalid test target.");

    const char *addon_name =
        axis_string_get_raw_str(&self->test_target.addon.addon_name);

    const char *property_json_str =
        axis_string_get_raw_str(&self->test_target.addon.property_json);

    axis_string_t graph_json_str;
    axis_string_init_formatted(&graph_json_str,
                              "{\
           \"nodes\": [{\
              \"type\": \"extension\",\
              \"name\": \"aptima:test_extension\",\
              \"addon\": \"aptima:test_extension\",\
              \"extension_group\": \"test_extension_group_1\",\
              \"app\": \"localhost\"\
           },{\
              \"type\": \"extension\",\
              \"name\": \"%s\",\
              \"addon\": \"%s\",\
              \"extension_group\": \"test_extension_group_2\",\
              \"app\": \"localhost\",\
              \"property\": %s\
           }],\
           \"connections\": [{\
             \"app\": \"localhost\",\
             \"extension_group\": \"test_extension_group_1\",\
             \"extension\": \"aptima:test_extension\",\
             \"cmd\": [{\
               \"name\": \"*\",\
               \"dest\": [{\
                  \"app\": \"localhost\",\
                  \"extension_group\": \"test_extension_group_2\",\
                  \"extension\": \"%s\"\
               }]\
             }],\
             \"data\": [{\
               \"name\": \"*\",\
               \"dest\": [{\
                  \"app\": \"localhost\",\
                  \"extension_group\": \"test_extension_group_2\",\
                  \"extension\": \"%s\"\
               }]\
             }],\
             \"video_frame\": [{\
               \"name\": \"*\",\
               \"dest\": [{\
                  \"app\": \"localhost\",\
                  \"extension_group\": \"test_extension_group_2\",\
                  \"extension\": \"%s\"\
               }]\
             }],\
             \"audio_frame\": [{\
               \"name\": \"*\",\
               \"dest\": [{\
                  \"app\": \"localhost\",\
                  \"extension_group\": \"test_extension_group_2\",\
                  \"extension\": \"%s\"\
               }]\
             }]\
           },{\
             \"app\": \"localhost\",\
             \"extension_group\": \"test_extension_group_2\",\
             \"extension\": \"%s\",\
             \"cmd\": [{\
               \"name\": \"*\",\
               \"dest\": [{\
                  \"app\": \"localhost\",\
                  \"extension_group\": \"test_extension_group_1\",\
                  \"extension\": \"aptima:test_extension\"\
               }]\
             }],\
             \"data\": [{\
               \"name\": \"*\",\
               \"dest\": [{\
                  \"app\": \"localhost\",\
                  \"extension_group\": \"test_extension_group_1\",\
                  \"extension\": \"aptima:test_extension\"\
               }]\
             }],\
             \"video_frame\": [{\
               \"name\": \"*\",\
               \"dest\": [{\
                  \"app\": \"localhost\",\
                  \"extension_group\": \"test_extension_group_1\",\
                  \"extension\": \"aptima:test_extension\"\
               }]\
             }],\
             \"audio_frame\": [{\
               \"name\": \"*\",\
               \"dest\": [{\
                  \"app\": \"localhost\",\
                  \"extension_group\": \"test_extension_group_1\",\
                  \"extension\": \"aptima:test_extension\"\
               }]\
             }]\
           }]\
       }",
                              addon_name, addon_name, property_json_str,
                              addon_name, addon_name, addon_name, addon_name,
                              addon_name);
    rc = axis_cmd_start_graph_set_graph_from_json_str(
        start_graph_cmd, axis_string_get_raw_str(&graph_json_str), NULL);
    axis_ASSERT(rc, "Should not happen.");

    axis_string_deinit(&graph_json_str);
  } else if (self->test_mode == axis_EXTENSION_TESTER_TEST_MODE_GRAPH) {
    axis_ASSERT(axis_string_check_integrity(&self->test_target.graph.graph_json),
               "Invalid test target.");
    axis_ASSERT(&self->test_target.graph.graph_json, "Should not happen.");

    rc = axis_cmd_start_graph_set_graph_from_json_str(
        start_graph_cmd,
        axis_string_get_raw_str(&self->test_target.graph.graph_json), NULL);
    axis_ASSERT(rc, "Should not happen.");
  }

  rc = axis_env_proxy_notify(self->test_app_axis_env_proxy,
                            test_app_axis_env_send_start_graph_cmd,
                            start_graph_cmd, false, NULL);
  axis_ASSERT(rc, "Should not happen.");

  // Wait for the tester extension to create the `axis_env_proxy`.
  axis_event_wait(self->test_extension_axis_env_proxy_create_completed, -1);

  axis_event_destroy(self->test_extension_axis_env_proxy_create_completed);
  self->test_extension_axis_env_proxy_create_completed = NULL;
}

static void axis_extension_tester_create_and_run_app(
    axis_extension_tester_t *self) {
  axis_ASSERT(self && axis_extension_tester_check_integrity(self, true),
             "Invalid argument.");

  // Create the tester app.
  self->test_app_thread = axis_thread_create(
      "test app thread", axis_builtin_test_app_thread_main, self);

  // Wait until the tester app is started successfully.
  axis_event_wait(self->test_app_axis_env_proxy_create_completed, -1);

  axis_event_destroy(self->test_app_axis_env_proxy_create_completed);
  self->test_app_axis_env_proxy_create_completed = NULL;

  axis_ASSERT(self->test_app_axis_env_proxy,
             "test_app should have been created its axis_env_proxy.");
}

void axis_extension_tester_on_start_done(axis_extension_tester_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_tester_check_integrity(self, true),
             "Invalid use of extension_tester %p.", self);

  axis_LOGI("tester on_start() done.");

  bool rc = axis_env_proxy_notify(
      self->test_extension_axis_env_proxy,
      axis_builtin_test_extension_axis_env_notify_on_start_done, NULL, false,
      NULL);
  axis_ASSERT(rc, "Should not happen.");
}

void axis_extension_tester_on_test_extension_start(
    axis_extension_tester_t *self) {
  axis_ASSERT(self && axis_extension_tester_check_integrity(self, true),
             "Invalid argument.");

  if (self->on_start) {
    self->on_start(self, self->axis_env_tester);
  } else {
    axis_extension_tester_on_start_done(self);
  }
}

void axis_extension_tester_on_test_extension_deinit(
    axis_extension_tester_t *self) {
  axis_ASSERT(self && axis_extension_tester_check_integrity(self, true),
             "Invalid argument.");

  // Since the tester uses the extension's `axis_env_proxy` to interact with
  // `test_extension`, it is necessary to release the extension's
  // `axis_env_proxy` within the tester thread to ensure thread safety.
  //
  // Releasing the extension's `axis_env_proxy` within the tester thread also
  // guarantees that `test_extension` is still active at that time (As long as
  // the `axis_env_proxy` exists, the extension will not be destroyed.), ensuring
  // that all operations using the extension's `axis_env_proxy` before the
  // releasing of axis_env_proxy are valid.
  bool rc = axis_env_proxy_release(self->test_extension_axis_env_proxy, NULL);
  axis_ASSERT(rc, "Should not happen.");

  self->test_extension_axis_env_proxy = NULL;
}

static void axis_extension_tester_on_first_task(void *self_,
                                               axis_UNUSED void *arg) {
  axis_extension_tester_t *self = (axis_extension_tester_t *)self_;
  axis_ASSERT(self && axis_extension_tester_check_integrity(self, true),
             "Invalid argument.");

  axis_extension_tester_create_and_run_app(self);
  axis_extension_tester_create_and_start_graph(self);
}

static void axis_extension_tester_inherit_thread_ownership(
    axis_extension_tester_t *self) {
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: The correct threading ownership will be setup soon, so we can
  // _not_ check thread safety here.
  axis_ASSERT(self && axis_extension_tester_check_integrity(self, false),
             "Invalid argument.");

  axis_sanitizer_thread_check_set_belonging_thread_to_current_thread(
      &self->thread_check);
}

bool axis_extension_tester_run(axis_extension_tester_t *self) {
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: this function could be called in different threads other than
  // the creation thread.
  axis_ASSERT(self && axis_extension_tester_check_integrity(self, false),
             "Invalid argument.");
  axis_ASSERT(self->test_mode != axis_EXTENSION_TESTER_TEST_MODE_INVALID,
             "Invalid test mode.");

  axis_extension_tester_inherit_thread_ownership(self);

  // Inject the task that calls the first task into the runloop of
  // extension_tester, ensuring that the first task is called within the
  // extension_tester thread to guarantee thread safety.
  axis_runloop_post_task_tail(self->tester_runloop,
                             axis_extension_tester_on_first_task, self, NULL);

  // Start the runloop of tester.
  axis_runloop_run(self->tester_runloop);

  return true;
}

axis_env_tester_t *axis_extension_tester_get_axis_env_tester(
    axis_extension_tester_t *self) {
  axis_ASSERT(self && axis_extension_tester_check_integrity(self, true),
             "Invalid argument.");

  return self->axis_env_tester;
}
