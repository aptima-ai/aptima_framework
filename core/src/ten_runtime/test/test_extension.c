//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <stdio.h>
#include <stdlib.h>

#include "include_internal/ten_runtime/addon/addon.h"
#include "include_internal/ten_runtime/addon/extension/extension.h"
#include "include_internal/ten_runtime/addon/extension_group/extension_group.h"
#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/extension/extension.h"
#include "include_internal/ten_runtime/extension_group/extension_group.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "include_internal/ten_runtime/test/env_tester.h"
#include "include_internal/ten_runtime/test/extension_tester.h"
#include "ten_runtime/addon/addon.h"
#include "ten_runtime/aptima.h"
#include "ten_runtime/ten_env/internal/log.h"
#include "ten_runtime/ten_env/internal/on_xxx_done.h"
#include "ten_runtime/ten_env/ten_env.h"
#include "ten_utils/lib/smart_ptr.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"

static ten_extension_tester_t *test_extension_get_extension_tester_ptr(
    ten_env_t *ten_env) {
  ten_value_t *test_info_ptr_value =
      ten_env_peek_property(ten_env, "app:tester_ptr", NULL);
  TEN_ASSERT(test_info_ptr_value, "Should not happen.");

  ten_extension_tester_t *tester = ten_value_get_ptr(test_info_ptr_value, NULL);
  TEN_ASSERT(tester && ten_extension_tester_check_integrity(tester, false),
             "Should not happen.");

  return tester;
}

static void test_extension_on_configure(ten_extension_t *self,
                                        ten_env_t *ten_env) {
  TEN_ASSERT(self && ten_env, "Invalid argument.");

  ten_extension_tester_t *tester =
      test_extension_get_extension_tester_ptr(ten_env);
  self->user_data = tester;

  // Create the ten_env_proxy, and notify the testing environment that the
  // ten_env_proxy is ready.
  tester->test_extension_ten_env_proxy = ten_env_proxy_create(ten_env, 1, NULL);

  ten_event_set(tester->test_extension_ten_env_proxy_create_completed);

  bool rc = ten_env_on_configure_done(ten_env, NULL);
  TEN_ASSERT(rc, "Should not happen.");
}

static void ten_extension_tester_on_test_extension_start_task(
    void *self_, TEN_UNUSED void *arg) {
  ten_extension_tester_t *tester = self_;
  TEN_ASSERT(tester && ten_extension_tester_check_integrity(tester, true),
             "Invalid argument.");

  ten_extension_tester_on_test_extension_start(tester);
}

static void test_extension_on_start(ten_extension_t *self, ten_env_t *ten_env) {
  TEN_ASSERT(self && ten_env, "Invalid argument.");

  // The tester framework needs to ensure that the tester's environment is
  // always destroyed later than the test_extension, so calling the tester
  // within the test_extension is always valid.
  ten_extension_tester_t *tester =
      test_extension_get_extension_tester_ptr(ten_env);
  self->user_data = tester;

  ten_runloop_post_task_tail(tester->tester_runloop,
                             ten_extension_tester_on_test_extension_start_task,
                             tester, NULL);
}

void ten_builtin_test_extension_ten_env_notify_on_start_done(
    ten_env_t *ten_env, TEN_UNUSED void *user_data) {
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  bool rc = ten_env_on_start_done(ten_env, NULL);
  TEN_ASSERT(rc, "Should not happen.");
}

static void ten_extension_tester_on_test_extension_cmd_task(void *self_,
                                                            void *arg) {
  ten_extension_tester_t *tester = self_;
  TEN_ASSERT(tester && ten_extension_tester_check_integrity(tester, true),
             "Invalid argument.");

  ten_shared_ptr_t *cmd = arg;
  TEN_ASSERT(cmd, "Invalid argument.");

  if (tester->on_cmd) {
    tester->on_cmd(tester, tester->ten_env_tester, cmd);
  }

  ten_shared_ptr_destroy(cmd);
}

static void test_extension_on_cmd(ten_extension_t *self, ten_env_t *ten_env,
                                  ten_shared_ptr_t *cmd) {
  TEN_ASSERT(self && ten_env, "Invalid argument.");

  ten_extension_tester_t *tester = self->user_data;
  TEN_ASSERT(tester && ten_extension_tester_check_integrity(tester, false),
             "Should not happen.");

  // Inject cmd into the extension_tester thread to ensure thread safety.
  ten_runloop_post_task_tail(tester->tester_runloop,
                             ten_extension_tester_on_test_extension_cmd_task,
                             tester, ten_shared_ptr_clone(cmd));
}

static void ten_extension_tester_on_test_extension_data_task(void *self_,
                                                             void *arg) {
  ten_extension_tester_t *tester = self_;
  TEN_ASSERT(tester && ten_extension_tester_check_integrity(tester, true),
             "Invalid argument.");

  ten_shared_ptr_t *data = arg;
  TEN_ASSERT(data, "Invalid argument.");

  if (tester->on_data) {
    tester->on_data(tester, tester->ten_env_tester, data);
  }

  ten_shared_ptr_destroy(data);
}

static void test_extension_on_data(ten_extension_t *self, ten_env_t *ten_env,
                                   ten_shared_ptr_t *data) {
  TEN_ASSERT(self && ten_env, "Invalid argument.");

  ten_extension_tester_t *tester = self->user_data;
  TEN_ASSERT(tester && ten_extension_tester_check_integrity(tester, false),
             "Should not happen.");

  // Inject data into the extension_tester thread to ensure thread safety.
  ten_runloop_post_task_tail(tester->tester_runloop,
                             ten_extension_tester_on_test_extension_data_task,
                             tester, ten_shared_ptr_clone(data));
}

static void ten_extension_tester_on_test_extension_audio_frame_task(void *self_,
                                                                    void *arg) {
  ten_extension_tester_t *tester = self_;
  TEN_ASSERT(tester && ten_extension_tester_check_integrity(tester, true),
             "Invalid argument.");

  ten_shared_ptr_t *audio_frame = arg;
  TEN_ASSERT(audio_frame, "Invalid argument.");

  if (tester->on_audio_frame) {
    tester->on_audio_frame(tester, tester->ten_env_tester, audio_frame);
  }

  ten_shared_ptr_destroy(audio_frame);
}

static void test_extension_on_audio_frame(ten_extension_t *self,
                                          ten_env_t *ten_env,
                                          ten_shared_ptr_t *audio_frame) {
  TEN_ASSERT(self && ten_env, "Invalid argument.");

  ten_extension_tester_t *tester = self->user_data;
  TEN_ASSERT(tester && ten_extension_tester_check_integrity(tester, false),
             "Should not happen.");

  // Inject audio_frame into the extension_tester thread to ensure thread
  // safety.
  ten_runloop_post_task_tail(
      tester->tester_runloop,
      ten_extension_tester_on_test_extension_audio_frame_task, tester,
      ten_shared_ptr_clone(audio_frame));
}

static void ten_extension_tester_on_test_extension_video_frame_task(void *self_,
                                                                    void *arg) {
  ten_extension_tester_t *tester = self_;
  TEN_ASSERT(tester && ten_extension_tester_check_integrity(tester, true),
             "Invalid argument.");

  ten_shared_ptr_t *video_frame = arg;
  TEN_ASSERT(video_frame, "Invalid argument.");

  if (tester->on_video_frame) {
    tester->on_video_frame(tester, tester->ten_env_tester, video_frame);
  }

  ten_shared_ptr_destroy(video_frame);
}

static void test_extension_on_video_frame(ten_extension_t *self,
                                          ten_env_t *ten_env,
                                          ten_shared_ptr_t *video_frame) {
  TEN_ASSERT(self && ten_env, "Invalid argument.");

  ten_extension_tester_t *tester = self->user_data;
  TEN_ASSERT(tester && ten_extension_tester_check_integrity(tester, false),
             "Should not happen.");

  // Inject video_frame into the extension_tester thread to ensure thread
  // safety.
  ten_runloop_post_task_tail(
      tester->tester_runloop,
      ten_extension_tester_on_test_extension_video_frame_task, tester,
      ten_shared_ptr_clone(video_frame));
}

static void ten_extension_tester_on_test_extension_deinit_task(
    void *self_, TEN_UNUSED void *arg) {
  ten_extension_tester_t *tester = self_;
  TEN_ASSERT(tester && ten_extension_tester_check_integrity(tester, true),
             "Invalid argument.");

  ten_extension_tester_on_test_extension_deinit(tester);
}

static void test_extension_on_deinit(ten_extension_t *self,
                                     ten_env_t *ten_env) {
  TEN_ASSERT(self && ten_env, "Invalid argument.");

  // The tester framework needs to ensure that the tester's environment is
  // always destroyed later than the test_extension, so calling the tester
  // within the test_extension is always valid.
  ten_extension_tester_t *tester = self->user_data;
  TEN_ASSERT(tester && ten_extension_tester_check_integrity(tester, false),
             "Should not happen.");

  ten_runloop_post_task_tail(tester->tester_runloop,
                             ten_extension_tester_on_test_extension_deinit_task,
                             tester, NULL);

  // It is safe to call on_deinit_done here, because as long as the
  // ten_env_proxy has not been destroyed, the test_extension will not be
  // destroyed either. Therefore, any task in the tester environment before the
  // actual destruction of ten_env_proxy can still use it to interact with the
  // test_extension as usual.
  bool rc = ten_env_on_deinit_done(ten_env, NULL);
  TEN_ASSERT(rc, "Should not happen.");
}

static void test_extension_addon_create_instance(ten_addon_t *addon,
                                                 ten_env_t *ten_env,
                                                 const char *name,
                                                 void *context) {
  TEN_ASSERT(addon && name, "Invalid argument.");

  ten_extension_t *extension = ten_extension_create(
      name, test_extension_on_configure, NULL, test_extension_on_start, NULL,
      test_extension_on_deinit, test_extension_on_cmd, test_extension_on_data,
      test_extension_on_audio_frame, test_extension_on_video_frame, NULL);

  ten_env_on_create_instance_done(ten_env, extension, context, NULL);
}

static void test_extension_addon_destroy_instance(TEN_UNUSED ten_addon_t *addon,
                                                  ten_env_t *ten_env,
                                                  void *_extension,
                                                  void *context) {
  ten_extension_t *extension = (ten_extension_t *)_extension;
  TEN_ASSERT(extension, "Invalid argument.");

  ten_extension_destroy(extension);

  ten_env_on_destroy_instance_done(ten_env, context, NULL);
}

static ten_addon_t ten_builtin_test_extension_addon = {
    NULL,
    TEN_ADDON_SIGNATURE,
    NULL,
    NULL,
    test_extension_addon_create_instance,
    test_extension_addon_destroy_instance,
    NULL,
    NULL,
};

void ten_builtin_test_extension_addon_register(void) {
  ten_addon_register_extension(TEN_STR_TEN_TEST_EXTENSION, NULL,
                               &ten_builtin_test_extension_addon, NULL);
}

void ten_builtin_test_extension_addon_unregister(void) {
  ten_addon_unregister_extension(TEN_STR_TEN_TEST_EXTENSION);
}
