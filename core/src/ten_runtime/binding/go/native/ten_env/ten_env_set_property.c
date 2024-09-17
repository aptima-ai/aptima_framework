//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <stdlib.h>

#include "include_internal/ten_runtime/binding/go/internal/common.h"
#include "include_internal/ten_runtime/binding/go/internal/json.h"
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env.h"
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env_internal.h"
#include "include_internal/ten_runtime/binding/go/value/value.h"
#include "ten_utils/macro/check.h"
#include "ten_runtime/binding/go/interface/ten/common.h"
#include "ten_runtime/binding/go/interface/ten/ten_env.h"
#include "ten_runtime/binding/go/interface/ten/value.h"
#include "ten_runtime/common/errno.h"
#include "ten_runtime/ten_env_proxy/ten_env_proxy.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/lib/event.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/value/value.h"

typedef struct ten_env_notify_set_property_info_t {
  bool result;
  ten_string_t path;
  ten_value_t *c_value;
  ten_event_t *completed;
} ten_env_notify_set_property_info_t;

static ten_env_notify_set_property_info_t *
ten_env_notify_set_property_info_create(const void *path, int path_len,
                                        ten_value_t *value) {
  ten_env_notify_set_property_info_t *info =
      TEN_MALLOC(sizeof(ten_env_notify_set_property_info_t));
  TEN_ASSERT(info, "Failed to allocate memory.");

  info->result = true;
  ten_string_init_formatted(&info->path, "%.*s", path_len, path);
  info->c_value = value;
  info->completed = ten_event_create(0, 1);

  return info;
}

static void ten_env_notify_set_property_info_destroy(
    ten_env_notify_set_property_info_t *self) {
  TEN_ASSERT(self, "Invalid argument.");

  ten_string_deinit(&self->path);
  self->c_value = NULL;
  ten_event_destroy(self->completed);

  TEN_FREE(self);
}

static void ten_env_notify_set_property(ten_env_t *ten_env, void *user_data) {
  TEN_ASSERT(user_data, "Invalid argument.");
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_env_notify_set_property_info_t *info = user_data;
  TEN_ASSERT(info, "Should not happen.");

  ten_error_t err;
  ten_error_init(&err);

  info->result = ten_env_set_property(
      ten_env, ten_string_get_raw_str(&info->path), info->c_value, &err);
  TEN_ASSERT(info->result, "Should not happen.");

  ten_event_set(info->completed);

  ten_error_deinit(&err);
}

static void ten_go_ten_env_set_property(ten_go_ten_env_t *self,
                                        const void *path, int path_len,
                                        ten_value_t *value,
                                        ten_go_status_t *status) {
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");
  TEN_ASSERT(value && ten_value_check_integrity(value), "Should not happen.");

  TEN_GO_TEN_IS_ALIVE_REGION_BEGIN(self, {
    ten_value_destroy(value);
    ten_go_status_set_errno(status, TEN_ERRNO_TEN_IS_CLOSED);
  });

  ten_error_t err;
  ten_error_init(&err);

  ten_env_notify_set_property_info_t *info =
      ten_env_notify_set_property_info_create(path, path_len, value);

  if (!ten_env_proxy_notify(self->c_ten_env_proxy, ten_env_notify_set_property,
                            info, false, &err)) {
    ten_go_status_from_error(status, &err);
    goto done;
  }

  ten_event_wait(info->completed, -1);

done:
  ten_env_notify_set_property_info_destroy(info);

  ten_error_deinit(&err);

  TEN_GO_TEN_IS_ALIVE_REGION_END(self);

ten_is_close:
  return;
}

ten_go_status_t ten_go_ten_env_set_property_bool(uintptr_t bridge_addr,
                                                 const void *path, int path_len,
                                                 bool value) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_value_t *c_value = ten_value_create_bool(value);
  TEN_ASSERT(c_value, "Should not happen.");

  ten_go_ten_env_set_property(self, path, path_len, c_value, &status);

  return status;
}

ten_go_status_t ten_go_ten_env_set_property_int8(uintptr_t bridge_addr,
                                                 const void *path, int path_len,
                                                 int8_t value) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_value_t *c_value = ten_value_create_int8(value);
  TEN_ASSERT(c_value, "Should not happen.");

  ten_go_ten_env_set_property(self, path, path_len, c_value, &status);

  return status;
}

ten_go_status_t ten_go_ten_env_set_property_int16(uintptr_t bridge_addr,
                                                  const void *path,
                                                  int path_len, int16_t value) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_value_t *c_value = ten_value_create_int16(value);
  TEN_ASSERT(c_value, "Should not happen.");

  ten_go_ten_env_set_property(self, path, path_len, c_value, &status);

  return status;
}

ten_go_status_t ten_go_ten_env_set_property_int32(uintptr_t bridge_addr,
                                                  const void *path,
                                                  int path_len, int32_t value) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_value_t *c_value = ten_value_create_int32(value);
  TEN_ASSERT(c_value, "Should not happen.");

  ten_go_ten_env_set_property(self, path, path_len, c_value, &status);

  return status;
}

ten_go_status_t ten_go_ten_env_set_property_int64(uintptr_t bridge_addr,
                                                  const void *path,
                                                  int path_len, int64_t value) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_value_t *c_value = ten_value_create_int64(value);
  TEN_ASSERT(c_value, "Should not happen.");

  ten_go_ten_env_set_property(self, path, path_len, c_value, &status);

  return status;
}

ten_go_status_t ten_go_ten_env_set_property_uint8(uintptr_t bridge_addr,
                                                  const void *path,
                                                  int path_len, uint8_t value) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_value_t *c_value = ten_value_create_uint8(value);
  TEN_ASSERT(c_value, "Should not happen.");

  ten_go_ten_env_set_property(self, path, path_len, c_value, &status);

  return status;
}

ten_go_status_t ten_go_ten_env_set_property_uint16(uintptr_t bridge_addr,
                                                   const void *path,
                                                   int path_len,
                                                   uint16_t value) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_value_t *c_value = ten_value_create_uint16(value);
  TEN_ASSERT(c_value, "Should not happen.");

  ten_go_ten_env_set_property(self, path, path_len, c_value, &status);

  return status;
}

ten_go_status_t ten_go_ten_env_set_property_uint32(uintptr_t bridge_addr,
                                                   const void *path,
                                                   int path_len,
                                                   uint32_t value) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_value_t *c_value = ten_value_create_uint32(value);
  TEN_ASSERT(c_value, "Should not happen.");

  ten_go_ten_env_set_property(self, path, path_len, c_value, &status);

  return status;
}

ten_go_status_t ten_go_ten_env_set_property_uint64(uintptr_t bridge_addr,
                                                   const void *path,
                                                   int path_len,
                                                   uint64_t value) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_value_t *c_value = ten_value_create_uint64(value);
  TEN_ASSERT(c_value, "Should not happen.");

  ten_go_ten_env_set_property(self, path, path_len, c_value, &status);

  return status;
}

ten_go_status_t ten_go_ten_env_set_property_float32(uintptr_t bridge_addr,
                                                    const void *path,
                                                    int path_len, float value) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_value_t *c_value = ten_value_create_float32(value);
  TEN_ASSERT(c_value, "Should not happen.");

  ten_go_ten_env_set_property(self, path, path_len, c_value, &status);

  return status;
}

ten_go_status_t ten_go_ten_env_set_property_float64(uintptr_t bridge_addr,
                                                    const void *path,
                                                    int path_len,
                                                    double value) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_value_t *c_value = ten_value_create_float64(value);
  TEN_ASSERT(c_value, "Should not happen.");

  ten_go_ten_env_set_property(self, path, path_len, c_value, &status);

  return status;
}

ten_go_status_t ten_go_ten_env_set_property_string(uintptr_t bridge_addr,
                                                   const void *path,
                                                   int path_len,
                                                   const void *value,
                                                   int value_len) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  const char *str_value = "";

  // According to the document of `unsafe.StringData()`, the underlying data
  // (i.e., value here) of an empty GO string is unspecified. So it's unsafe to
  // access. We should handle this case explicitly.
  if (value_len > 0) {
    str_value = (const char *)value;
  }

  ten_value_t *c_value =
      ten_value_create_string_with_size(str_value, value_len);
  TEN_ASSERT(c_value, "Should not happen.");

  ten_go_ten_env_set_property(self, path, path_len, c_value, &status);

  return status;
}

ten_go_status_t ten_go_ten_env_set_property_buf(uintptr_t bridge_addr,
                                                const void *path, int path_len,
                                                void *value, int value_len) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_value_t *c_value = ten_go_ten_value_create_buf(value, value_len);
  TEN_ASSERT(c_value, "Should not happen.");

  ten_go_ten_env_set_property(self, path, path_len, c_value, &status);

  return status;
}

ten_go_status_t ten_go_ten_env_set_property_ptr(uintptr_t bridge_addr,
                                                const void *path, int path_len,
                                                ten_go_handle_t value) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_value_t *c_value = ten_go_ten_value_create_ptr(value);
  TEN_ASSERT(c_value, "Should not happen.");

  ten_go_ten_env_set_property(self, path, path_len, c_value, &status);

  return status;
}

ten_go_status_t ten_go_ten_env_set_property_json_bytes(uintptr_t bridge_addr,
                                                       const void *path,
                                                       int path_len,
                                                       const void *json_str,
                                                       int json_str_len) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");
  TEN_ASSERT(path_len > 0 && path, "Should not happen.");
  TEN_ASSERT(json_str && json_str_len > 0, "Should not happen.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_json_t *json = ten_go_json_loads(json_str, json_str_len, &status);
  if (json == NULL) {
    return status;
  }

  ten_value_t *value = ten_value_from_json(json);
  ten_json_destroy(json);

  ten_go_ten_env_set_property(self, path, path_len, value, &status);

  return status;
}
