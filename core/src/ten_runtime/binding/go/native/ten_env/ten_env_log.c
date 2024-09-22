//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "include_internal/ten_runtime/binding/go/ten_env/ten_env.h"
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env_internal.h"
#include "include_internal/ten_runtime/ten_env/log.h"
#include "ten_runtime/binding/go/interface/ten/common.h"
#include "ten_runtime/binding/go/interface/ten/ten_env.h"
#include "ten_runtime/ten_env/internal/log.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/memory.h"

typedef struct ten_env_notify_log_info_t {
  int32_t level;
  const char *func_name;
  size_t func_name_len;
  const char *file_name;
  size_t file_name_len;
  size_t line_no;
  const char *msg;
  size_t msg_len;
  ten_event_t *completed;
} ten_env_notify_log_info_t;

static ten_env_notify_log_info_t *ten_env_notify_log_info_create(
    int32_t level, const char *func_name, size_t func_name_len,
    const char *file_name, size_t file_name_len, size_t line_no,
    const char *msg, size_t msg_len) {
  ten_env_notify_log_info_t *info =
      TEN_MALLOC(sizeof(ten_env_notify_log_info_t));
  TEN_ASSERT(info, "Failed to allocate memory.");

  info->level = level;
  info->func_name = func_name;
  info->func_name_len = func_name_len;
  info->file_name = file_name;
  info->file_name_len = file_name_len;
  info->line_no = line_no;
  info->msg = msg;
  info->msg_len = msg_len;
  info->completed = ten_event_create(0, 1);

  return info;
}

static void ten_env_notify_log_info_destroy(ten_env_notify_log_info_t *info) {
  TEN_ASSERT(info, "Invalid argument.");

  ten_event_destroy(info->completed);

  TEN_FREE(info);
}

static void ten_env_proxy_notify_log(ten_env_t *ten_env, void *user_data) {
  TEN_ASSERT(user_data, "Invalid argument.");
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_env_notify_log_info_t *info = user_data;
  TEN_ASSERT(info, "Should not happen.");

  ten_env_log_with_size_formatted(ten_env, info->level, info->func_name,
                                  info->func_name_len, info->file_name,
                                  info->file_name_len, info->line_no, "%.*s",
                                  info->msg_len, info->msg);

  ten_event_set(info->completed);
}

void ten_go_ten_env_log(uintptr_t bridge_addr, int level, const void *func_name,
                        int func_name_len, const void *file_name,
                        int file_name_len, int line_no, const void *msg,
                        int msg_len) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");

  // According to the document of `unsafe.StringData()`, the underlying data
  // (i.e., value here) of an empty GO string is unspecified. So it's unsafe to
  // access. We should handle this case explicitly.
  const char *func_name_value = NULL;
  if (func_name_len > 0) {
    func_name_value = (const char *)func_name;
  }

  const char *file_name_value = NULL;
  if (file_name_len > 0) {
    file_name_value = (const char *)file_name;
  }

  const char *msg_value = NULL;
  if (msg_len > 0) {
    msg_value = (const char *)msg;
  }

  ten_env_notify_log_info_t *info = ten_env_notify_log_info_create(
      level, func_name_value, func_name_len, file_name_value, file_name_len,
      line_no, msg_value, msg_len);

  ten_error_t err;
  ten_error_init(&err);

  if (self->c_ten_env->attach_to == TEN_ENV_ATTACH_TO_ADDON) {
    // TODO(Wei): This function is currently specifically designed for the addon
    // because the addon currently does not have a main thread, so it's unable
    // to check thread safety. Once the main thread for the addon is determined
    // in the future, these hacks made specifically for the addon can be
    // completely removed, and comprehensive thread safety checking can be
    // implemented.
    ten_env_log_with_size_formatted_without_check_thread(
        self->c_ten_env, info->level, info->func_name, info->func_name_len,
        info->file_name, info->file_name_len, info->line_no, "%.*s",
        info->msg_len, info->msg);
  } else {
    if (!ten_env_proxy_notify(self->c_ten_env_proxy, ten_env_proxy_notify_log,
                              info, false, &err)) {
      goto done;
    }
    ten_event_wait(info->completed, -1);
  }

done:
  ten_error_deinit(&err);
  ten_env_notify_log_info_destroy(info);
}
