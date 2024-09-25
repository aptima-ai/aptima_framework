//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/test/extension_test.h"

#include <string.h>

#include "include_internal/ten_runtime/app/app.h"
#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/extension_group/builtin/builtin_extension_group.h"
#include "include_internal/ten_runtime/extension_group/extension_group.h"
#include "include_internal/ten_runtime/extension_thread/extension_thread.h"
#include "include_internal/ten_runtime/extension_thread/on_xxx.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "ten_runtime/app/app.h"
#include "ten_runtime/extension/extension.h"
#include "ten_runtime/msg/cmd/close_app/cmd.h"
#include "ten_runtime/msg/msg.h"
#include "ten_runtime/ten_env/internal/metadata.h"
#include "ten_runtime/ten_env/internal/on_xxx_done.h"
#include "ten_runtime/ten_env_proxy/ten_env_proxy.h"
#include "ten_utils/container/list.h"
#include "ten_utils/container/list_ptr.h"
#include "ten_utils/lib/event.h"
#include "ten_utils/lib/smart_ptr.h"
#include "ten_utils/lib/thread.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"
#include "ten_utils/macro/memory.h"

static void *ten_extension_thread_main(void *self_) {
  ten_extension_test_t *self = (ten_extension_test_t *)self_;
  TEN_ASSERT(self, "Should not happen.");

  ten_list_foreach (&self->pre_created_extensions, iter) {
    ten_extension_t *pre_created_extension = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(pre_created_extension, "Should not happen.");

    ten_extension_inherit_thread_ownership(pre_created_extension,
                                           self->test_extension_thread);

    pre_created_extension->extension_thread = self->test_extension_thread;
    ten_extension_set_unique_name_in_graph(pre_created_extension);
  }

  return ten_extension_thread_main_actual(self->test_extension_thread);
}

static void test_ten_app_on_configure(ten_app_t *app, ten_env_t *ten_env) {
#if 0
  const char *property_json =
      "{\
         \"_ten\": {\
           \"predefined_graphs\": [{\
              \"name\": \"0\",\
              \"auto_start\": false,\
              \"nodes\": [{\
                \"type\": \"extension_group\",\
                \"name\": \"test_extension_group\",\
                \"addon\": \"test_extension_group\"\
              },{\
                \"type\": \"extension\",\
                \"name\": \"...\",\
                \"addon\": \"...\",\
                \"extension_group\": \"test_extension_group\"\
              }]\
           }]\
         }\
       }";
  bool rc = ten_env_init_property_from_json(ten_env, property_json, NULL);
  TEN_ASSERT(rc, "Should not happen.");
#endif

  bool rc = ten_env_on_configure_done(ten_env, NULL);
  TEN_ASSERT(rc, "Should not happen.");
}

static void test_ten_app_on_init(ten_app_t *app, ten_env_t *ten_env) {
  ten_extension_test_new_t *test_info = app->user_data;
  TEN_ASSERT(test_info, "Should not happen.");

  test_info->test_app_ten_env_proxy = ten_env_proxy_create(ten_env, 1, NULL);
  TEN_ASSERT(test_info->test_app_ten_env_proxy, "Should not happen.");

  ten_event_set(test_info->test_app_ten_env_proxy_create_completed);

  ten_env_on_init_done(ten_env, NULL);
}

static void test_ten_app_on_deinit(ten_app_t *app, ten_env_t *ten_env) {
  ten_extension_test_new_t *test_info = app->user_data;
  TEN_ASSERT(test_info, "Should not happen.");

  bool rc = ten_env_proxy_release(test_info->test_app_ten_env_proxy, NULL);
  TEN_ASSERT(rc, "Should not happen.");

  test_info->test_app_ten_env_proxy = NULL;

  ten_env_on_deinit_done(ten_env, NULL);
}

void *test_app_thread_main(void *args) {
  ten_error_t err;
  ten_error_init(&err);

  ten_extension_test_new_t *test_info = args;

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

ten_extension_test_new_t *ten_extension_test_create_new(void) {
  ten_extension_test_new_t *self = TEN_MALLOC(sizeof(ten_extension_test_new_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  self->test_app_ten_env_proxy = NULL;
  self->test_app_ten_env_proxy_create_completed = ten_event_create(0, 1);

  self->test_app_thread =
      ten_thread_create("test app thread", test_app_thread_main, self);

  ten_event_wait(self->test_app_ten_env_proxy_create_completed, -1);

  return self;
}

static void ten_env_proxy_notify_close_app(ten_env_t *ten_env,
                                           TEN_UNUSED void *user_data) {
  TEN_ASSERT(
      ten_env &&
          ten_env_check_integrity(
              ten_env,
              ten_env->attach_to != TEN_ENV_ATTACH_TO_ADDON ? true : false),
      "Should not happen.");

  ten_shared_ptr_t *close_app_cmd = ten_cmd_close_app_create();
  TEN_ASSERT(close_app_cmd, "Should not happen.");

  // Set the destination so that the recipient is the app itself.
  bool rc = ten_msg_clear_and_set_dest(close_app_cmd, TEN_STR_LOCALHOST, NULL,
                                       NULL, NULL, NULL, NULL);
  TEN_ASSERT(rc, "Should not happen.");

  rc = ten_env_send_cmd(ten_env, close_app_cmd, NULL, NULL, NULL);
  TEN_ASSERT(rc, "Should not happen.");
}

void ten_extension_test_destroy_new(ten_extension_test_new_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(self->test_app_ten_env_proxy, "Invalid argument.");

  ten_env_proxy_notify(self->test_app_ten_env_proxy,
                       ten_env_proxy_notify_close_app, NULL, false, NULL);

  ten_thread_join(self->test_app_thread, -1);

  TEN_ASSERT(self->test_app_ten_env_proxy == NULL, "Should not happen.");
  ten_event_destroy(self->test_app_ten_env_proxy_create_completed);

  TEN_FREE(self);
}

ten_extension_test_t *ten_extension_test_create(
    ten_extension_t *test_extension, ten_extension_t *target_extension) {
  ten_extension_test_t *self = TEN_MALLOC(sizeof(ten_extension_test_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  self->test_extension_thread = ten_extension_thread_create();
  self->test_extension_group = ten_extension_group_create_internal(
      "test_extension_group", NULL, NULL, NULL, NULL, NULL);

  ten_list_init(&self->pre_created_extensions);
  ten_list_push_ptr_back(&self->pre_created_extensions, test_extension, NULL);
  ten_list_push_ptr_back(&self->pre_created_extensions, target_extension, NULL);

  ten_extension_direct_all_msg_to_another_extension(test_extension,
                                                    target_extension);
  ten_extension_direct_all_msg_to_another_extension(test_extension,
                                                    target_extension);

  self->test_thread =
      ten_thread_create("extension thread", ten_extension_thread_main, self);

  return self;
}

void ten_extension_test_wait(ten_extension_test_t *self) {
  TEN_ASSERT(self, "Invalid argument.");

  int rc = ten_thread_join(self->test_thread, -1);
  TEN_ASSERT(!rc, "Should not happen.");
}

void ten_extension_test_destroy(ten_extension_test_t *self) {
  TEN_ASSERT(self, "Invalid argument.");

  ten_extension_thread_destroy(self->test_extension_thread);

  TEN_FREE(self);
}
