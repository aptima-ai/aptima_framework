//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "include_internal/ten_runtime/binding/go/internal/common.h"
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env.h"
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env_internal.h"
#include "include_internal/ten_runtime/binding/go/value/value.h"
#include "ten_runtime/binding/go/interface/ten/common.h"
#include "ten_runtime/binding/go/interface/ten/ten_env.h"
#include "ten_runtime/binding/go/interface/ten/value.h"
#include "ten_runtime/common/errno.h"
#include "ten_runtime/ten_env_proxy/ten_env_proxy.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/lib/event.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/value/value.h"
#include "ten_utils/value/value_get.h"

typedef struct ten_env_notify_get_property_info_t {
  ten_string_t path;
  ten_value_t *c_value;
  ten_event_t *completed;
} ten_env_notify_get_property_info_t;

static ten_env_notify_get_property_info_t *
ten_env_notify_get_property_info_create(const void *path, int path_len) {
  ten_env_notify_get_property_info_t *info =
      TEN_MALLOC(sizeof(ten_env_notify_get_property_info_t));
  TEN_ASSERT(info, "Failed to allocate memory.");

  ten_string_init_formatted(&info->path, "%.*s", path_len, path);
  info->c_value = NULL;
  info->completed = ten_event_create(0, 1);

  return info;
}

static void ten_env_notify_get_property_info_destroy(
    ten_env_notify_get_property_info_t *info) {
  TEN_ASSERT(info, "Invalid argument.");

  ten_string_deinit(&info->path);
  info->c_value = NULL;
  ten_event_destroy(info->completed);

  TEN_FREE(info);
}

static void ten_env_proxy_notify_get_property(ten_env_t *ten_env,
                                              void *user_data) {
  TEN_ASSERT(user_data, "Invalid argument.");
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_env_notify_get_property_info_t *info = user_data;
  TEN_ASSERT(info, "Should not happen.");

  ten_error_t err;
  ten_error_init(&err);

  // In the extension thread now.

  // The value shall be cloned (the third parameter of ten_get_property is
  // `true`) to ensure the value integrity.
  //
  // Imagine the following scenario:
  //
  // 1. There are two goroutine in one extension. Goroutine A wants to get the
  //    property "p" from the ten instance bound to the extension, and goroutine
  //    B wants to update the property "p" in the same ten instance. Goroutine A
  //    and B run in parallel, that A runs on thread M1 and B runs on thread M2
  //    in GO world.
  //
  // 2. Then the `get` and `set` operations are executed in the extension thread
  //    in order.
  //
  // 3. The `get` operation is executed first, then a `ten_value_t*` is passed
  //    to M1, and the extension thread starts to execute the `set` operation.
  //    If the `ten_value_t*` is not cloned from the extension thread, then a
  //    read operation from M1 and a write operation from the extension thread
  //    on the same `ten_value_t*` might happen in parallel.

  ten_value_t *c_value =
      ten_env_peek_property(ten_env, ten_string_get_raw_str(&info->path), &err);

  // Because this value will be passed out of the TEN world and back into the
  // GO world, and these two worlds are in different threads, copy semantics are
  // used to avoid thread safety issues.
  info->c_value = c_value ? ten_value_clone(c_value) : NULL;

  ten_event_set(info->completed);

  ten_error_deinit(&err);
}

static ten_value_t *ten_go_ten_env_get_property_and_check_if_exists(
    ten_go_ten_env_t *self, const void *path, int path_len,
    ten_go_error_t *status) {
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");

  ten_value_t *c_value = NULL;

  ten_error_t err;
  ten_error_init(&err);

  ten_env_notify_get_property_info_t *info =
      ten_env_notify_get_property_info_create(path, path_len);

  if (!ten_env_proxy_notify(self->c_ten_env_proxy,
                            ten_env_proxy_notify_get_property, info, false,
                            &err)) {
    ten_go_error_from_error(status, &err);
    goto done;
  }

  // The ten_go_ten_env_get_property_and_check_if_exists() is called from a
  // goroutine in GO world. The goroutine runs on a OS thread (i.e., M is GO
  // world), and the M won't be scheduled to other goroutine until the cgo call
  // is completed (i.e., this function returns). The following
  // `ten_event_wait()` may block the M which might lead to an increase of
  // creating new M in GO world. Especially when there are a many messages
  // piling up in the eventloop.
  //
  // TODO(Liu): compare the performance of the following two implementations.
  //
  // 1. Use `ten_event_wait()` to block the M, then the GO function is a sync
  // call.
  //
  // 2. The C function is always async, using a callback to notify the GO world.
  // There are a c-to-go call and a channel wait in GO.

  ten_event_wait(info->completed, -1);
  c_value = info->c_value;
  if (c_value == NULL) {
    ten_go_error_set_errno(status, TEN_ERRNO_GENERIC);
  }

done:
  ten_error_deinit(&err);
  ten_env_notify_get_property_info_destroy(info);

  return c_value;
}

ten_go_error_t ten_go_ten_env_get_property_type_and_size(
    uintptr_t bridge_addr, const void *path, int path_len, uint8_t *type,
    uintptr_t *size, uintptr_t *value_addr) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");
  TEN_ASSERT(type && size, "Should not happen.");

  ten_go_error_t cgo_error;
  ten_go_error_init_with_errno(&cgo_error, TEN_ERRNO_OK);

  TEN_GO_TEN_ENV_IS_ALIVE_REGION_BEGIN(
      self, { ten_go_error_set_errno(&cgo_error, TEN_ERRNO_TEN_IS_CLOSED); });

  ten_value_t *c_value = ten_go_ten_env_get_property_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (c_value != NULL) {
    ten_go_ten_value_get_type_and_size(c_value, type, size);

    // The c_value is cloned from TEN runtime, refer to the comments in
    // ten_notify_get_property.
    //
    // A property will be retrieved according to the following two steps.
    //
    // 1. Call this function (ten_go_ten_property_get_type_and_size) to get the
    //    type and size. And do some preparation in GO world, ex: make a slice
    //    to store the buffer.
    //
    // 2. Call the best match function to get the property value. Ex:
    //    ten_go_ten_property_get_int8.
    //
    // However, the value of the property might be modified between step 1 and 2
    // by other goroutine. That's also why the c_value is cloned in step 1. The
    // value (i.e., the pointer to the ten_value_t and the data in the
    // ten_value_t) operated in step 1 and 2 must be the same. Otherwise, the
    // data in the ten_value_t might be corrupted, especially the type or size
    // might be changed. So we have to keep the c_value as a returned value of
    // this function, and the same c_value has to be passed to the step 2, and
    // to be destroyed in step 2.
    *value_addr = (uintptr_t)c_value;
  }

  TEN_GO_TEN_ENV_IS_ALIVE_REGION_END(self);

ten_is_close:
  return cgo_error;
}

ten_go_error_t ten_go_ten_env_get_property_int8(uintptr_t bridge_addr,
                                                const void *path, int path_len,
                                                int8_t *value) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");
  TEN_ASSERT(value, "Should not happen.");

  ten_go_error_t cgo_error;
  ten_go_error_init_with_errno(&cgo_error, TEN_ERRNO_OK);

  TEN_GO_TEN_ENV_IS_ALIVE_REGION_BEGIN(
      self, { ten_go_error_set_errno(&cgo_error, TEN_ERRNO_TEN_IS_CLOSED); });

  ten_value_t *c_value = ten_go_ten_env_get_property_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (c_value != NULL) {
    ten_error_t err;
    ten_error_init(&err);

    *value = ten_value_get_int8(c_value, &err);

    ten_go_error_from_error(&cgo_error, &err);
    ten_error_deinit(&err);

    // The c_value is cloned from TEN runtime, so we have to destroy it.
    ten_value_destroy(c_value);
  }

  TEN_GO_TEN_ENV_IS_ALIVE_REGION_END(self);

ten_is_close:
  return cgo_error;
}

ten_go_error_t ten_go_ten_env_get_property_int16(uintptr_t bridge_addr,
                                                 const void *path, int path_len,
                                                 int16_t *value) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");
  TEN_ASSERT(value, "Should not happen.");

  ten_go_error_t cgo_error;
  ten_go_error_init_with_errno(&cgo_error, TEN_ERRNO_OK);

  TEN_GO_TEN_ENV_IS_ALIVE_REGION_BEGIN(
      self, { ten_go_error_set_errno(&cgo_error, TEN_ERRNO_TEN_IS_CLOSED); });

  ten_value_t *c_value = ten_go_ten_env_get_property_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (c_value != NULL) {
    ten_error_t err;
    ten_error_init(&err);

    *value = ten_value_get_int16(c_value, &err);

    ten_go_error_from_error(&cgo_error, &err);
    ten_error_deinit(&err);

    // The c_value is cloned from TEN runtime, so we have to destroy it.
    ten_value_destroy(c_value);
  }

  TEN_GO_TEN_ENV_IS_ALIVE_REGION_END(self);

ten_is_close:
  return cgo_error;
}

ten_go_error_t ten_go_ten_env_get_property_int32(uintptr_t bridge_addr,
                                                 const void *path, int path_len,
                                                 int32_t *value) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");
  TEN_ASSERT(value, "Should not happen.");

  ten_go_error_t cgo_error;
  ten_go_error_init_with_errno(&cgo_error, TEN_ERRNO_OK);

  TEN_GO_TEN_ENV_IS_ALIVE_REGION_BEGIN(
      self, { ten_go_error_set_errno(&cgo_error, TEN_ERRNO_TEN_IS_CLOSED); });

  ten_value_t *c_value = ten_go_ten_env_get_property_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (c_value != NULL) {
    ten_error_t err;
    ten_error_init(&err);

    *value = ten_value_get_int32(c_value, &err);

    ten_go_error_from_error(&cgo_error, &err);
    ten_error_deinit(&err);

    // The c_value is cloned from TEN runtime, so we have to destroy it.
    ten_value_destroy(c_value);
  }

  TEN_GO_TEN_ENV_IS_ALIVE_REGION_END(self);

ten_is_close:
  return cgo_error;
}

ten_go_error_t ten_go_ten_env_get_property_int64(uintptr_t bridge_addr,
                                                 const void *path, int path_len,
                                                 int64_t *value) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");
  TEN_ASSERT(value, "Should not happen.");

  ten_go_error_t cgo_error;
  ten_go_error_init_with_errno(&cgo_error, TEN_ERRNO_OK);

  TEN_GO_TEN_ENV_IS_ALIVE_REGION_BEGIN(
      self, { ten_go_error_set_errno(&cgo_error, TEN_ERRNO_TEN_IS_CLOSED); });

  ten_value_t *c_value = ten_go_ten_env_get_property_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (c_value != NULL) {
    ten_error_t err;
    ten_error_init(&err);

    *value = ten_value_get_int64(c_value, &err);

    ten_go_error_from_error(&cgo_error, &err);
    ten_error_deinit(&err);

    // The c_value is cloned from TEN runtime, so we have to destroy it.
    ten_value_destroy(c_value);
  }

  TEN_GO_TEN_ENV_IS_ALIVE_REGION_END(self);

ten_is_close:
  return cgo_error;
}

ten_go_error_t ten_go_ten_env_get_property_uint8(uintptr_t bridge_addr,
                                                 const void *path, int path_len,
                                                 uint8_t *value) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");
  TEN_ASSERT(value, "Should not happen.");

  ten_go_error_t cgo_error;
  ten_go_error_init_with_errno(&cgo_error, TEN_ERRNO_OK);

  TEN_GO_TEN_ENV_IS_ALIVE_REGION_BEGIN(
      self, { ten_go_error_set_errno(&cgo_error, TEN_ERRNO_TEN_IS_CLOSED); });

  ten_value_t *c_value = ten_go_ten_env_get_property_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (c_value != NULL) {
    ten_error_t err;
    ten_error_init(&err);

    *value = ten_value_get_uint8(c_value, &err);

    ten_go_error_from_error(&cgo_error, &err);
    ten_error_deinit(&err);

    // The c_value is cloned from TEN runtime, so we have to destroy it.
    ten_value_destroy(c_value);
  }

  TEN_GO_TEN_ENV_IS_ALIVE_REGION_END(self);

ten_is_close:
  return cgo_error;
}

ten_go_error_t ten_go_ten_env_get_property_uint16(uintptr_t bridge_addr,
                                                  const void *path,
                                                  int path_len,
                                                  uint16_t *value) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");
  TEN_ASSERT(value, "Should not happen.");

  ten_go_error_t cgo_error;
  ten_go_error_init_with_errno(&cgo_error, TEN_ERRNO_OK);

  TEN_GO_TEN_ENV_IS_ALIVE_REGION_BEGIN(
      self, { ten_go_error_set_errno(&cgo_error, TEN_ERRNO_TEN_IS_CLOSED); });

  ten_value_t *c_value = ten_go_ten_env_get_property_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (c_value != NULL) {
    ten_error_t err;
    ten_error_init(&err);

    *value = ten_value_get_uint16(c_value, &err);

    ten_go_error_from_error(&cgo_error, &err);
    ten_error_deinit(&err);

    // The c_value is cloned from TEN runtime, so we have to destroy it.
    ten_value_destroy(c_value);
  }

  TEN_GO_TEN_ENV_IS_ALIVE_REGION_END(self);

ten_is_close:
  return cgo_error;
}

ten_go_error_t ten_go_ten_env_get_property_uint32(uintptr_t bridge_addr,
                                                  const void *path,
                                                  int path_len,
                                                  uint32_t *value) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");
  TEN_ASSERT(value, "Should not happen.");

  ten_go_error_t cgo_error;
  ten_go_error_init_with_errno(&cgo_error, TEN_ERRNO_OK);

  TEN_GO_TEN_ENV_IS_ALIVE_REGION_BEGIN(
      self, { ten_go_error_set_errno(&cgo_error, TEN_ERRNO_TEN_IS_CLOSED); });

  ten_value_t *c_value = ten_go_ten_env_get_property_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (c_value != NULL) {
    ten_error_t err;
    ten_error_init(&err);

    *value = ten_value_get_uint32(c_value, &err);

    ten_go_error_from_error(&cgo_error, &err);
    ten_error_deinit(&err);

    // The c_value is cloned from TEN runtime, so we have to destroy it.
    ten_value_destroy(c_value);
  }

  TEN_GO_TEN_ENV_IS_ALIVE_REGION_END(self);

ten_is_close:
  return cgo_error;
}

ten_go_error_t ten_go_ten_env_get_property_uint64(uintptr_t bridge_addr,
                                                  const void *path,
                                                  int path_len,
                                                  uint64_t *value) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");
  TEN_ASSERT(value, "Should not happen.");

  ten_go_error_t cgo_error;
  ten_go_error_init_with_errno(&cgo_error, TEN_ERRNO_OK);

  TEN_GO_TEN_ENV_IS_ALIVE_REGION_BEGIN(
      self, { ten_go_error_set_errno(&cgo_error, TEN_ERRNO_TEN_IS_CLOSED); });

  ten_value_t *c_value = ten_go_ten_env_get_property_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (c_value != NULL) {
    ten_error_t err;
    ten_error_init(&err);

    *value = ten_value_get_uint64(c_value, &err);

    ten_go_error_from_error(&cgo_error, &err);
    ten_error_deinit(&err);

    // The c_value is cloned from TEN runtime, so we have to destroy it.
    ten_value_destroy(c_value);
  }

  TEN_GO_TEN_ENV_IS_ALIVE_REGION_END(self);

ten_is_close:
  return cgo_error;
}

ten_go_error_t ten_go_ten_env_get_property_float32(uintptr_t bridge_addr,
                                                   const void *path,
                                                   int path_len, float *value) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");
  TEN_ASSERT(value, "Should not happen.");

  ten_go_error_t cgo_error;
  ten_go_error_init_with_errno(&cgo_error, TEN_ERRNO_OK);

  TEN_GO_TEN_ENV_IS_ALIVE_REGION_BEGIN(
      self, { ten_go_error_set_errno(&cgo_error, TEN_ERRNO_TEN_IS_CLOSED); });

  ten_value_t *c_value = ten_go_ten_env_get_property_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (c_value != NULL) {
    ten_error_t err;
    ten_error_init(&err);

    *value = ten_value_get_float32(c_value, &err);

    ten_go_error_from_error(&cgo_error, &err);
    ten_error_deinit(&err);

    // The c_value is cloned from TEN runtime, so we have to destroy it.
    ten_value_destroy(c_value);
  }

  TEN_GO_TEN_ENV_IS_ALIVE_REGION_END(self);

ten_is_close:
  return cgo_error;
}

ten_go_error_t ten_go_ten_env_get_property_float64(uintptr_t bridge_addr,
                                                   const void *path,
                                                   int path_len,
                                                   double *value) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");
  TEN_ASSERT(value, "Should not happen.");

  ten_go_error_t cgo_error;
  ten_go_error_init_with_errno(&cgo_error, TEN_ERRNO_OK);

  TEN_GO_TEN_ENV_IS_ALIVE_REGION_BEGIN(
      self, { ten_go_error_set_errno(&cgo_error, TEN_ERRNO_TEN_IS_CLOSED); });

  ten_value_t *c_value = ten_go_ten_env_get_property_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (c_value != NULL) {
    ten_error_t err;
    ten_error_init(&err);

    *value = ten_value_get_float64(c_value, &err);

    ten_go_error_from_error(&cgo_error, &err);
    ten_error_deinit(&err);

    // The c_value is cloned from TEN runtime, so we have to destroy it.
    ten_value_destroy(c_value);
  }

  TEN_GO_TEN_ENV_IS_ALIVE_REGION_END(self);

ten_is_close:
  return cgo_error;
}

ten_go_error_t ten_go_ten_env_get_property_bool(uintptr_t bridge_addr,
                                                const void *path, int path_len,
                                                bool *value) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");
  TEN_ASSERT(value, "Should not happen.");

  ten_go_error_t cgo_error;
  ten_go_error_init_with_errno(&cgo_error, TEN_ERRNO_OK);

  TEN_GO_TEN_ENV_IS_ALIVE_REGION_BEGIN(
      self, { ten_go_error_set_errno(&cgo_error, TEN_ERRNO_TEN_IS_CLOSED); });

  ten_value_t *c_value = ten_go_ten_env_get_property_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (c_value != NULL) {
    ten_error_t err;
    ten_error_init(&err);

    *value = ten_value_get_bool(c_value, &err);

    ten_go_error_from_error(&cgo_error, &err);
    ten_error_deinit(&err);

    // The c_value is cloned from TEN runtime, so we have to destroy it.
    ten_value_destroy(c_value);
  }

  TEN_GO_TEN_ENV_IS_ALIVE_REGION_END(self);

ten_is_close:
  return cgo_error;
}

ten_go_error_t ten_go_ten_env_get_property_ptr(uintptr_t bridge_addr,
                                               const void *path, int path_len,
                                               ten_go_handle_t *value) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");
  TEN_ASSERT(value, "Should not happen.");

  ten_go_error_t cgo_error;
  ten_go_error_init_with_errno(&cgo_error, TEN_ERRNO_OK);

  TEN_GO_TEN_ENV_IS_ALIVE_REGION_BEGIN(
      self, { ten_go_error_set_errno(&cgo_error, TEN_ERRNO_TEN_IS_CLOSED); });

  ten_value_t *c_value = ten_go_ten_env_get_property_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (c_value != NULL) {
    ten_go_ten_value_get_ptr(c_value, value, &cgo_error);

    // The c_value is cloned from TEN runtime, so we have to destroy it.
    ten_value_destroy(c_value);
  }

  TEN_GO_TEN_ENV_IS_ALIVE_REGION_END(self);

ten_is_close:
  return cgo_error;
}

ten_go_error_t ten_go_ten_env_get_property_json_and_size(
    uintptr_t bridge_addr, const void *path, int path_len,
    uintptr_t *json_str_len, const char **json_str) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");
  TEN_ASSERT(json_str && json_str_len > 0, "Should not happen.");

  ten_go_error_t cgo_error;
  ten_go_error_init_with_errno(&cgo_error, TEN_ERRNO_OK);

  TEN_GO_TEN_ENV_IS_ALIVE_REGION_BEGIN(
      self, { ten_go_error_set_errno(&cgo_error, TEN_ERRNO_TEN_IS_CLOSED); });

  ten_value_t *value = ten_go_ten_env_get_property_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (value != NULL) {
    ten_go_ten_value_to_json(value, json_str_len, json_str, &cgo_error);

    // The c_value is cloned from TEN runtime, so we have to destroy it.
    ten_value_destroy(value);
  }

  TEN_GO_TEN_ENV_IS_ALIVE_REGION_END(self);

ten_is_close:
  return cgo_error;
}
