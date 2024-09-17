//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_runtime/binding/go/interface/ten/msg.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "include_internal/ten_runtime/binding/go/internal/common.h"
#include "include_internal/ten_runtime/binding/go/internal/json.h"
#include "include_internal/ten_runtime/binding/go/msg/msg.h"
#include "include_internal/ten_runtime/binding/go/value/value.h"
#include "include_internal/ten_runtime/msg/field/properties.h"
#include "ten_utils/macro/check.h"
#include "ten_runtime/binding/go/interface/ten/common.h"
#include "ten_runtime/binding/go/interface/ten/value.h"
#include "ten_runtime/common/errno.h"
#include "ten_runtime/msg/msg.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/lib/signature.h"
#include "ten_utils/lib/smart_ptr.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/macro/memory.h"
#include "ten_utils/value/value.h"
#include "ten_utils/value/value_get.h"

typedef struct ten_go_msg_t {
  ten_signature_t signature;

  ten_shared_ptr_t *c_msg;
  ten_go_handle_t go_msg;
} ten_go_msg_t;

bool ten_go_msg_check_integrity(ten_go_msg_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  if (ten_signature_get(&self->signature) != TEN_GO_MSG_SIGNATURE) {
    return false;
  }

  return true;
}

ten_go_msg_t *ten_go_msg_reinterpret(uintptr_t msg) {
  // All msgs are created in the C world, and passed to the GO world. So the msg
  // passed from the GO world must be always valid.
  TEN_ASSERT(msg > 0, "Should not happen.");

  // NOLINTNEXTLINE(performance-no-int-to-ptr)
  ten_go_msg_t *self = (ten_go_msg_t *)msg;
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");

  return self;
}

ten_go_handle_t ten_go_msg_go_handle(ten_go_msg_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  return self->go_msg;
}

ten_shared_ptr_t *ten_go_msg_c_msg(ten_go_msg_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  return self->c_msg;
}

ten_shared_ptr_t *ten_go_msg_move_c_msg(ten_go_msg_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  ten_shared_ptr_t *c_msg = self->c_msg;
  self->c_msg = NULL;

  return c_msg;
}

ten_go_msg_t *ten_go_msg_create(ten_shared_ptr_t *c_msg) {
  ten_go_msg_t *msg_bridge = (ten_go_msg_t *)TEN_MALLOC(sizeof(ten_go_msg_t));
  TEN_ASSERT(msg_bridge, "Failed to allocate memory.");

  ten_signature_set(&msg_bridge->signature, TEN_GO_MSG_SIGNATURE);

  msg_bridge->c_msg = ten_shared_ptr_clone(c_msg);

  return msg_bridge;
}

void ten_go_msg_set_go_handle(ten_go_msg_t *self, ten_go_handle_t go_handle) {
  TEN_ASSERT(ten_go_msg_check_integrity(self), "Should not happen.");

  self->go_msg = go_handle;
}

int ten_go_msg_get_type(uintptr_t bridge_addr) {
  ten_go_msg_t *msg_bridge = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(msg_bridge && ten_go_msg_check_integrity(msg_bridge),
             "Should not happen.");

  TEN_MSG_TYPE type = ten_msg_get_type(msg_bridge->c_msg);
  TEN_ASSERT(type != TEN_MSG_TYPE_INVALID, "Should not happen.");

  return type;
}

const char *ten_go_msg_to_json(uintptr_t bridge_addr) {
  ten_go_msg_t *msg_bridge = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(msg_bridge && ten_go_msg_check_integrity(msg_bridge),
             "Should not happen.");

  ten_json_t *json = ten_msg_to_json(msg_bridge->c_msg, NULL);
  if (!json) {
    return NULL;
  }

  TEN_ASSERT(json, "Failed to get json from TEN C message.");

  bool must_free = false;
  const char *json_str = ten_json_to_string(json, NULL, &must_free);
  TEN_ASSERT(json_str, "Failed to get JSON string from JSON.");

  ten_json_destroy(json);
  return json_str;
}

static ten_value_t *ten_go_msg_property_get_and_check_if_exists(
    ten_go_msg_t *self, const void *path, ten_go_handle_t path_len,
    ten_go_status_t *status) {
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");
  TEN_ASSERT(status, "Should not happen.");

  ten_go_status_init_with_errno(status, TEN_ERRNO_OK);

  ten_string_t prop_path;
  ten_string_init_formatted(&prop_path, "%.*s", path_len, path);

  ten_value_t *value = ten_msg_peek_property(
      self->c_msg, ten_string_get_raw_str(&prop_path), NULL);

  ten_string_deinit(&prop_path);

  if (value == NULL) {
    ten_go_status_set_errno(status, TEN_ERRNO_GENERIC);
  }

  return value;
}

ten_go_status_t ten_go_msg_property_get_type_and_size(uintptr_t bridge_addr,
                                                      const void *path,
                                                      int path_len,
                                                      uint8_t *type,
                                                      ten_go_handle_t *size) {
  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");
  TEN_ASSERT(type && size, "Should not happen.");

  ten_go_status_t status;
  ten_value_t *value = ten_go_msg_property_get_and_check_if_exists(
      self, path, path_len, &status);
  if (value == NULL) {
    return status;
  }

  ten_go_ten_value_get_type_and_size(value, type, size);

  return status;
}

ten_go_status_t ten_go_msg_property_get_int8(uintptr_t bridge_addr,
                                             const void *path, int path_len,
                                             int8_t *value) {
  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");
  TEN_ASSERT(value, "Should not happen");

  ten_go_status_t status;
  ten_value_t *c_value = ten_go_msg_property_get_and_check_if_exists(
      self, path, path_len, &status);
  if (c_value == NULL) {
    return status;
  }

  ten_error_t err;
  ten_error_init(&err);

  *value = ten_value_get_int8(c_value, &err);

  ten_go_status_from_error(&status, &err);
  ten_error_deinit(&err);

  return status;
}

ten_go_status_t ten_go_msg_property_get_int16(uintptr_t bridge_addr,
                                              const void *path, int path_len,
                                              int16_t *value) {
  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");
  TEN_ASSERT(value, "Should not happen");

  ten_go_status_t status;
  ten_value_t *c_value = ten_go_msg_property_get_and_check_if_exists(
      self, path, path_len, &status);
  if (c_value == NULL) {
    return status;
  }

  ten_error_t err;
  ten_error_init(&err);

  *value = ten_value_get_int16(c_value, &err);

  ten_go_status_from_error(&status, &err);
  ten_error_deinit(&err);

  return status;
}

ten_go_status_t ten_go_msg_property_get_int32(uintptr_t bridge_addr,
                                              const void *path, int path_len,
                                              int32_t *value) {
  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");
  TEN_ASSERT(value, "Should not happen");

  ten_go_status_t status;
  ten_value_t *c_value = ten_go_msg_property_get_and_check_if_exists(
      self, path, path_len, &status);
  if (c_value == NULL) {
    return status;
  }

  ten_error_t err;
  ten_error_init(&err);

  *value = ten_value_get_int32(c_value, &err);

  ten_go_status_from_error(&status, &err);
  ten_error_deinit(&err);

  return status;
}

ten_go_status_t ten_go_msg_property_get_int64(uintptr_t bridge_addr,
                                              const void *path, int path_len,
                                              int64_t *value) {
  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");
  TEN_ASSERT(value, "Should not happen");

  ten_go_status_t status;
  ten_value_t *c_value = ten_go_msg_property_get_and_check_if_exists(
      self, path, path_len, &status);
  if (c_value == NULL) {
    return status;
  }

  ten_error_t err;
  ten_error_init(&err);

  *value = ten_value_get_int64(c_value, &err);

  ten_go_status_from_error(&status, &err);
  ten_error_deinit(&err);

  return status;
}

ten_go_status_t ten_go_msg_property_get_uint8(uintptr_t bridge_addr,
                                              const void *path, int path_len,
                                              uint8_t *value) {
  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");
  TEN_ASSERT(value, "Should not happen");

  ten_go_status_t status;
  ten_value_t *c_value = ten_go_msg_property_get_and_check_if_exists(
      self, path, path_len, &status);
  if (c_value == NULL) {
    return status;
  }

  ten_error_t err;
  ten_error_init(&err);

  *value = ten_value_get_uint8(c_value, &err);

  ten_go_status_from_error(&status, &err);
  ten_error_deinit(&err);

  return status;
}

ten_go_status_t ten_go_msg_property_get_uint16(uintptr_t bridge_addr,
                                               const void *path, int path_len,
                                               uint16_t *value) {
  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");
  TEN_ASSERT(value, "Should not happen");

  ten_go_status_t status;
  ten_value_t *c_value = ten_go_msg_property_get_and_check_if_exists(
      self, path, path_len, &status);
  if (c_value == NULL) {
    return status;
  }

  ten_error_t err;
  ten_error_init(&err);

  *value = ten_value_get_uint16(c_value, &err);

  ten_go_status_from_error(&status, &err);
  ten_error_deinit(&err);

  return status;
}

ten_go_status_t ten_go_msg_property_get_uint32(uintptr_t bridge_addr,
                                               const void *path, int path_len,
                                               uint32_t *value) {
  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");
  TEN_ASSERT(value, "Should not happen");

  ten_go_status_t status;
  ten_value_t *c_value = ten_go_msg_property_get_and_check_if_exists(
      self, path, path_len, &status);
  if (c_value == NULL) {
    return status;
  }

  ten_error_t err;
  ten_error_init(&err);

  *value = ten_value_get_uint32(c_value, &err);

  ten_go_status_from_error(&status, &err);
  ten_error_deinit(&err);

  return status;
}

ten_go_status_t ten_go_msg_property_get_uint64(uintptr_t bridge_addr,
                                               const void *path, int path_len,
                                               uint64_t *value) {
  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");
  TEN_ASSERT(value, "Should not happen");

  ten_go_status_t status;
  ten_value_t *c_value = ten_go_msg_property_get_and_check_if_exists(
      self, path, path_len, &status);
  if (c_value == NULL) {
    return status;
  }

  ten_error_t err;
  ten_error_init(&err);

  *value = ten_value_get_uint64(c_value, &err);

  ten_go_status_from_error(&status, &err);
  ten_error_deinit(&err);

  return status;
}

ten_go_status_t ten_go_msg_property_get_float32(uintptr_t bridge_addr,
                                                const void *path, int path_len,
                                                float *value) {
  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");
  TEN_ASSERT(value, "Should not happen");

  ten_go_status_t status;
  ten_value_t *c_value = ten_go_msg_property_get_and_check_if_exists(
      self, path, path_len, &status);
  if (c_value == NULL) {
    return status;
  }

  ten_error_t err;
  ten_error_init(&err);

  *value = ten_value_get_float32(c_value, &err);

  ten_go_status_from_error(&status, &err);
  ten_error_deinit(&err);

  return status;
}

ten_go_status_t ten_go_msg_property_get_float64(uintptr_t bridge_addr,
                                                const void *path, int path_len,
                                                double *value) {
  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");
  TEN_ASSERT(value, "Should not happen");

  ten_go_status_t status;
  ten_value_t *c_value = ten_go_msg_property_get_and_check_if_exists(
      self, path, path_len, &status);
  if (c_value == NULL) {
    return status;
  }

  ten_error_t err;
  ten_error_init(&err);

  *value = ten_value_get_float64(c_value, &err);

  ten_go_status_from_error(&status, &err);
  ten_error_deinit(&err);

  return status;
}

ten_go_status_t ten_go_msg_property_get_bool(uintptr_t bridge_addr,
                                             const void *path, int path_len,
                                             bool *value) {
  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");
  TEN_ASSERT(value, "Should not happen");

  ten_go_status_t status;
  ten_value_t *c_value = ten_go_msg_property_get_and_check_if_exists(
      self, path, path_len, &status);
  if (c_value == NULL) {
    return status;
  }

  ten_error_t err;
  ten_error_init(&err);

  *value = ten_value_get_bool(c_value, &err);

  ten_go_status_from_error(&status, &err);
  ten_error_deinit(&err);

  return status;
}

ten_go_status_t ten_go_msg_property_get_string(uintptr_t bridge_addr,
                                               const void *path, int path_len,
                                               void *value) {
  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");
  TEN_ASSERT(value, "Should not happen");

  ten_go_status_t status;
  ten_value_t *c_value = ten_go_msg_property_get_and_check_if_exists(
      self, path, path_len, &status);
  if (c_value == NULL) {
    return status;
  }

  ten_go_ten_value_get_string(c_value, value, &status);
  return status;
}

ten_go_status_t ten_go_msg_property_get_buf(uintptr_t bridge_addr,
                                            const void *path, int path_len,
                                            void *value) {
  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");
  TEN_ASSERT(value, "Should not happen");

  ten_go_status_t status;
  ten_value_t *c_value = ten_go_msg_property_get_and_check_if_exists(
      self, path, path_len, &status);
  if (c_value == NULL) {
    return status;
  }

  ten_go_ten_value_get_buf(c_value, value, &status);
  return status;
}

ten_go_status_t ten_go_msg_property_get_ptr(uintptr_t bridge_addr,
                                            const void *path, int path_len,
                                            ten_go_handle_t *value) {
  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");
  TEN_ASSERT(value, "Should not happen.");

  ten_go_status_t status;
  ten_value_t *c_value = ten_go_msg_property_get_and_check_if_exists(
      self, path, path_len, &status);
  if (c_value == NULL) {
    return status;
  }

  ten_go_ten_value_get_ptr(c_value, value, &status);
  return status;
}

static void ten_go_msg_set_property(ten_go_msg_t *self, const void *path,
                                    int path_len, ten_value_t *value) {
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(value && ten_value_check_integrity(value), "Should not happen.");

  ten_string_t key;
  ten_string_init_formatted(&key, "%.*s", path_len, path);

  ten_msg_set_property(self->c_msg, ten_string_get_raw_str(&key), value, NULL);

  ten_string_deinit(&key);
}

ten_go_status_t ten_go_msg_property_set_bool(uintptr_t bridge_addr,
                                             const void *path, int path_len,
                                             bool value) {
  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_value_t *c_value = ten_value_create_bool(value);
  ten_go_msg_set_property(self, path, path_len, c_value);

  return status;
}

ten_go_status_t ten_go_msg_property_set_int8(uintptr_t bridge_addr,
                                             const void *path, int path_len,
                                             int8_t value) {
  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_value_t *c_value = ten_value_create_int8(value);
  ten_go_msg_set_property(self, path, path_len, c_value);

  return status;
}

ten_go_status_t ten_go_msg_property_set_int16(uintptr_t bridge_addr,
                                              const void *path, int path_len,
                                              int16_t value) {
  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_value_t *c_value = ten_value_create_int16(value);
  ten_go_msg_set_property(self, path, path_len, c_value);

  return status;
}

ten_go_status_t ten_go_msg_property_set_int32(uintptr_t bridge_addr,
                                              const void *path, int path_len,
                                              int32_t value) {
  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_value_t *c_value = ten_value_create_int32(value);
  ten_go_msg_set_property(self, path, path_len, c_value);

  return status;
}

ten_go_status_t ten_go_msg_property_set_int64(uintptr_t bridge_addr,
                                              const void *path, int path_len,
                                              int64_t value) {
  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_value_t *c_value = ten_value_create_int64(value);
  ten_go_msg_set_property(self, path, path_len, c_value);

  return status;
}

ten_go_status_t ten_go_msg_property_set_uint8(uintptr_t bridge_addr,
                                              const void *path, int path_len,
                                              uint8_t value) {
  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_value_t *c_value = ten_value_create_uint8(value);
  ten_go_msg_set_property(self, path, path_len, c_value);

  return status;
}

ten_go_status_t ten_go_msg_property_set_uint16(uintptr_t bridge_addr,
                                               const void *path, int path_len,
                                               uint16_t value) {
  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_value_t *c_value = ten_value_create_uint16(value);
  ten_go_msg_set_property(self, path, path_len, c_value);

  return status;
}

ten_go_status_t ten_go_msg_property_set_uint32(uintptr_t bridge_addr,
                                               const void *path, int path_len,
                                               uint32_t value) {
  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_value_t *c_value = ten_value_create_uint32(value);
  ten_go_msg_set_property(self, path, path_len, c_value);

  return status;
}

ten_go_status_t ten_go_msg_property_set_uint64(uintptr_t bridge_addr,
                                               const void *path, int path_len,
                                               uint64_t value) {
  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_value_t *c_value = ten_value_create_uint64(value);
  ten_go_msg_set_property(self, path, path_len, c_value);

  return status;
}

ten_go_status_t ten_go_msg_property_set_float32(uintptr_t bridge_addr,
                                                const void *path, int path_len,
                                                float value) {
  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_value_t *c_value = ten_value_create_float32(value);
  ten_go_msg_set_property(self, path, path_len, c_value);

  return status;
}

ten_go_status_t ten_go_msg_property_set_float64(uintptr_t bridge_addr,
                                                const void *path, int path_len,
                                                double value) {
  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_value_t *c_value = ten_value_create_float64(value);
  ten_go_msg_set_property(self, path, path_len, c_value);

  return status;
}

ten_go_status_t ten_go_msg_property_set_string(uintptr_t bridge_addr,
                                               const void *path, int path_len,
                                               const void *value,
                                               int value_len) {
  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");
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
  ten_go_msg_set_property(self, path, path_len, c_value);

  return status;
}

ten_go_status_t ten_go_msg_property_set_buf(uintptr_t bridge_addr,
                                            const void *path, int path_len,
                                            void *value, int value_len) {
  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Invalid argument.");
  TEN_ASSERT(path && path_len > 0, "Invalid argument.");

  // The size must be > 0 when calling TEN_MALLOC().
  TEN_ASSERT(value && value_len > 0, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_value_t *c_value = ten_go_ten_value_create_buf(value, value_len);
  ten_go_msg_set_property(self, path, path_len, c_value);

  return status;
}

ten_go_status_t ten_go_msg_property_set_ptr(uintptr_t bridge_addr,
                                            const void *path, int path_len,
                                            ten_go_handle_t value) {
  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_value_t *c_value = ten_go_ten_value_create_ptr(value);
  ten_go_msg_set_property(self, path, path_len, c_value);

  return status;
}

ten_go_status_t ten_go_msg_property_get_json_and_size(uintptr_t bridge_addr,
                                                      const void *path,
                                                      int path_len,
                                                      uintptr_t *json_str_len,
                                                      const char **json_str) {
  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");
  TEN_ASSERT(json_str_len && json_str, "Should not happen.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_value_t *value = ten_go_msg_property_get_and_check_if_exists(
      self, path, path_len, &status);
  if (value == NULL) {
    return status;
  }

  ten_go_ten_value_to_json(value, json_str_len, json_str, &status);

  return status;
}

ten_go_status_t ten_go_msg_property_set_json_bytes(uintptr_t bridge_addr,
                                                   const void *path,
                                                   int path_len,
                                                   const void *json_str,
                                                   int json_str_len) {
  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(path && path_len > 0, "Should not happen.");
  TEN_ASSERT(json_str && json_str_len > 0, "Should not happen.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_json_t *json = ten_go_json_loads(json_str, json_str_len, &status);
  if (json == NULL) {
    return status;
  }

  ten_value_t *value = ten_value_from_json(json);
  ten_json_destroy(json);

  ten_go_msg_set_property(self, path, path_len, value);
  return status;
}

void ten_go_msg_finalize(uintptr_t bridge_addr) {
  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_msg_check_integrity(self), "Should not happen.");

  if (self->c_msg) {
    ten_shared_ptr_destroy(self->c_msg);
    self->c_msg = NULL;
  }

  TEN_FREE(self);
}

ten_go_status_t ten_go_msg_get_name(uintptr_t bridge_addr, const char **name) {
  TEN_ASSERT(bridge_addr > 0 && name, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_go_msg_t *self = ten_go_msg_reinterpret(bridge_addr);
  const char *msg_name = ten_msg_get_name(ten_go_msg_c_msg(self));
  TEN_ASSERT(msg_name, "Should not happen.");

  *name = msg_name;
  return status;
}
