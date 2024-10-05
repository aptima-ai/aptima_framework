//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <stdio.h>
#include <stdlib.h>

#include "include_internal/ten_runtime/addon/addon.h"
#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/extension/extension.h"
#include "include_internal/ten_runtime/extension_group/extension_group.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "include_internal/ten_runtime/test/env_tester.h"
#include "include_internal/ten_runtime/test/extension_tester.h"
#include "ten_runtime/addon/addon.h"
#include "ten_runtime/addon/extension_group/extension_group.h"
#include "ten_runtime/extension_group/extension_group.h"
#include "ten_runtime/ten.h"
#include "ten_runtime/ten_env/internal/log.h"
#include "ten_runtime/ten_env/internal/on_xxx_done.h"
#include "ten_runtime/ten_env/ten_env.h"
#include "ten_utils/lib/smart_ptr.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"

static ten_extension_tester_t *tester_extension_get_extension_tester_ptr(
    ten_env_t *ten_env) {
  ten_value_t *test_info_ptr_value =
      ten_env_peek_property(ten_env, "app:tester_ptr", NULL);
  TEN_ASSERT(test_info_ptr_value, "Should not happen.");

  ten_extension_tester_t *tester = ten_value_get_ptr(test_info_ptr_value, NULL);
  TEN_ASSERT(tester && ten_extension_tester_check_integrity(tester, false),
             "Should not happen.");

  return tester;
}

static void tester_extension_on_configure(ten_extension_t *self,
                                          ten_env_t *ten_env) {
  TEN_ASSERT(self && ten_env, "Invalid argument.");

  ten_extension_tester_t *tester =
      tester_extension_get_extension_tester_ptr(ten_env);
  self->user_data = tester;

  // Create the ten_env_proxy, and notify the testing environment that the
  // ten_env_proxy is ready.
  tester->tester_extension_ten_env_proxy =
      ten_env_proxy_create(ten_env, 1, NULL);

  ten_event_set(tester->tester_extension_ten_env_proxy_create_completed);

  bool rc = ten_env_on_configure_done(ten_env, NULL);
  TEN_ASSERT(rc, "Should not happen.");
}

static void ten_extension_tester_on_tester_extension_cmd_task(void *self_,
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

static void tester_extension_on_cmd(ten_extension_t *self, ten_env_t *ten_env,
                                    ten_shared_ptr_t *cmd) {
  TEN_ASSERT(self && ten_env, "Invalid argument.");

  ten_extension_tester_t *tester = self->user_data;
  TEN_ASSERT(tester && ten_extension_tester_check_integrity(tester, false),
             "Should not happen.");

  ten_runloop_post_task_tail(tester->tester_runloop,
                             ten_extension_tester_on_tester_extension_cmd_task,
                             tester, ten_shared_ptr_clone(cmd));
}

static void ten_extension_tester_on_tester_extension_deinit_task(
    void *self_, TEN_UNUSED void *arg) {
  ten_extension_tester_t *tester = self_;
  TEN_ASSERT(tester && ten_extension_tester_check_integrity(tester, true),
             "Invalid argument.");

  // Since the tester uses the extension's `ten_env_proxy` to interact with
  // `tester_extension`, it is necessary to release the extension's
  // `ten_env_proxy` within the tester thread to ensure thread safety.
  //
  // Releasing the extension's `ten_env_proxy` within the tester thread also
  // guarantees that `tester_extension` is still active at that time (As long as
  // the `ten_env_proxy` exists, the extension will not be destroyed.), ensuring
  // that all operations using the extension's `ten_env_proxy` before the
  // releasing of ten_env_proxy are valid.
  bool rc = ten_env_proxy_release(tester->tester_extension_ten_env_proxy, NULL);
  TEN_ASSERT(rc, "Should not happen.");

  tester->tester_extension_ten_env_proxy = NULL;
}

static void tester_extension_on_deinit(ten_extension_t *self,
                                       ten_env_t *ten_env) {
  TEN_ASSERT(self && ten_env, "Invalid argument.");

  ten_extension_tester_t *tester = self->user_data;
  TEN_ASSERT(tester && ten_extension_tester_check_integrity(tester, false),
             "Should not happen.");

  ten_runloop_post_task_tail(
      tester->tester_runloop,
      ten_extension_tester_on_tester_extension_deinit_task, tester, NULL);

  // =-=-= 这里会有 race condition???
  bool rc = ten_env_on_deinit_done(ten_env, NULL);
  TEN_ASSERT(rc, "Should not happen.");
}

static void tester_extension_addon_create_instance(ten_addon_t *addon,
                                                   ten_env_t *ten_env,
                                                   const char *name,
                                                   void *context) {
  TEN_ASSERT(addon && name, "Invalid argument.");

  ten_extension_t *extension =
      ten_extension_create(name, tester_extension_on_configure, NULL, NULL,
                           NULL, tester_extension_on_deinit,
                           tester_extension_on_cmd, NULL, NULL, NULL, NULL);

  ten_env_on_create_instance_done(ten_env, extension, context, NULL);
}

static void tester_extension_addon_destroy_instance(
    TEN_UNUSED ten_addon_t *addon, ten_env_t *ten_env, void *_extension,
    void *context) {
  ten_extension_t *extension = (ten_extension_t *)_extension;
  TEN_ASSERT(extension, "Invalid argument.");

  ten_extension_destroy(extension);

  ten_env_on_destroy_instance_done(ten_env, context, NULL);
}

static ten_addon_t ten_builtin_tester_extension_addon = {
    NULL,
    TEN_ADDON_SIGNATURE,
    NULL,
    NULL,
    NULL,
    NULL,
    tester_extension_addon_create_instance,
    tester_extension_addon_destroy_instance,
    NULL,
};

void ten_builtin_tester_extension_addon_register(void) {
  ten_addon_register_extension(TEN_STR_TEN_TEST_EXTENSION, NULL,
                               &ten_builtin_tester_extension_addon);
}

void ten_builtin_tester_extension_addon_unregister(void) {
  ten_addon_unregister_extension(TEN_STR_TEN_TEST_EXTENSION);
}
