//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/test/extension_test.h"

#include <string.h>

#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/extension_group/builtin/builtin_extension_group.h"
#include "include_internal/ten_runtime/extension_group/extension_group.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd_base.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "include_internal/ten_runtime/test/test_app.h"
#include "ten_runtime/app/app.h"
#include "ten_runtime/extension/extension.h"
#include "ten_runtime/msg/cmd/close_app/cmd.h"
#include "ten_runtime/msg/cmd/start_graph/cmd.h"
#include "ten_runtime/msg/cmd_result/cmd_result.h"
#include "ten_runtime/msg/msg.h"
#include "ten_runtime/ten_env/internal/metadata.h"
#include "ten_runtime/ten_env/internal/on_xxx_done.h"
#include "ten_runtime/ten_env_proxy/ten_env_proxy.h"
#include "ten_utils/lib/event.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/lib/smart_ptr.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/lib/thread.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/memory.h"

ten_extension_test_t *ten_extension_test_create(void) {
  ten_extension_test_t *self = TEN_MALLOC(sizeof(ten_extension_test_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  ten_string_init(&self->target_extension_addon_name);

  self->test_app_ten_env_proxy = NULL;
  self->test_app_ten_env_proxy_create_completed = ten_event_create(0, 1);
  self->test_app_thread = ten_thread_create(
      "test app thread", ten_extension_test_app_thread_main, self);

  self->test_extension_ten_env_proxy = NULL;
  self->test_extension_ten_env_proxy_create_completed = ten_event_create(0, 1);

  ten_event_wait(self->test_app_ten_env_proxy_create_completed, -1);

  return self;
}

void ten_extension_test_add_addon(ten_extension_test_t *self,
                                  const char *addon_name) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(addon_name, "Invalid argument.");

  ten_string_set_formatted(&self->target_extension_addon_name, "%s",
                           addon_name);
}

static void send_cmd_to_app_callback(ten_extension_t *extension,
                                     ten_env_t *ten_env,
                                     ten_shared_ptr_t *cmd_result,
                                     TEN_UNUSED void *callback_info) {
  TEN_ASSERT(extension && ten_extension_check_integrity(extension, true),
             "Should not happen.");
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");
  TEN_ASSERT(cmd_result && ten_cmd_base_check_integrity(cmd_result),
             "Should not happen.");

  TEN_STATUS_CODE status_code = ten_cmd_result_get_status_code(cmd_result);
  TEN_ASSERT(status_code == TEN_STATUS_CODE_OK, "Should not happen.");
}

static void test_app_ten_env_send_cmd(ten_env_t *ten_env, void *user_data) {
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_shared_ptr_t *cmd = user_data;
  TEN_ASSERT(cmd && ten_msg_check_integrity(cmd), "Should not happen.");

  bool rc =
      ten_env_send_cmd(ten_env, cmd, send_cmd_to_app_callback, NULL, NULL);
  TEN_ASSERT(rc, "Should not happen.");
}

void ten_extension_test_destroy(ten_extension_test_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(self->test_app_ten_env_proxy, "Invalid argument.");

  ten_shared_ptr_t *close_app_cmd = ten_cmd_close_app_create();
  TEN_ASSERT(close_app_cmd, "Should not happen.");

  // Set the destination so that the recipient is the app itself.
  bool rc = ten_msg_clear_and_set_dest(close_app_cmd, TEN_STR_LOCALHOST, NULL,
                                       NULL, NULL, NULL);
  TEN_ASSERT(rc, "Should not happen.");

  ten_env_proxy_notify(self->test_app_ten_env_proxy, test_app_ten_env_send_cmd,
                       close_app_cmd, false, NULL);

  ten_thread_join(self->test_app_thread, -1);

  TEN_ASSERT(self->test_app_ten_env_proxy == NULL, "Should not happen.");
  ten_event_destroy(self->test_app_ten_env_proxy_create_completed);

  TEN_ASSERT(self->test_extension_ten_env_proxy == NULL, "Should not happen.");
  ten_event_destroy(self->test_extension_ten_env_proxy_create_completed);

  ten_string_deinit(&self->target_extension_addon_name);

  TEN_FREE(self);
}

void ten_extension_test_start(ten_extension_test_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(self->test_app_ten_env_proxy, "Invalid argument.");

  ten_shared_ptr_t *start_graph_cmd = ten_cmd_start_graph_create();
  TEN_ASSERT(start_graph_cmd, "Should not happen.");

  // Set the destination so that the recipient is the app itself.
  bool rc = ten_msg_clear_and_set_dest(start_graph_cmd, TEN_STR_LOCALHOST, NULL,
                                       NULL, NULL, NULL);
  TEN_ASSERT(rc, "Should not happen.");

  ten_string_t start_graph_cmd_json_str;
  ten_string_init_formatted(
      &start_graph_cmd_json_str,
      "{\
         \"_ten\": {\
           \"type\": \"start_graph\",\
           \"nodes\": [{\
              \"type\": \"extension\",\
              \"name\": \"test_extension\",\
              \"addon\": \"ten:test_extension\",\
              \"extension_group\": \"test_extension_group_1\",\
              \"app\": \"localhost\"\
           },{\
              \"type\": \"extension\",\
              \"name\": \"%s\",\
              \"addon\": \"%s\",\
              \"extension_group\": \"test_extension_group_2\",\
              \"app\": \"localhost\"\
           }],\
           \"connections\": [{\
             \"app\": \"localhost\",\
             \"extension_group\": \"test_extension_group_1\",\
             \"extension\": \"test_extension\",\
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
                  \"extension\": \"test_extension\"\
               }]\
             }],\
             \"data\": [{\
               \"name\": \"*\",\
               \"dest\": [{\
                  \"app\": \"localhost\",\
                  \"extension_group\": \"test_extension_group_1\",\
                  \"extension\": \"test_extension\"\
               }]\
             }],\
             \"video_frame\": [{\
               \"name\": \"*\",\
               \"dest\": [{\
                  \"app\": \"localhost\",\
                  \"extension_group\": \"test_extension_group_1\",\
                  \"extension\": \"test_extension\"\
               }]\
             }],\
             \"audio_frame\": [{\
               \"name\": \"*\",\
               \"dest\": [{\
                  \"app\": \"localhost\",\
                  \"extension_group\": \"test_extension_group_1\",\
                  \"extension\": \"test_extension\"\
               }]\
             }]\
           }]\
         }\
       }",
      ten_string_get_raw_str(&self->target_extension_addon_name),
      ten_string_get_raw_str(&self->target_extension_addon_name),
      ten_string_get_raw_str(&self->target_extension_addon_name),
      ten_string_get_raw_str(&self->target_extension_addon_name),
      ten_string_get_raw_str(&self->target_extension_addon_name),
      ten_string_get_raw_str(&self->target_extension_addon_name),
      ten_string_get_raw_str(&self->target_extension_addon_name));

  ten_json_t *start_graph_cmd_json = ten_json_from_string(
      ten_string_get_raw_str(&start_graph_cmd_json_str), NULL);

  ten_string_deinit(&start_graph_cmd_json_str);

  rc = ten_msg_from_json(start_graph_cmd, start_graph_cmd_json, NULL);
  TEN_ASSERT(rc, "Should not happen.");

  ten_json_destroy(start_graph_cmd_json);

  rc = ten_env_proxy_notify(self->test_app_ten_env_proxy,
                            test_app_ten_env_send_cmd, start_graph_cmd, false,
                            NULL);
  TEN_ASSERT(rc, "Should not happen.");

  ten_event_wait(self->test_extension_ten_env_proxy_create_completed, -1);
}

typedef struct ten_extension_test_send_cmd_info_t {
  ten_shared_ptr_t *cmd;
  ten_extension_test_cmd_result_handler_func_t handler;
  void *handler_user_data;
} ten_extension_test_send_cmd_info_t;

static ten_extension_test_send_cmd_info_t *
ten_extension_test_send_cmd_info_create(
    ten_shared_ptr_t *cmd, ten_extension_test_cmd_result_handler_func_t handler,
    void *handler_user_data) {
  ten_extension_test_send_cmd_info_t *self =
      TEN_MALLOC(sizeof(ten_extension_test_send_cmd_info_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  self->cmd = cmd;
  self->handler = handler;
  self->handler_user_data = handler_user_data;

  return self;
}

static void ten_extension_test_send_cmd_info_destroy(
    ten_extension_test_send_cmd_info_t *self) {
  TEN_ASSERT(self, "Invalid argument.");

  TEN_FREE(self);
}

static void send_cmd_to_extension_callback(ten_extension_t *extension,
                                           ten_env_t *ten_env,
                                           ten_shared_ptr_t *cmd_result,
                                           void *callback_user_data) {
  TEN_ASSERT(extension && ten_extension_check_integrity(extension, true),
             "Should not happen.");
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");
  TEN_ASSERT(cmd_result && ten_cmd_base_check_integrity(cmd_result),
             "Should not happen.");

  ten_extension_test_send_cmd_info_t *send_cmd_info = callback_user_data;
  TEN_ASSERT(send_cmd_info, "Invalid argument.");

  if (send_cmd_info->handler) {
    send_cmd_info->handler(cmd_result, send_cmd_info->handler_user_data);
  }

  ten_extension_test_send_cmd_info_destroy(send_cmd_info);
}

static void test_extension_ten_env_send_cmd(ten_env_t *ten_env,
                                            void *user_data) {
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_extension_test_send_cmd_info_t *send_cmd_info = user_data;
  TEN_ASSERT(send_cmd_info, "Invalid argument.");
  TEN_ASSERT(send_cmd_info->cmd && ten_msg_check_integrity(send_cmd_info->cmd),
             "Should not happen.");

  bool rc =
      ten_env_send_cmd(ten_env, send_cmd_info->cmd,
                       send_cmd_to_extension_callback, send_cmd_info, NULL);
  TEN_ASSERT(rc, "Should not happen.");

  ten_shared_ptr_destroy(send_cmd_info->cmd);
}

void ten_extension_test_send_cmd(
    ten_extension_test_t *self, ten_shared_ptr_t *cmd,
    ten_extension_test_cmd_result_handler_func_t handler, void *user_data) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(self->test_extension_ten_env_proxy, "Invalid argument.");

  ten_extension_test_send_cmd_info_t *send_cmd_info =
      ten_extension_test_send_cmd_info_create(cmd, handler, user_data);

  ten_env_proxy_notify(self->test_extension_ten_env_proxy,
                       test_extension_ten_env_send_cmd, send_cmd_info, false,
                       NULL);
}
