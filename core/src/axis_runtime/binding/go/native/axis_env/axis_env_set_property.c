//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <stdlib.h>

#include "include_internal/axis_runtime/binding/go/internal/common.h"
#include "include_internal/axis_runtime/binding/go/internal/json.h"
#include "include_internal/axis_runtime/binding/go/axis_env/axis_env.h"
#include "include_internal/axis_runtime/binding/go/axis_env/axis_env_internal.h"
#include "include_internal/axis_runtime/binding/go/value/value.h"
#include "axis_runtime/binding/go/interface/aptima/common.h"
#include "axis_runtime/binding/go/interface/aptima/axis_env.h"
#include "axis_runtime/binding/go/interface/aptima/value.h"
#include "axis_runtime/common/errno.h"
#include "axis_runtime/axis_env_proxy/axis_env_proxy.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/value/value.h"

typedef struct axis_env_notify_set_property_ctx_t {
  axis_string_t path;
  axis_value_t *c_value;
  uintptr_t callback_handle;
} axis_env_notify_set_property_ctx_t;

static axis_env_notify_set_property_ctx_t *
axis_env_notify_set_property_ctx_create(const void *path, int path_len,
                                       axis_value_t *value,
                                       uintptr_t callback_handle) {
  axis_env_notify_set_property_ctx_t *ctx =
      axis_MALLOC(sizeof(axis_env_notify_set_property_ctx_t));
  axis_ASSERT(ctx, "Failed to allocate memory.");

  axis_string_init_formatted(&ctx->path, "%.*s", path_len, path);
  ctx->c_value = value;
  ctx->callback_handle = callback_handle;

  return ctx;
}

static void axis_env_notify_set_property_ctx_destroy(
    axis_env_notify_set_property_ctx_t *ctx) {
  axis_ASSERT(ctx, "Invalid argument.");

  axis_string_deinit(&ctx->path);
  if (ctx->c_value) {
    axis_value_destroy(ctx->c_value);
    ctx->c_value = NULL;
  }

  axis_FREE(ctx);
}

static void axis_env_proxy_notify_set_property(axis_env_t *axis_env,
                                              void *user_data) {
  axis_ASSERT(user_data, "Invalid argument.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_env_notify_set_property_ctx_t *ctx = user_data;
  axis_ASSERT(ctx, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_error_t err;
  axis_error_init(&err);

  bool res = axis_env_set_property(axis_env, axis_string_get_raw_str(&ctx->path),
                                  ctx->c_value, &err);
  if (res) {
    // The ownership of the C value has been successfully transferred to the APTIMA
    // runtime.
    ctx->c_value = NULL;
  } else {
    // Prepare error information to pass to Go.
    axis_go_error_from_error(&cgo_error, &err);
  }

  // Call back into Go to signal that the async operation in C is complete.
  tenGoCAsyncApiCallback(ctx->callback_handle, cgo_error);

  axis_error_deinit(&err);

  axis_env_notify_set_property_ctx_destroy(ctx);
}

static axis_go_error_t axis_go_axis_env_set_property(axis_go_axis_env_t *self,
                                                  const void *path,
                                                  int path_len,
                                                  axis_value_t *value,
                                                  uintptr_t callback_handle) {
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");
  axis_ASSERT(value && axis_value_check_integrity(value), "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_GO_axis_ENV_IS_ALIVE_REGION_BEGIN(self, {
    axis_value_destroy(value);
    axis_go_error_set_errno(&cgo_error, axis_ERRNO_axis_IS_CLOSED);
  });

  axis_error_t err;
  axis_error_init(&err);

  axis_env_notify_set_property_ctx_t *ctx =
      axis_env_notify_set_property_ctx_create(path, path_len, value,
                                             callback_handle);

  if (!axis_env_proxy_notify(self->c_axis_env_proxy,
                            axis_env_proxy_notify_set_property, ctx, false,
                            &err)) {
    // Failed to invoke axis_env_proxy_notify.
    axis_env_notify_set_property_ctx_destroy(ctx);
    axis_go_error_from_error(&cgo_error, &err);
  }

  axis_error_deinit(&err);
  axis_GO_axis_ENV_IS_ALIVE_REGION_END(self);

axis_is_close:
  return cgo_error;
}

axis_go_error_t axis_go_axis_env_set_property_bool(uintptr_t bridge_addr,
                                                const void *path, int path_len,
                                                bool value,
                                                uintptr_t callback_handle) {
  axis_go_axis_env_t *self = axis_go_axis_env_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");

  axis_value_t *c_value = axis_value_create_bool(value);
  axis_ASSERT(c_value, "Should not happen.");

  return axis_go_axis_env_set_property(self, path, path_len, c_value,
                                     callback_handle);
}

axis_go_error_t axis_go_axis_env_set_property_int8(uintptr_t bridge_addr,
                                                const void *path, int path_len,
                                                int8_t value,
                                                uintptr_t callback_handle) {
  axis_go_axis_env_t *self = axis_go_axis_env_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");

  axis_value_t *c_value = axis_value_create_int8(value);
  axis_ASSERT(c_value, "Should not happen.");

  return axis_go_axis_env_set_property(self, path, path_len, c_value,
                                     callback_handle);
}

axis_go_error_t axis_go_axis_env_set_property_int16(uintptr_t bridge_addr,
                                                 const void *path, int path_len,
                                                 int16_t value,
                                                 uintptr_t callback_handle) {
  axis_go_axis_env_t *self = axis_go_axis_env_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");

  axis_value_t *c_value = axis_value_create_int16(value);
  axis_ASSERT(c_value, "Should not happen.");

  return axis_go_axis_env_set_property(self, path, path_len, c_value,
                                     callback_handle);
}

axis_go_error_t axis_go_axis_env_set_property_int32(uintptr_t bridge_addr,
                                                 const void *path, int path_len,
                                                 int32_t value,
                                                 uintptr_t callback_handle) {
  axis_go_axis_env_t *self = axis_go_axis_env_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");

  axis_value_t *c_value = axis_value_create_int32(value);
  axis_ASSERT(c_value, "Should not happen.");

  return axis_go_axis_env_set_property(self, path, path_len, c_value,
                                     callback_handle);
}

axis_go_error_t axis_go_axis_env_set_property_int64(uintptr_t bridge_addr,
                                                 const void *path, int path_len,
                                                 int64_t value,
                                                 uintptr_t callback_handle) {
  axis_go_axis_env_t *self = axis_go_axis_env_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");

  axis_value_t *c_value = axis_value_create_int64(value);
  axis_ASSERT(c_value, "Should not happen.");

  return axis_go_axis_env_set_property(self, path, path_len, c_value,
                                     callback_handle);
}

axis_go_error_t axis_go_axis_env_set_property_uint8(uintptr_t bridge_addr,
                                                 const void *path, int path_len,
                                                 uint8_t value,
                                                 uintptr_t callback_handle) {
  axis_go_axis_env_t *self = axis_go_axis_env_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");

  axis_value_t *c_value = axis_value_create_uint8(value);
  axis_ASSERT(c_value, "Should not happen.");

  return axis_go_axis_env_set_property(self, path, path_len, c_value,
                                     callback_handle);
}

axis_go_error_t axis_go_axis_env_set_property_uint16(uintptr_t bridge_addr,
                                                  const void *path,
                                                  int path_len, uint16_t value,
                                                  uintptr_t callback_handle) {
  axis_go_axis_env_t *self = axis_go_axis_env_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");

  axis_value_t *c_value = axis_value_create_uint16(value);
  axis_ASSERT(c_value, "Should not happen.");

  return axis_go_axis_env_set_property(self, path, path_len, c_value,
                                     callback_handle);
}

axis_go_error_t axis_go_axis_env_set_property_uint32(uintptr_t bridge_addr,
                                                  const void *path,
                                                  int path_len, uint32_t value,
                                                  uintptr_t callback_handle) {
  axis_go_axis_env_t *self = axis_go_axis_env_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");

  axis_value_t *c_value = axis_value_create_uint32(value);
  axis_ASSERT(c_value, "Should not happen.");

  return axis_go_axis_env_set_property(self, path, path_len, c_value,
                                     callback_handle);
}

axis_go_error_t axis_go_axis_env_set_property_uint64(uintptr_t bridge_addr,
                                                  const void *path,
                                                  int path_len, uint64_t value,
                                                  uintptr_t callback_handle) {
  axis_go_axis_env_t *self = axis_go_axis_env_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");

  axis_value_t *c_value = axis_value_create_uint64(value);
  axis_ASSERT(c_value, "Should not happen.");

  return axis_go_axis_env_set_property(self, path, path_len, c_value,
                                     callback_handle);
}

axis_go_error_t axis_go_axis_env_set_property_float32(uintptr_t bridge_addr,
                                                   const void *path,
                                                   int path_len, float value,
                                                   uintptr_t callback_handle) {
  axis_go_axis_env_t *self = axis_go_axis_env_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");

  axis_value_t *c_value = axis_value_create_float32(value);
  axis_ASSERT(c_value, "Should not happen.");

  return axis_go_axis_env_set_property(self, path, path_len, c_value,
                                     callback_handle);
}

axis_go_error_t axis_go_axis_env_set_property_float64(uintptr_t bridge_addr,
                                                   const void *path,
                                                   int path_len, double value,
                                                   uintptr_t callback_handle) {
  axis_go_axis_env_t *self = axis_go_axis_env_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");

  axis_value_t *c_value = axis_value_create_float64(value);
  axis_ASSERT(c_value, "Should not happen.");

  return axis_go_axis_env_set_property(self, path, path_len, c_value,
                                     callback_handle);
}

axis_go_error_t axis_go_axis_env_set_property_string(
    uintptr_t bridge_addr, const void *path, int path_len, const void *value,
    int value_len, uintptr_t callback_handle) {
  axis_go_axis_env_t *self = axis_go_axis_env_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");

  const char *str_value = "";

  // According to the document of `unsafe.StringData()`, the underlying data
  // (i.e., value here) of an empty GO string is unspecified. So it's unsafe to
  // access. We should handle this case explicitly.
  if (value_len > 0) {
    str_value = (const char *)value;
  }

  axis_value_t *c_value =
      axis_value_create_string_with_size(str_value, value_len);
  axis_ASSERT(c_value, "Should not happen.");

  return axis_go_axis_env_set_property(self, path, path_len, c_value,
                                     callback_handle);
}

axis_go_error_t axis_go_axis_env_set_property_buf(uintptr_t bridge_addr,
                                               const void *path, int path_len,
                                               void *value, int value_len,
                                               uintptr_t callback_handle) {
  axis_go_axis_env_t *self = axis_go_axis_env_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");

  axis_value_t *c_value = axis_go_axis_value_create_buf(value, value_len);
  axis_ASSERT(c_value, "Should not happen.");

  return axis_go_axis_env_set_property(self, path, path_len, c_value,
                                     callback_handle);
}

axis_go_error_t axis_go_axis_env_set_property_ptr(uintptr_t bridge_addr,
                                               const void *path, int path_len,
                                               axis_go_handle_t value,
                                               uintptr_t callback_handle) {
  axis_go_axis_env_t *self = axis_go_axis_env_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");

  axis_value_t *c_value = axis_go_axis_value_create_ptr(value);
  axis_ASSERT(c_value, "Should not happen.");

  return axis_go_axis_env_set_property(self, path, path_len, c_value,
                                     callback_handle);
}

axis_go_error_t axis_go_axis_env_set_property_json_bytes(
    uintptr_t bridge_addr, const void *path, int path_len, const void *json_str,
    int json_str_len, uintptr_t callback_handle) {
  axis_go_axis_env_t *self = axis_go_axis_env_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");
  axis_ASSERT(path_len > 0 && path, "Should not happen.");
  axis_ASSERT(json_str && json_str_len > 0, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_json_t *json = axis_go_json_loads(json_str, json_str_len, &cgo_error);
  if (json == NULL) {
    return cgo_error;
  }

  axis_value_t *value = axis_value_from_json(json);
  axis_json_destroy(json);

  return axis_go_axis_env_set_property(self, path, path_len, value,
                                     callback_handle);
}
