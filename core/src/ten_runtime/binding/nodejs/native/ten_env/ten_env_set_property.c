//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/binding/nodejs/ten_env/ten_env.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/macro/memory.h"

typedef struct ten_env_notify_set_property_info_t {
  ten_string_t path;
  ten_value_t *value;
  ten_nodejs_tsfn_t *js_cb;
} ten_env_notify_set_property_info_t;

static ten_env_notify_set_property_info_t *
ten_env_notify_set_property_info_create(const void *path, ten_value_t *value,
                                        ten_nodejs_tsfn_t *js_cb) {
  ten_env_notify_set_property_info_t *info =
      TEN_MALLOC(sizeof(ten_env_notify_set_property_info_t));
  TEN_ASSERT(info, "Failed to allocate memory.");

  ten_string_init_from_c_str(&info->path, path, strlen(path));
  info->value = value;
  info->js_cb = js_cb;

  return info;
}

static void ten_env_notify_set_property_info_destroy(
    ten_env_notify_set_property_info_t *info) {
  TEN_ASSERT(info, "Invalid argument.");

  ten_string_deinit(&info->path);

  if (info->value) {
    ten_value_destroy(info->value);
  }

  TEN_FREE(info);
}

static void ten_env_proxy_notify_set_property(ten_env_t *ten_env,
                                              void *user_data) {
  TEN_ASSERT(user_data, "Invalid argument.");
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_env_notify_set_property_info_t *info = user_data;
  TEN_ASSERT(info, "Should not happen.");

  ten_nodejs_tsfn_t *js_cb = info->js_cb;
  TEN_ASSERT(js_cb && ten_nodejs_tsfn_check_integrity(js_cb, false),
             "Should not happen.");

  ten_error_t err;
  ten_error_init(&err);

  bool rc = ten_env_set_property(ten_env, ten_string_get_raw_str(&info->path),
                                 info->value, &err);

  ten_error_t *cloned_error = NULL;
  if (!rc) {
    cloned_error = ten_error_create();
    ten_error_copy(cloned_error, &err);
  }

  ten_nodejs_set_property_call_info_t *call_info =
      ten_nodejs_set_property_call_info_create(js_cb, rc, cloned_error);
  TEN_ASSERT(call_info, "Should not happen.");

  bool call_js_rc = ten_nodejs_tsfn_invoke(js_cb, call_info);
  TEN_ASSERT(call_js_rc, "Should not happen.");

  if (rc) {
    // The ownership of the value is transferred to the runtime.
    info->value = NULL;
  }

  ten_env_notify_set_property_info_destroy(info);

  ten_error_deinit(&err);
}

bool ten_nodejs_ten_env_set_property_value(ten_nodejs_ten_env_t *self,
                                           const char *path, ten_value_t *value,
                                           ten_nodejs_tsfn_t *cb_tsfn,
                                           ten_error_t *error) {
  TEN_ASSERT(self && ten_nodejs_ten_env_check_integrity(self, true),
             "Invalid argument.");
  TEN_ASSERT(path, "Invalid argument.");
  TEN_ASSERT(value && ten_value_check_integrity(value), "Invalid argument.");
  TEN_ASSERT(cb_tsfn && ten_nodejs_tsfn_check_integrity(cb_tsfn, true),
             "Invalid argument.");

  ten_env_notify_set_property_info_t *info =
      ten_env_notify_set_property_info_create(path, value, cb_tsfn);
  TEN_ASSERT(info, "Should not happen.");

  if (!ten_env_proxy_notify(self->c_ten_env_proxy,
                            ten_env_proxy_notify_set_property, info, false,
                            error)) {
    ten_env_notify_set_property_info_destroy(info);
    return false;
  }

  return true;
}
