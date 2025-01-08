//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_runtime/binding/go/interface/ten/msg.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "include_internal/axis_runtime/binding/go/internal/common.h"
#include "include_internal/axis_runtime/binding/go/internal/json.h"
#include "include_internal/axis_runtime/binding/go/msg/msg.h"
#include "include_internal/axis_runtime/binding/go/value/value.h"
#include "include_internal/axis_runtime/msg/field/properties.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "axis_runtime/binding/go/interface/ten/common.h"
#include "axis_runtime/binding/go/interface/ten/value.h"
#include "axis_runtime/common/errno.h"
#include "axis_runtime/msg/msg.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/log/log.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/memory.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_get.h"

bool axis_go_msg_check_integrity(axis_go_msg_t *self) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) != axis_GO_MSG_SIGNATURE) {
    return false;
  }

  return true;
}

axis_go_msg_t *axis_go_msg_reinterpret(uintptr_t msg) {
  // All msgs are created in the C world, and passed to the GO world. So the msg
  // passed from the GO world must be always valid.
  axis_ASSERT(msg > 0, "Should not happen.");

  // NOLINTNEXTLINE(performance-no-int-to-ptr)
  axis_go_msg_t *self = (axis_go_msg_t *)msg;
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");

  return self;
}

axis_go_handle_t axis_go_msg_go_handle(axis_go_msg_t *self) {
  axis_ASSERT(self, "Should not happen.");

  return self->go_msg;
}

axis_shared_ptr_t *axis_go_msg_c_msg(axis_go_msg_t *self) {
  axis_ASSERT(self, "Should not happen.");

  return self->c_msg;
}

axis_shared_ptr_t *axis_go_msg_move_c_msg(axis_go_msg_t *self) {
  axis_ASSERT(self, "Should not happen.");

  axis_shared_ptr_t *c_msg = self->c_msg;
  self->c_msg = NULL;

  return c_msg;
}

axis_go_msg_t *axis_go_msg_create(axis_shared_ptr_t *c_msg) {
  axis_go_msg_t *msg_bridge = (axis_go_msg_t *)axis_MALLOC(sizeof(axis_go_msg_t));
  axis_ASSERT(msg_bridge, "Failed to allocate memory.");

  axis_signature_set(&msg_bridge->signature, axis_GO_MSG_SIGNATURE);

  msg_bridge->c_msg = axis_shared_ptr_clone(c_msg);

  return msg_bridge;
}

void axis_go_msg_set_go_handle(axis_go_msg_t *self, axis_go_handle_t go_handle) {
  axis_ASSERT(axis_go_msg_check_integrity(self), "Should not happen.");

  self->go_msg = go_handle;
}

static axis_value_t *axis_go_msg_property_get_and_check_if_exists(
    axis_go_msg_t *self, const void *path, axis_go_handle_t path_len,
    axis_go_error_t *status) {
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");
  axis_ASSERT(status, "Should not happen.");

  axis_go_error_init_with_errno(status, axis_ERRNO_OK);

  axis_string_t prop_path;
  axis_string_init_formatted(&prop_path, "%.*s", path_len, path);

  axis_value_t *value = axis_msg_peek_property(
      self->c_msg, axis_string_get_raw_str(&prop_path), NULL);

  axis_string_deinit(&prop_path);

  if (value == NULL) {
    axis_go_error_set_errno(status, axis_ERRNO_GENERIC);
  }

  return value;
}

axis_go_error_t axis_go_msg_property_get_type_and_size(uintptr_t bridge_addr,
                                                     const void *path,
                                                     int path_len,
                                                     uint8_t *type,
                                                     axis_go_handle_t *size) {
  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");
  axis_ASSERT(type && size, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_value_t *value = axis_go_msg_property_get_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (value == NULL) {
    return cgo_error;
  }

  axis_go_axis_value_get_type_and_size(value, type, size);

  return cgo_error;
}

axis_go_error_t axis_go_msg_property_get_int8(uintptr_t bridge_addr,
                                            const void *path, int path_len,
                                            int8_t *value) {
  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");
  axis_ASSERT(value, "Should not happen");

  axis_go_error_t cgo_error;
  axis_value_t *c_value = axis_go_msg_property_get_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (c_value == NULL) {
    return cgo_error;
  }

  axis_error_t err;
  axis_error_init(&err);

  *value = axis_value_get_int8(c_value, &err);

  axis_go_error_from_error(&cgo_error, &err);
  axis_error_deinit(&err);

  return cgo_error;
}

axis_go_error_t axis_go_msg_property_get_int16(uintptr_t bridge_addr,
                                             const void *path, int path_len,
                                             int16_t *value) {
  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");
  axis_ASSERT(value, "Should not happen");

  axis_go_error_t cgo_error;
  axis_value_t *c_value = axis_go_msg_property_get_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (c_value == NULL) {
    return cgo_error;
  }

  axis_error_t err;
  axis_error_init(&err);

  *value = axis_value_get_int16(c_value, &err);

  axis_go_error_from_error(&cgo_error, &err);
  axis_error_deinit(&err);

  return cgo_error;
}

axis_go_error_t axis_go_msg_property_get_int32(uintptr_t bridge_addr,
                                             const void *path, int path_len,
                                             int32_t *value) {
  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");
  axis_ASSERT(value, "Should not happen");

  axis_go_error_t cgo_error;
  axis_value_t *c_value = axis_go_msg_property_get_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (c_value == NULL) {
    return cgo_error;
  }

  axis_error_t err;
  axis_error_init(&err);

  *value = axis_value_get_int32(c_value, &err);

  axis_go_error_from_error(&cgo_error, &err);
  axis_error_deinit(&err);

  return cgo_error;
}

axis_go_error_t axis_go_msg_property_get_int64(uintptr_t bridge_addr,
                                             const void *path, int path_len,
                                             int64_t *value) {
  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");
  axis_ASSERT(value, "Should not happen");

  axis_go_error_t cgo_error;
  axis_value_t *c_value = axis_go_msg_property_get_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (c_value == NULL) {
    return cgo_error;
  }

  axis_error_t err;
  axis_error_init(&err);

  *value = axis_value_get_int64(c_value, &err);

  axis_go_error_from_error(&cgo_error, &err);
  axis_error_deinit(&err);

  return cgo_error;
}

axis_go_error_t axis_go_msg_property_get_uint8(uintptr_t bridge_addr,
                                             const void *path, int path_len,
                                             uint8_t *value) {
  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");
  axis_ASSERT(value, "Should not happen");

  axis_go_error_t cgo_error;
  axis_value_t *c_value = axis_go_msg_property_get_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (c_value == NULL) {
    return cgo_error;
  }

  axis_error_t err;
  axis_error_init(&err);

  *value = axis_value_get_uint8(c_value, &err);

  axis_go_error_from_error(&cgo_error, &err);
  axis_error_deinit(&err);

  return cgo_error;
}

axis_go_error_t axis_go_msg_property_get_uint16(uintptr_t bridge_addr,
                                              const void *path, int path_len,
                                              uint16_t *value) {
  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");
  axis_ASSERT(value, "Should not happen");

  axis_go_error_t cgo_error;
  axis_value_t *c_value = axis_go_msg_property_get_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (c_value == NULL) {
    return cgo_error;
  }

  axis_error_t err;
  axis_error_init(&err);

  *value = axis_value_get_uint16(c_value, &err);

  axis_go_error_from_error(&cgo_error, &err);
  axis_error_deinit(&err);

  return cgo_error;
}

axis_go_error_t axis_go_msg_property_get_uint32(uintptr_t bridge_addr,
                                              const void *path, int path_len,
                                              uint32_t *value) {
  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");
  axis_ASSERT(value, "Should not happen");

  axis_go_error_t cgo_error;
  axis_value_t *c_value = axis_go_msg_property_get_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (c_value == NULL) {
    return cgo_error;
  }

  axis_error_t err;
  axis_error_init(&err);

  *value = axis_value_get_uint32(c_value, &err);

  axis_go_error_from_error(&cgo_error, &err);
  axis_error_deinit(&err);

  return cgo_error;
}

axis_go_error_t axis_go_msg_property_get_uint64(uintptr_t bridge_addr,
                                              const void *path, int path_len,
                                              uint64_t *value) {
  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");
  axis_ASSERT(value, "Should not happen");

  axis_go_error_t cgo_error;
  axis_value_t *c_value = axis_go_msg_property_get_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (c_value == NULL) {
    return cgo_error;
  }

  axis_error_t err;
  axis_error_init(&err);

  *value = axis_value_get_uint64(c_value, &err);

  axis_go_error_from_error(&cgo_error, &err);
  axis_error_deinit(&err);

  return cgo_error;
}

axis_go_error_t axis_go_msg_property_get_float32(uintptr_t bridge_addr,
                                               const void *path, int path_len,
                                               float *value) {
  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");
  axis_ASSERT(value, "Should not happen");

  axis_go_error_t cgo_error;
  axis_value_t *c_value = axis_go_msg_property_get_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (c_value == NULL) {
    return cgo_error;
  }

  axis_error_t err;
  axis_error_init(&err);

  *value = axis_value_get_float32(c_value, &err);

  axis_go_error_from_error(&cgo_error, &err);
  axis_error_deinit(&err);

  return cgo_error;
}

axis_go_error_t axis_go_msg_property_get_float64(uintptr_t bridge_addr,
                                               const void *path, int path_len,
                                               double *value) {
  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");
  axis_ASSERT(value, "Should not happen");

  axis_go_error_t cgo_error;
  axis_value_t *c_value = axis_go_msg_property_get_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (c_value == NULL) {
    return cgo_error;
  }

  axis_error_t err;
  axis_error_init(&err);

  *value = axis_value_get_float64(c_value, &err);

  axis_go_error_from_error(&cgo_error, &err);
  axis_error_deinit(&err);

  return cgo_error;
}

axis_go_error_t axis_go_msg_property_get_bool(uintptr_t bridge_addr,
                                            const void *path, int path_len,
                                            bool *value) {
  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");
  axis_ASSERT(value, "Should not happen");

  axis_go_error_t cgo_error;
  axis_value_t *c_value = axis_go_msg_property_get_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (c_value == NULL) {
    return cgo_error;
  }

  axis_error_t err;
  axis_error_init(&err);

  *value = axis_value_get_bool(c_value, &err);

  axis_go_error_from_error(&cgo_error, &err);
  axis_error_deinit(&err);

  return cgo_error;
}

axis_go_error_t axis_go_msg_property_get_string(uintptr_t bridge_addr,
                                              const void *path, int path_len,
                                              void *value) {
  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");
  axis_ASSERT(value, "Should not happen");

  axis_go_error_t cgo_error;
  axis_value_t *c_value = axis_go_msg_property_get_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (c_value == NULL) {
    return cgo_error;
  }

  axis_go_axis_value_get_string(c_value, value, &cgo_error);
  return cgo_error;
}

axis_go_error_t axis_go_msg_property_get_buf(uintptr_t bridge_addr,
                                           const void *path, int path_len,
                                           void *value) {
  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");
  axis_ASSERT(value, "Should not happen");

  axis_go_error_t cgo_error;
  axis_value_t *c_value = axis_go_msg_property_get_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (c_value == NULL) {
    return cgo_error;
  }

  axis_go_axis_value_get_buf(c_value, value, &cgo_error);
  return cgo_error;
}

axis_go_error_t axis_go_msg_property_get_ptr(uintptr_t bridge_addr,
                                           const void *path, int path_len,
                                           axis_go_handle_t *value) {
  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");
  axis_ASSERT(value, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_value_t *c_value = axis_go_msg_property_get_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (c_value == NULL) {
    return cgo_error;
  }

  axis_go_axis_value_get_ptr(c_value, value, &cgo_error);
  return cgo_error;
}

static void axis_go_msg_set_property(axis_go_msg_t *self, const void *path,
                                    int path_len, axis_value_t *value) {
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(value && axis_value_check_integrity(value), "Should not happen.");

  axis_string_t key;
  axis_string_init_formatted(&key, "%.*s", path_len, path);

  axis_msg_set_property(self->c_msg, axis_string_get_raw_str(&key), value, NULL);

  axis_string_deinit(&key);
}

axis_go_error_t axis_go_msg_property_set_bool(uintptr_t bridge_addr,
                                            const void *path, int path_len,
                                            bool value) {
  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_value_t *c_value = axis_value_create_bool(value);
  axis_go_msg_set_property(self, path, path_len, c_value);

  return cgo_error;
}

axis_go_error_t axis_go_msg_property_set_int8(uintptr_t bridge_addr,
                                            const void *path, int path_len,
                                            int8_t value) {
  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_value_t *c_value = axis_value_create_int8(value);
  axis_go_msg_set_property(self, path, path_len, c_value);

  return cgo_error;
}

axis_go_error_t axis_go_msg_property_set_int16(uintptr_t bridge_addr,
                                             const void *path, int path_len,
                                             int16_t value) {
  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_value_t *c_value = axis_value_create_int16(value);
  axis_go_msg_set_property(self, path, path_len, c_value);

  return cgo_error;
}

axis_go_error_t axis_go_msg_property_set_int32(uintptr_t bridge_addr,
                                             const void *path, int path_len,
                                             int32_t value) {
  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_value_t *c_value = axis_value_create_int32(value);
  axis_go_msg_set_property(self, path, path_len, c_value);

  return cgo_error;
}

axis_go_error_t axis_go_msg_property_set_int64(uintptr_t bridge_addr,
                                             const void *path, int path_len,
                                             int64_t value) {
  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_value_t *c_value = axis_value_create_int64(value);
  axis_go_msg_set_property(self, path, path_len, c_value);

  return cgo_error;
}

axis_go_error_t axis_go_msg_property_set_uint8(uintptr_t bridge_addr,
                                             const void *path, int path_len,
                                             uint8_t value) {
  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_value_t *c_value = axis_value_create_uint8(value);
  axis_go_msg_set_property(self, path, path_len, c_value);

  return cgo_error;
}

axis_go_error_t axis_go_msg_property_set_uint16(uintptr_t bridge_addr,
                                              const void *path, int path_len,
                                              uint16_t value) {
  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_value_t *c_value = axis_value_create_uint16(value);
  axis_go_msg_set_property(self, path, path_len, c_value);

  return cgo_error;
}

axis_go_error_t axis_go_msg_property_set_uint32(uintptr_t bridge_addr,
                                              const void *path, int path_len,
                                              uint32_t value) {
  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_value_t *c_value = axis_value_create_uint32(value);
  axis_go_msg_set_property(self, path, path_len, c_value);

  return cgo_error;
}

axis_go_error_t axis_go_msg_property_set_uint64(uintptr_t bridge_addr,
                                              const void *path, int path_len,
                                              uint64_t value) {
  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_value_t *c_value = axis_value_create_uint64(value);
  axis_go_msg_set_property(self, path, path_len, c_value);

  return cgo_error;
}

axis_go_error_t axis_go_msg_property_set_float32(uintptr_t bridge_addr,
                                               const void *path, int path_len,
                                               float value) {
  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_value_t *c_value = axis_value_create_float32(value);
  axis_go_msg_set_property(self, path, path_len, c_value);

  return cgo_error;
}

axis_go_error_t axis_go_msg_property_set_float64(uintptr_t bridge_addr,
                                               const void *path, int path_len,
                                               double value) {
  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_value_t *c_value = axis_value_create_float64(value);
  axis_go_msg_set_property(self, path, path_len, c_value);

  return cgo_error;
}

axis_go_error_t axis_go_msg_property_set_string(uintptr_t bridge_addr,
                                              const void *path, int path_len,
                                              const void *value,
                                              int value_len) {
  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  const char *str_value = "";

  // According to the document of `unsafe.StringData()`, the underlying data
  // (i.e., value here) of an empty GO string is unspecified. So it's unsafe to
  // access. We should handle this case explicitly.
  if (value_len > 0) {
    str_value = (const char *)value;
  }

  axis_value_t *c_value =
      axis_value_create_string_with_size(str_value, value_len);
  axis_go_msg_set_property(self, path, path_len, c_value);

  return cgo_error;
}

axis_go_error_t axis_go_msg_property_set_buf(uintptr_t bridge_addr,
                                           const void *path, int path_len,
                                           void *value, int value_len) {
  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Invalid argument.");
  axis_ASSERT(path && path_len > 0, "Invalid argument.");

  // The size must be > 0 when calling axis_MALLOC().
  axis_ASSERT(value && value_len > 0, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_value_t *c_value = axis_go_axis_value_create_buf(value, value_len);
  axis_go_msg_set_property(self, path, path_len, c_value);

  return cgo_error;
}

axis_go_error_t axis_go_msg_property_set_ptr(uintptr_t bridge_addr,
                                           const void *path, int path_len,
                                           axis_go_handle_t value) {
  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_value_t *c_value = axis_go_axis_value_create_ptr(value);
  axis_go_msg_set_property(self, path, path_len, c_value);

  return cgo_error;
}

axis_go_error_t axis_go_msg_property_get_json_and_size(uintptr_t bridge_addr,
                                                     const void *path,
                                                     int path_len,
                                                     uintptr_t *json_str_len,
                                                     const char **json_str) {
  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");
  axis_ASSERT(json_str_len && json_str, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_value_t *value = axis_go_msg_property_get_and_check_if_exists(
      self, path, path_len, &cgo_error);
  if (value == NULL) {
    return cgo_error;
  }

  axis_go_axis_value_to_json(value, json_str_len, json_str, &cgo_error);

  return cgo_error;
}

axis_go_error_t axis_go_msg_property_set_json_bytes(uintptr_t bridge_addr,
                                                  const void *path,
                                                  int path_len,
                                                  const void *json_str,
                                                  int json_str_len) {
  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(path && path_len > 0, "Should not happen.");
  axis_ASSERT(json_str && json_str_len > 0, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_json_t *json = axis_go_json_loads(json_str, json_str_len, &cgo_error);
  if (json == NULL) {
    return cgo_error;
  }

  axis_value_t *value = axis_value_from_json(json);
  axis_json_destroy(json);

  axis_go_msg_set_property(self, path, path_len, value);
  return cgo_error;
}

void axis_go_msg_finalize(uintptr_t bridge_addr) {
  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_msg_check_integrity(self), "Should not happen.");

  if (self->c_msg) {
    axis_shared_ptr_destroy(self->c_msg);
    self->c_msg = NULL;
  }

  axis_FREE(self);
}

axis_go_error_t axis_go_msg_get_name(uintptr_t bridge_addr, const char **name) {
  axis_ASSERT(bridge_addr && name, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_msg_t *self = axis_go_msg_reinterpret(bridge_addr);
  const char *msg_name = axis_msg_get_name(axis_go_msg_c_msg(self));
  axis_ASSERT(msg_name, "Should not happen.");

  *name = msg_name;
  return cgo_error;
}
