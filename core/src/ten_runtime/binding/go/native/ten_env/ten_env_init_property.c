//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/binding/go/internal/common.h"
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env.h"
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env_internal.h"
#include "ten_runtime/binding/go/interface/ten/common.h"
#include "ten_runtime/binding/go/interface/ten/ten_env.h"
#include "ten_runtime/common/errno.h"
#include "ten_runtime/ten_env/internal/metadata.h"
#include "ten_runtime/ten_env_proxy/ten_env_proxy.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/lib/event.h"
#include "ten_utils/lib/string.h"

typedef struct ten_notify_set_init_property_info_t {
  ten_string_t value;
  ten_error_t err;
  ten_event_t *completed;
} ten_env_notify_init_property_info_t;

static ten_env_notify_init_property_info_t *
ten_env_notify_init_property_info_create(const void *value, int value_len) {
  ten_env_notify_init_property_info_t *info =
      TEN_MALLOC(sizeof(ten_env_notify_init_property_info_t));
  TEN_ASSERT(info, "Failed to allocate memory.");

  ten_string_init_formatted(&info->value, "%.*s", value_len, value);
  ten_error_init(&info->err);
  info->completed = ten_event_create(0, 1);

  return info;
}

static void ten_env_notify_init_property_info_destroy(
    ten_env_notify_init_property_info_t *self) {
  TEN_ASSERT(self, "Invalid argument.");

  ten_string_deinit(&self->value);
  ten_error_deinit(&self->err);
  ten_event_destroy(self->completed);

  TEN_FREE(self);
}

static void ten_env_proxy_notify_init_property_from_json(ten_env_t *ten_env,
                                                         void *user_data) {
  TEN_ASSERT(user_data, "Invalid argument.");
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_env_notify_init_property_info_t *info = user_data;
  TEN_ASSERT(info, "Should not happen.");

  ten_error_t err;
  ten_error_init(&err);

  ten_env_init_property_from_json(ten_env, ten_string_get_raw_str(&info->value),
                                  &err);

  ten_event_set(info->completed);

  ten_error_deinit(&err);
}

ten_go_error_t ten_go_ten_env_init_property_from_json_bytes(
    uintptr_t bridge_addr, const void *json_str, int json_str_len) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");

  ten_go_error_t cgo_error;
  ten_go_error_init_with_errno(&cgo_error, TEN_ERRNO_OK);

  TEN_GO_TEN_ENV_IS_ALIVE_REGION_BEGIN(self, {
    ten_go_error_init_with_errno(&cgo_error, TEN_ERRNO_TEN_IS_CLOSED);
    return cgo_error;
  });

  ten_env_notify_init_property_info_t *info =
      ten_env_notify_init_property_info_create(json_str, json_str_len);
  TEN_ASSERT(info, "Should not happen.");

  if (!ten_env_proxy_notify(self->c_ten_env_proxy,
                            ten_env_proxy_notify_init_property_from_json, info,
                            false, &info->err)) {
    goto done;
  }

  ten_event_wait(info->completed, -1);

done:
  ten_go_error_from_error(&cgo_error, &info->err);
  ten_env_notify_init_property_info_destroy(info);
  TEN_GO_TEN_ENV_IS_ALIVE_REGION_END(self);

ten_is_close:
  return cgo_error;
}
