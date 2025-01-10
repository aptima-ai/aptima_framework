//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <stdio.h>
#include <stdlib.h>

#include "include_internal/axis_runtime/addon/addon.h"
#include "include_internal/axis_runtime/addon/extension/extension.h"
#include "include_internal/axis_runtime/addon/extension_group/extension_group.h"
#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension_group/extension_group.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "include_internal/axis_runtime/test/env_tester.h"
#include "include_internal/axis_runtime/test/extension_tester.h"
#include "axis_runtime/addon/addon.h"
#include "axis_runtime/aptima.h"
#include "axis_runtime/axis_env/internal/log.h"
#include "axis_runtime/axis_env/internal/on_xxx_done.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"

static axis_extension_tester_t *test_extension_get_extension_tester_ptr(
    axis_env_t *axis_env) {
  axis_value_t *test_info_ptr_value =
      axis_env_peek_property(axis_env, "app:tester_ptr", NULL);
  axis_ASSERT(test_info_ptr_value, "Should not happen.");

  axis_extension_tester_t *tester = axis_value_get_ptr(test_info_ptr_value, NULL);
  axis_ASSERT(tester && axis_extension_tester_check_integrity(tester, false),
             "Should not happen.");

  return tester;
}

static void test_extension_on_configure(axis_extension_t *self,
                                        axis_env_t *axis_env) {
  axis_ASSERT(self && axis_env, "Invalid argument.");

  axis_extension_tester_t *tester =
      test_extension_get_extension_tester_ptr(axis_env);
  self->user_data = tester;

  // Create the axis_env_proxy, and notify the testing environment that the
  // axis_env_proxy is ready.
  tester->test_extension_axis_env_proxy = axis_env_proxy_create(axis_env, 1, NULL);

  axis_event_set(tester->test_extension_axis_env_proxy_create_completed);

  bool rc = axis_env_on_configure_done(axis_env, NULL);
  axis_ASSERT(rc, "Should not happen.");
}

static void axis_extension_tester_on_test_extension_start_task(
    void *self_, axis_UNUSED void *arg) {
  axis_extension_tester_t *tester = self_;
  axis_ASSERT(tester && axis_extension_tester_check_integrity(tester, true),
             "Invalid argument.");

  axis_extension_tester_on_test_extension_start(tester);
}

static void test_extension_on_start(axis_extension_t *self, axis_env_t *axis_env) {
  axis_ASSERT(self && axis_env, "Invalid argument.");

  // The tester framework needs to ensure that the tester's environment is
  // always destroyed later than the test_extension, so calling the tester
  // within the test_extension is always valid.
  axis_extension_tester_t *tester =
      test_extension_get_extension_tester_ptr(axis_env);
  self->user_data = tester;

  axis_runloop_post_task_tail(tester->tester_runloop,
                             axis_extension_tester_on_test_extension_start_task,
                             tester, NULL);
}

void axis_builtin_test_extension_axis_env_notify_on_start_done(
    axis_env_t *axis_env, axis_UNUSED void *user_data) {
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  bool rc = axis_env_on_start_done(axis_env, NULL);
  axis_ASSERT(rc, "Should not happen.");
}

static void axis_extension_tester_on_test_extension_cmd_task(void *self_,
                                                            void *arg) {
  axis_extension_tester_t *tester = self_;
  axis_ASSERT(tester && axis_extension_tester_check_integrity(tester, true),
             "Invalid argument.");

  axis_shared_ptr_t *cmd = arg;
  axis_ASSERT(cmd, "Invalid argument.");

  if (tester->on_cmd) {
    tester->on_cmd(tester, tester->axis_env_tester, cmd);
  }

  axis_shared_ptr_destroy(cmd);
}

static void test_extension_on_cmd(axis_extension_t *self, axis_env_t *axis_env,
                                  axis_shared_ptr_t *cmd) {
  axis_ASSERT(self && axis_env, "Invalid argument.");

  axis_extension_tester_t *tester = self->user_data;
  axis_ASSERT(tester && axis_extension_tester_check_integrity(tester, false),
             "Should not happen.");

  // Inject cmd into the extension_tester thread to ensure thread safety.
  axis_runloop_post_task_tail(tester->tester_runloop,
                             axis_extension_tester_on_test_extension_cmd_task,
                             tester, axis_shared_ptr_clone(cmd));
}

static void axis_extension_tester_on_test_extension_data_task(void *self_,
                                                             void *arg) {
  axis_extension_tester_t *tester = self_;
  axis_ASSERT(tester && axis_extension_tester_check_integrity(tester, true),
             "Invalid argument.");

  axis_shared_ptr_t *data = arg;
  axis_ASSERT(data, "Invalid argument.");

  if (tester->on_data) {
    tester->on_data(tester, tester->axis_env_tester, data);
  }

  axis_shared_ptr_destroy(data);
}

static void test_extension_on_data(axis_extension_t *self, axis_env_t *axis_env,
                                   axis_shared_ptr_t *data) {
  axis_ASSERT(self && axis_env, "Invalid argument.");

  axis_extension_tester_t *tester = self->user_data;
  axis_ASSERT(tester && axis_extension_tester_check_integrity(tester, false),
             "Should not happen.");

  // Inject data into the extension_tester thread to ensure thread safety.
  axis_runloop_post_task_tail(tester->tester_runloop,
                             axis_extension_tester_on_test_extension_data_task,
                             tester, axis_shared_ptr_clone(data));
}

static void axis_extension_tester_on_test_extension_audio_frame_task(void *self_,
                                                                    void *arg) {
  axis_extension_tester_t *tester = self_;
  axis_ASSERT(tester && axis_extension_tester_check_integrity(tester, true),
             "Invalid argument.");

  axis_shared_ptr_t *audio_frame = arg;
  axis_ASSERT(audio_frame, "Invalid argument.");

  if (tester->on_audio_frame) {
    tester->on_audio_frame(tester, tester->axis_env_tester, audio_frame);
  }

  axis_shared_ptr_destroy(audio_frame);
}

static void test_extension_on_audio_frame(axis_extension_t *self,
                                          axis_env_t *axis_env,
                                          axis_shared_ptr_t *audio_frame) {
  axis_ASSERT(self && axis_env, "Invalid argument.");

  axis_extension_tester_t *tester = self->user_data;
  axis_ASSERT(tester && axis_extension_tester_check_integrity(tester, false),
             "Should not happen.");

  // Inject audio_frame into the extension_tester thread to ensure thread
  // safety.
  axis_runloop_post_task_tail(
      tester->tester_runloop,
      axis_extension_tester_on_test_extension_audio_frame_task, tester,
      axis_shared_ptr_clone(audio_frame));
}

static void axis_extension_tester_on_test_extension_video_frame_task(void *self_,
                                                                    void *arg) {
  axis_extension_tester_t *tester = self_;
  axis_ASSERT(tester && axis_extension_tester_check_integrity(tester, true),
             "Invalid argument.");

  axis_shared_ptr_t *video_frame = arg;
  axis_ASSERT(video_frame, "Invalid argument.");

  if (tester->on_video_frame) {
    tester->on_video_frame(tester, tester->axis_env_tester, video_frame);
  }

  axis_shared_ptr_destroy(video_frame);
}

static void test_extension_on_video_frame(axis_extension_t *self,
                                          axis_env_t *axis_env,
                                          axis_shared_ptr_t *video_frame) {
  axis_ASSERT(self && axis_env, "Invalid argument.");

  axis_extension_tester_t *tester = self->user_data;
  axis_ASSERT(tester && axis_extension_tester_check_integrity(tester, false),
             "Should not happen.");

  // Inject video_frame into the extension_tester thread to ensure thread
  // safety.
  axis_runloop_post_task_tail(
      tester->tester_runloop,
      axis_extension_tester_on_test_extension_video_frame_task, tester,
      axis_shared_ptr_clone(video_frame));
}

static void axis_extension_tester_on_test_extension_deinit_task(
    void *self_, axis_UNUSED void *arg) {
  axis_extension_tester_t *tester = self_;
  axis_ASSERT(tester && axis_extension_tester_check_integrity(tester, true),
             "Invalid argument.");

  axis_extension_tester_on_test_extension_deinit(tester);
}

static void test_extension_on_deinit(axis_extension_t *self,
                                     axis_env_t *axis_env) {
  axis_ASSERT(self && axis_env, "Invalid argument.");

  // The tester framework needs to ensure that the tester's environment is
  // always destroyed later than the test_extension, so calling the tester
  // within the test_extension is always valid.
  axis_extension_tester_t *tester = self->user_data;
  axis_ASSERT(tester && axis_extension_tester_check_integrity(tester, false),
             "Should not happen.");

  axis_runloop_post_task_tail(tester->tester_runloop,
                             axis_extension_tester_on_test_extension_deinit_task,
                             tester, NULL);

  // It is safe to call on_deinit_done here, because as long as the
  // axis_env_proxy has not been destroyed, the test_extension will not be
  // destroyed either. Therefore, any task in the tester environment before the
  // actual destruction of axis_env_proxy can still use it to interact with the
  // test_extension as usual.
  bool rc = axis_env_on_deinit_done(axis_env, NULL);
  axis_ASSERT(rc, "Should not happen.");
}

static void test_extension_addon_create_instance(axis_addon_t *addon,
                                                 axis_env_t *axis_env,
                                                 const char *name,
                                                 void *context) {
  axis_ASSERT(addon && name, "Invalid argument.");

  axis_extension_t *extension = axis_extension_create(
      name, test_extension_on_configure, NULL, test_extension_on_start, NULL,
      test_extension_on_deinit, test_extension_on_cmd, test_extension_on_data,
      test_extension_on_audio_frame, test_extension_on_video_frame, NULL);

  axis_env_on_create_instance_done(axis_env, extension, context, NULL);
}

static void test_extension_addon_destroy_instance(axis_UNUSED axis_addon_t *addon,
                                                  axis_env_t *axis_env,
                                                  void *_extension,
                                                  void *context) {
  axis_extension_t *extension = (axis_extension_t *)_extension;
  axis_ASSERT(extension, "Invalid argument.");

  axis_extension_destroy(extension);

  axis_env_on_destroy_instance_done(axis_env, context, NULL);
}

static axis_addon_t axis_builtin_test_extension_addon = {
    NULL,
    axis_ADDON_SIGNATURE,
    NULL,
    NULL,
    test_extension_addon_create_instance,
    test_extension_addon_destroy_instance,
    NULL,
    NULL,
};

void axis_builtin_test_extension_addon_register(void) {
  axis_addon_register_extension(axis_STR_axis_TEST_EXTENSION, NULL,
                               &axis_builtin_test_extension_addon, NULL);
}

void axis_builtin_test_extension_addon_unregister(void) {
  axis_addon_unregister_extension(axis_STR_axis_TEST_EXTENSION);
}
