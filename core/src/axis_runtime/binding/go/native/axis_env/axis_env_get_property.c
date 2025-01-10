//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "include_internal/axis_runtime/binding/go/internal/common.h"
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
#include "axis_utils/lib/event.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_get.h"

typedef struct axis_env_notify_get_property_ctx_t {
  axis_string_t path;
  axis_value_t *c_value;
  axis_event_t *completed;
} axis_env_notify_get_property_ctx_t;

static axis_env_notify_get_property_ctx_t *
axis_env_notify_get_property_ctx_create(const void *path, int path_len) {
  axis_env_notify_get_property_ctx_t *ctx =
      axis_MALLOC(sizeof(axis_env_notify_get_property_ctx_t));
  axis_ASSERT(ctx, "Failed to allocate memory.");

  axis_string_init_formatted(&ctx->path, "%.*s", path_len, path);
  ctx->c_value = NULL;
  ctx->completed = axis_event_create(0, 1);

  return ctx;
}

static void axis_env_notify_get_property_ctx_destroy(
    axis_env_notify_get_property_ctx_t *ctx) {
  axis_ASSERT(ctx, "Invalid argument.");

  axis_string_deinit(&ctx->path);
  ctx->c_value = NULL;
  axis_event_destroy(ctx->completed);

  axis_FREE(ctx);
}

static void axis_env_proxy_notify_get_property(axis_env_t *axis_env,
                                              void *user_data) {
  axis_ASSERT(user_data, "Invalid argument.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_env_notify_get_property_ctx_t *ctx = user_data;
  axis_ASSERT(ctx, "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  // In the extension thread now.

  // The value shall be cloned (the third parameter of axis_get_property is
  // `true`) to ensure the value integrity.
  //
  // Imagine the following scenario:
  //
  // 1. There are two goroutine in one extension. Goroutine A wants to get the
  //    property "p" from the aptima instance bound to the extension, and goroutine
  //    B wants to update the property "p" in the same aptima instance. Goroutine A
  //    and B run in parallel, that A runs on thread M1 and B runs on thread M2
  //    in GO world.
  //
  // 2. Then the `get` and `set` operations are executed in the extension thread
  //    in order.
  //
  // 3. The `get` operation is executed first, then a `axis_value_t*` is passed
  //    to M1, and the extension thread starts to execute the `set` operation.
  //    If the `axis_value_t*` is not cloned from the extension thread, then a
  //    read operation from M1 and a write operation from the extension thread
  //    on the same `axis_value_t*` might happen in parallel.

  axis_value_t *c_value =
      axis_env_peek_property(axis_env, axis_string_get_raw_str(&ctx->path), &err);

  // Because this value will be passed out of the APTIMA world and back into the
  // GO world, and these two worlds are in different threads, copy semantics are
  // used to avoid thread safety issues.
  ctx->c_value = c_value ? axis_value_clone(c_value) : NULL;

  axis_event_set(ctx->completed);

  axis_error_deinit(&err);
}

static axis_value_t *axis_go_axis_env_get_property_and_check_if_exists(
    axis_go_axis_env_t *self, const void *path, int path_len,
    axis_go_error_t *status) {
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");

  axis_value_t *c_value = NULL;

  axis_error_t err;
  axis_error_init(&err);

  axis_env_notify_get_property_ctx_t *ctx =
      axis_env_notify_get_property_ctx_create(path, path_len);

  if (!axis_env_proxy_notify(self->c_axis_env_proxy,
                            axis_env_proxy_notify_get_property, ctx, false,
                            &err)) {
    axis_go_error_from_error(status, &err);
    goto done;
  }

  // The axis_go_axis_env_get_property_and_check_if_exists() is called from a
  // goroutine in GO world. The goroutine runs on a OS thread (i.e., M is GO
  // world), and the M won't be scheduled to other goroutine until the cgo call
  // is completed (i.e., this function returns). The following
  // `axis_event_wait()` may block the M which might lead to an increase of
  // creating new M in GO world. Especially when there are a many messages
  // piling up in the eventloop.
  //
  // TODO(Liu): compare the performance of the following two implementations.
  //
  // 1. Use `axis_event_wait()` to block the M, then the GO function is a sync
  // call.
  //
  // 2. The C function is always async, using a callback to notify the GO world.
  // There are a c-to-go call and a channel wait in GO.

  axis_event_wait(ctx->completed, -1);
  c_value = ctx->c_value;
  if (c_value == NULL) {
    axis_go_error_set_errno(status, axis_ERRNO_GENERIC);
  }

done:
  axis_error_deinit(&err);
  axis_env_notify_get_property_ctx_destroy(ctx);

  return c_value;
}

axis_go_error_t axis_go_axis_env_get_property_type_and_size(
    uintptr_t bridge_addr, const void *path, int path_len, uint8_t *type,
    uintptr_t *size, uintptr_t *value_addr) {
  axis_go_axis_env_t *self = axis_go_axis_env_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");
  axis_ASSERT(type && size, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_GO_axis_ENV_IS_ALIVE_REGION_BEGIN(
      self, { axis_go_error_set_errno(&cgo_error, axis_ERRNO_axis_IS_CLOSED); });

  axis_value_t *c_value = axis_go_axis_env_get_property_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (c_value != NULL) {
    axis_go_axis_value_get_type_and_size(c_value, type, size);

    // The c_value is cloned from APTIMA runtime, refer to the comments in
    // axis_notify_get_property.
    //
    // A property will be retrieved according to the following two steps.
    //
    // 1. Call this function (axis_go_axis_property_get_type_and_size) to get the
    //    type and size. And do some preparation in GO world, ex: make a slice
    //    to store the buffer.
    //
    // 2. Call the best match function to get the property value. Ex:
    //    axis_go_axis_property_get_int8.
    //
    // However, the value of the property might be modified between step 1 and 2
    // by other goroutine. That's also why the c_value is cloned in step 1. The
    // value (i.e., the pointer to the axis_value_t and the data in the
    // axis_value_t) operated in step 1 and 2 must be the same. Otherwise, the
    // data in the axis_value_t might be corrupted, especially the type or size
    // might be changed. So we have to keep the c_value as a returned value of
    // this function, and the same c_value has to be passed to the step 2, and
    // to be destroyed in step 2.
    *value_addr = (uintptr_t)c_value;
  }

  axis_GO_axis_ENV_IS_ALIVE_REGION_END(self);

axis_is_close:
  return cgo_error;
}

axis_go_error_t axis_go_axis_env_get_property_int8(uintptr_t bridge_addr,
                                                const void *path, int path_len,
                                                int8_t *value) {
  axis_go_axis_env_t *self = axis_go_axis_env_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");
  axis_ASSERT(value, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_GO_axis_ENV_IS_ALIVE_REGION_BEGIN(
      self, { axis_go_error_set_errno(&cgo_error, axis_ERRNO_axis_IS_CLOSED); });

  axis_value_t *c_value = axis_go_axis_env_get_property_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (c_value != NULL) {
    axis_error_t err;
    axis_error_init(&err);

    *value = axis_value_get_int8(c_value, &err);

    axis_go_error_from_error(&cgo_error, &err);
    axis_error_deinit(&err);

    // The c_value is cloned from APTIMA runtime, so we have to destroy it.
    axis_value_destroy(c_value);
  }

  axis_GO_axis_ENV_IS_ALIVE_REGION_END(self);

axis_is_close:
  return cgo_error;
}

axis_go_error_t axis_go_axis_env_get_property_int16(uintptr_t bridge_addr,
                                                 const void *path, int path_len,
                                                 int16_t *value) {
  axis_go_axis_env_t *self = axis_go_axis_env_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");
  axis_ASSERT(value, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_GO_axis_ENV_IS_ALIVE_REGION_BEGIN(
      self, { axis_go_error_set_errno(&cgo_error, axis_ERRNO_axis_IS_CLOSED); });

  axis_value_t *c_value = axis_go_axis_env_get_property_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (c_value != NULL) {
    axis_error_t err;
    axis_error_init(&err);

    *value = axis_value_get_int16(c_value, &err);

    axis_go_error_from_error(&cgo_error, &err);
    axis_error_deinit(&err);

    // The c_value is cloned from APTIMA runtime, so we have to destroy it.
    axis_value_destroy(c_value);
  }

  axis_GO_axis_ENV_IS_ALIVE_REGION_END(self);

axis_is_close:
  return cgo_error;
}

axis_go_error_t axis_go_axis_env_get_property_int32(uintptr_t bridge_addr,
                                                 const void *path, int path_len,
                                                 int32_t *value) {
  axis_go_axis_env_t *self = axis_go_axis_env_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");
  axis_ASSERT(value, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_GO_axis_ENV_IS_ALIVE_REGION_BEGIN(
      self, { axis_go_error_set_errno(&cgo_error, axis_ERRNO_axis_IS_CLOSED); });

  axis_value_t *c_value = axis_go_axis_env_get_property_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (c_value != NULL) {
    axis_error_t err;
    axis_error_init(&err);

    *value = axis_value_get_int32(c_value, &err);

    axis_go_error_from_error(&cgo_error, &err);
    axis_error_deinit(&err);

    // The c_value is cloned from APTIMA runtime, so we have to destroy it.
    axis_value_destroy(c_value);
  }

  axis_GO_axis_ENV_IS_ALIVE_REGION_END(self);

axis_is_close:
  return cgo_error;
}

axis_go_error_t axis_go_axis_env_get_property_int64(uintptr_t bridge_addr,
                                                 const void *path, int path_len,
                                                 int64_t *value) {
  axis_go_axis_env_t *self = axis_go_axis_env_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");
  axis_ASSERT(value, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_GO_axis_ENV_IS_ALIVE_REGION_BEGIN(
      self, { axis_go_error_set_errno(&cgo_error, axis_ERRNO_axis_IS_CLOSED); });

  axis_value_t *c_value = axis_go_axis_env_get_property_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (c_value != NULL) {
    axis_error_t err;
    axis_error_init(&err);

    *value = axis_value_get_int64(c_value, &err);

    axis_go_error_from_error(&cgo_error, &err);
    axis_error_deinit(&err);

    // The c_value is cloned from APTIMA runtime, so we have to destroy it.
    axis_value_destroy(c_value);
  }

  axis_GO_axis_ENV_IS_ALIVE_REGION_END(self);

axis_is_close:
  return cgo_error;
}

axis_go_error_t axis_go_axis_env_get_property_uint8(uintptr_t bridge_addr,
                                                 const void *path, int path_len,
                                                 uint8_t *value) {
  axis_go_axis_env_t *self = axis_go_axis_env_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");
  axis_ASSERT(value, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_GO_axis_ENV_IS_ALIVE_REGION_BEGIN(
      self, { axis_go_error_set_errno(&cgo_error, axis_ERRNO_axis_IS_CLOSED); });

  axis_value_t *c_value = axis_go_axis_env_get_property_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (c_value != NULL) {
    axis_error_t err;
    axis_error_init(&err);

    *value = axis_value_get_uint8(c_value, &err);

    axis_go_error_from_error(&cgo_error, &err);
    axis_error_deinit(&err);

    // The c_value is cloned from APTIMA runtime, so we have to destroy it.
    axis_value_destroy(c_value);
  }

  axis_GO_axis_ENV_IS_ALIVE_REGION_END(self);

axis_is_close:
  return cgo_error;
}

axis_go_error_t axis_go_axis_env_get_property_uint16(uintptr_t bridge_addr,
                                                  const void *path,
                                                  int path_len,
                                                  uint16_t *value) {
  axis_go_axis_env_t *self = axis_go_axis_env_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");
  axis_ASSERT(value, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_GO_axis_ENV_IS_ALIVE_REGION_BEGIN(
      self, { axis_go_error_set_errno(&cgo_error, axis_ERRNO_axis_IS_CLOSED); });

  axis_value_t *c_value = axis_go_axis_env_get_property_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (c_value != NULL) {
    axis_error_t err;
    axis_error_init(&err);

    *value = axis_value_get_uint16(c_value, &err);

    axis_go_error_from_error(&cgo_error, &err);
    axis_error_deinit(&err);

    // The c_value is cloned from APTIMA runtime, so we have to destroy it.
    axis_value_destroy(c_value);
  }

  axis_GO_axis_ENV_IS_ALIVE_REGION_END(self);

axis_is_close:
  return cgo_error;
}

axis_go_error_t axis_go_axis_env_get_property_uint32(uintptr_t bridge_addr,
                                                  const void *path,
                                                  int path_len,
                                                  uint32_t *value) {
  axis_go_axis_env_t *self = axis_go_axis_env_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");
  axis_ASSERT(value, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_GO_axis_ENV_IS_ALIVE_REGION_BEGIN(
      self, { axis_go_error_set_errno(&cgo_error, axis_ERRNO_axis_IS_CLOSED); });

  axis_value_t *c_value = axis_go_axis_env_get_property_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (c_value != NULL) {
    axis_error_t err;
    axis_error_init(&err);

    *value = axis_value_get_uint32(c_value, &err);

    axis_go_error_from_error(&cgo_error, &err);
    axis_error_deinit(&err);

    // The c_value is cloned from APTIMA runtime, so we have to destroy it.
    axis_value_destroy(c_value);
  }

  axis_GO_axis_ENV_IS_ALIVE_REGION_END(self);

axis_is_close:
  return cgo_error;
}

axis_go_error_t axis_go_axis_env_get_property_uint64(uintptr_t bridge_addr,
                                                  const void *path,
                                                  int path_len,
                                                  uint64_t *value) {
  axis_go_axis_env_t *self = axis_go_axis_env_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");
  axis_ASSERT(value, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_GO_axis_ENV_IS_ALIVE_REGION_BEGIN(
      self, { axis_go_error_set_errno(&cgo_error, axis_ERRNO_axis_IS_CLOSED); });

  axis_value_t *c_value = axis_go_axis_env_get_property_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (c_value != NULL) {
    axis_error_t err;
    axis_error_init(&err);

    *value = axis_value_get_uint64(c_value, &err);

    axis_go_error_from_error(&cgo_error, &err);
    axis_error_deinit(&err);

    // The c_value is cloned from APTIMA runtime, so we have to destroy it.
    axis_value_destroy(c_value);
  }

  axis_GO_axis_ENV_IS_ALIVE_REGION_END(self);

axis_is_close:
  return cgo_error;
}

axis_go_error_t axis_go_axis_env_get_property_float32(uintptr_t bridge_addr,
                                                   const void *path,
                                                   int path_len, float *value) {
  axis_go_axis_env_t *self = axis_go_axis_env_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");
  axis_ASSERT(value, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_GO_axis_ENV_IS_ALIVE_REGION_BEGIN(
      self, { axis_go_error_set_errno(&cgo_error, axis_ERRNO_axis_IS_CLOSED); });

  axis_value_t *c_value = axis_go_axis_env_get_property_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (c_value != NULL) {
    axis_error_t err;
    axis_error_init(&err);

    *value = axis_value_get_float32(c_value, &err);

    axis_go_error_from_error(&cgo_error, &err);
    axis_error_deinit(&err);

    // The c_value is cloned from APTIMA runtime, so we have to destroy it.
    axis_value_destroy(c_value);
  }

  axis_GO_axis_ENV_IS_ALIVE_REGION_END(self);

axis_is_close:
  return cgo_error;
}

axis_go_error_t axis_go_axis_env_get_property_float64(uintptr_t bridge_addr,
                                                   const void *path,
                                                   int path_len,
                                                   double *value) {
  axis_go_axis_env_t *self = axis_go_axis_env_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");
  axis_ASSERT(value, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_GO_axis_ENV_IS_ALIVE_REGION_BEGIN(
      self, { axis_go_error_set_errno(&cgo_error, axis_ERRNO_axis_IS_CLOSED); });

  axis_value_t *c_value = axis_go_axis_env_get_property_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (c_value != NULL) {
    axis_error_t err;
    axis_error_init(&err);

    *value = axis_value_get_float64(c_value, &err);

    axis_go_error_from_error(&cgo_error, &err);
    axis_error_deinit(&err);

    // The c_value is cloned from APTIMA runtime, so we have to destroy it.
    axis_value_destroy(c_value);
  }

  axis_GO_axis_ENV_IS_ALIVE_REGION_END(self);

axis_is_close:
  return cgo_error;
}

axis_go_error_t axis_go_axis_env_get_property_bool(uintptr_t bridge_addr,
                                                const void *path, int path_len,
                                                bool *value) {
  axis_go_axis_env_t *self = axis_go_axis_env_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");
  axis_ASSERT(value, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_GO_axis_ENV_IS_ALIVE_REGION_BEGIN(
      self, { axis_go_error_set_errno(&cgo_error, axis_ERRNO_axis_IS_CLOSED); });

  axis_value_t *c_value = axis_go_axis_env_get_property_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (c_value != NULL) {
    axis_error_t err;
    axis_error_init(&err);

    *value = axis_value_get_bool(c_value, &err);

    axis_go_error_from_error(&cgo_error, &err);
    axis_error_deinit(&err);

    // The c_value is cloned from APTIMA runtime, so we have to destroy it.
    axis_value_destroy(c_value);
  }

  axis_GO_axis_ENV_IS_ALIVE_REGION_END(self);

axis_is_close:
  return cgo_error;
}

axis_go_error_t axis_go_axis_env_get_property_ptr(uintptr_t bridge_addr,
                                               const void *path, int path_len,
                                               axis_go_handle_t *value) {
  axis_go_axis_env_t *self = axis_go_axis_env_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");
  axis_ASSERT(value, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_GO_axis_ENV_IS_ALIVE_REGION_BEGIN(
      self, { axis_go_error_set_errno(&cgo_error, axis_ERRNO_axis_IS_CLOSED); });

  axis_value_t *c_value = axis_go_axis_env_get_property_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (c_value != NULL) {
    axis_go_axis_value_get_ptr(c_value, value, &cgo_error);

    // The c_value is cloned from APTIMA runtime, so we have to destroy it.
    axis_value_destroy(c_value);
  }

  axis_GO_axis_ENV_IS_ALIVE_REGION_END(self);

axis_is_close:
  return cgo_error;
}

axis_go_error_t axis_go_axis_env_get_property_json_and_size(
    uintptr_t bridge_addr, const void *path, int path_len,
    uintptr_t *json_str_len, const char **json_str) {
  axis_go_axis_env_t *self = axis_go_axis_env_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");
  axis_ASSERT(json_str && json_str_len > 0, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_GO_axis_ENV_IS_ALIVE_REGION_BEGIN(
      self, { axis_go_error_set_errno(&cgo_error, axis_ERRNO_axis_IS_CLOSED); });

  axis_value_t *value = axis_go_axis_env_get_property_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (value != NULL) {
    axis_go_axis_value_to_json(value, json_str_len, json_str, &cgo_error);

    // The c_value is cloned from APTIMA runtime, so we have to destroy it.
    axis_value_destroy(value);
  }

  axis_GO_axis_ENV_IS_ALIVE_REGION_END(self);

axis_is_close:
  return cgo_error;
}
