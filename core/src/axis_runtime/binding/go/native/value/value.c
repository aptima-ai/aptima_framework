//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_runtime/binding/go/interface/aptima/value.h"

#include <stdint.h>
#include <stdlib.h>

#include "include_internal/axis_runtime/binding/go/internal/common.h"
#include "include_internal/axis_runtime/binding/go/value/value.h"
#include "include_internal/axis_utils/value/value_smart_ptr.h"
#include "axis_runtime/binding/go/interface/aptima/common.h"
#include "axis_runtime/common/errno.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/buf.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/value/type.h"
#include "axis_utils/value/type_operation.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_get.h"
#include "axis_utils/value/value_is.h"

// @{
// TODO(Liu): Deprecated.

axis_go_handle_t tenGoCreateValue(axis_go_value_t *);

void tenGoUnrefObj(axis_go_handle_t);

// The definition of tenUnpinGoPointer is in GO world, and tenUnpinGoPointer is
// exported to C. So we need to declare it, then it can be called from C to GO.
//
// Before a GO pointer is set as a property of a msg or aptima instance, it will be
// pinned into the handle map in GO world. And the handle id pointing to the GO
// pointer will be set as the property value, not the GO pointer itself. When
// the msg or aptima instance has been reclaimed by APTIMA runtime, the GO pointer
// must be unpinned from the handle map to avoid memory leak. This function is
// used to unpinned the GO pointer.
void tenUnpinGoPointer(axis_go_handle_t);

bool axis_go_value_check_integrity(axis_go_value_t *self) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) != axis_GO_VALUE_SIGNATURE) {
    return false;
  }

  return true;
}

axis_go_handle_t axis_go_value_go_handle(axis_go_value_t *self) {
  axis_ASSERT(self, "Should not happen.");

  return self->bridge.go_instance;
}

axis_value_t *axis_go_value_c_value(axis_go_value_t *self) {
  axis_ASSERT(self, "Should not happen.");

  return self->c_value;
}

static void axis_go_value_destroy_v1(axis_go_value_t *self) {
  axis_ASSERT(self && axis_go_value_check_integrity(self), "Should not happen.");

  if (self->own) {
    axis_value_destroy(self->c_value);
  }

  axis_FREE(self);
}

static axis_go_value_t *axis_go_create_empty_value(void) {
  axis_go_value_t *value_bridge =
      (axis_go_value_t *)axis_MALLOC(sizeof(axis_go_value_t));
  axis_ASSERT(value_bridge, "Failed to allocate memory.");

  axis_signature_set(&value_bridge->signature, axis_GO_VALUE_SIGNATURE);
  value_bridge->bridge.go_instance = tenGoCreateValue(value_bridge);

  value_bridge->bridge.sp_ref_by_go =
      axis_shared_ptr_create(value_bridge, axis_go_value_destroy_v1);
  value_bridge->bridge.sp_ref_by_c = NULL;

  return value_bridge;
}

axis_go_handle_t axis_go_wrap_value(axis_value_t *c_value, bool own) {
  axis_ASSERT(c_value && axis_value_check_integrity(c_value),
             "Should not happen.");

  axis_go_value_t *value_bridge = axis_go_create_empty_value();
  value_bridge->c_value = c_value;
  value_bridge->own = own;

  return value_bridge->bridge.go_instance;
}

void axis_go_value_finalize(axis_go_value_t *self) {
  axis_ASSERT(self && axis_go_value_check_integrity(self), "Should not happen.");

  axis_go_bridge_destroy_go_part(&self->bridge);
}

// @}

void axis_go_axis_value_get_type_and_size(axis_value_t *self, uint8_t *type,
                                        uintptr_t *size) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Should not happen.");

  axis_TYPE prop_type = axis_value_get_type(self);
  *type = prop_type;

  switch (prop_type) {
    case axis_TYPE_BUF: {
      axis_buf_t *buf = axis_value_peek_buf(self);
      *size = buf ? axis_buf_get_size(buf) : 0;
      break;
    }

    case axis_TYPE_STRING: {
      const char *str = axis_value_peek_raw_str(self, NULL);
      axis_ASSERT(str, "Should not happen.");

      *size = strlen(str);
      break;
    }

    default:
      // For other types, the size is always 0.
      break;
  }
}

void axis_go_axis_value_get_string(axis_value_t *self, void *value,
                                 axis_go_error_t *status) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Should not happen.");
  axis_ASSERT(value && status, "Should not happen.");

  if (!axis_value_is_string(self)) {
    axis_go_error_set_errno(status, axis_ERRNO_GENERIC);
    return;
  }

  const char *str_value = axis_value_peek_raw_str(self, NULL);
  axis_ASSERT(str_value, "Should not happen");

  // The value is a pointer to a GO slice which has no space for the null
  // terminator.
  //
  // If we use strcpy here, the byte next to the GO slice will be set to '\0',
  // as strcpy copies the null terminator to the dest. If the memory space has
  // been allocated for other variables in GO world, ex: a GO string, the first
  // byte of the variable will be modified. The memory layout is as follows.
  //
  // We prefer to use `memcpy` rather than `strncpy`, because `memcpy` has a
  // performance optimization on some platforms.
  memcpy(value, str_value, strlen(str_value));
}

void axis_go_axis_value_get_buf(axis_value_t *self, void *value,
                              axis_go_error_t *status) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Should not happen.");
  axis_ASSERT(value && status, "Should not happen.");

  if (!axis_value_is_buf(self)) {
    axis_go_error_set_errno(status, axis_ERRNO_GENERIC);
    return;
  }

  axis_buf_t *buf = axis_value_peek_buf(self);
  if (buf) {
    memcpy(value, axis_buf_get_data(buf), axis_buf_get_size(buf));
  }
}

void axis_go_axis_value_get_ptr(axis_value_t *self, axis_go_handle_t *value,
                              axis_go_error_t *status) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Should not happen.");
  axis_ASSERT(value && status, "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  axis_shared_ptr_t *handle_ptr = axis_value_get_ptr(self, &err);
  if (axis_error_is_success(&err)) {
    void *go_ref = axis_shared_ptr_get_data(handle_ptr);
    *value = (axis_go_handle_t)go_ref;
  } else {
    axis_go_error_from_error(status, &err);
  }

  axis_error_deinit(&err);
}

axis_value_t *axis_go_axis_value_create_buf(void *value, int value_len) {
  axis_ASSERT(value, "Should not happen.");

  axis_buf_t buf;
  axis_buf_init_with_owned_data(&buf, value_len);

  memcpy(buf.data, value, value_len);

  axis_value_t *c_value = axis_value_create_buf_with_move(buf);
  axis_ASSERT(c_value && axis_value_check_integrity(c_value),
             "Should not happen.");

  return c_value;
}

static void axis_go_handle_unpin_from_go(void *v) {
  axis_go_handle_t handle = (axis_go_handle_t)v;
  tenUnpinGoPointer(handle);
}

axis_value_t *axis_go_axis_value_create_ptr(axis_go_handle_t value) {
  axis_ASSERT(value > 0, "Should not happen.");

  // The base type of axis_go_handle_t is uintptr_t, whose bit size is same as
  // void*. It's ok to reinterpret the `value` as a void*. However, the `value`
  // is not an ordinary pointer actually, it is an index pointing to a GO
  // pointer in the handle map in the GO world. So the reinterpreted pointer can
  // not be dereferenced.
  void *handle = (void *)value;  // NOLINT(performance-no-int-to-ptr)

  // The reason why we need to create a shared_ptr here is as follows.
  //
  // A axis_go_handle_t is a reference to a GO pointer in the handle map in the
  // GO world. The handle map is used to pin the GO pointer when it is used as a
  // property of a msg.
  //
  // When extension A sets a GO pointer as a property of a msg, the GO pointer
  // will be pinned into the handle map. The relationship is as follows.
  //
  //                                 HandleMap (GO)
  //                                  <key, value>
  //                                    ^     |
  //                                    |     +--> A GO pointer.
  //                          +- equal -+
  //                          |
  //   msg.SetProperty(key, value)
  //                          |
  //                          +--> A axis_go_handle_t.
  //
  // Imagine that extension B is the upstream of A, and it gets the GO pointer
  // from the msg. The relationship is as follows.
  //
  //                                 HandleMap (GO)
  //                                  <key, value>
  //                                    ^     |
  //                                    |     +--> A GO pointer.
  //                          +- equal -+
  //                          |
  //                        value = msg.GetProperty(key)
  //
  // So the GO pointer in the handle map _must_ be pinned until extension B has
  // handed the msg over to APTIMA runtime. Thus, the GO pointer can only be
  // unpinned from C to GO, but not be unpinned once the msg is sent out from
  // extension A. And if A has more than one consumer, the GO pointer must be
  // pinned until all the consumers have handed the msgs over to APTIMA runtime.
  // That's what `axis_go_handle_unpin_from_go` will do.
  axis_shared_ptr_t *handle_ptr =
      axis_shared_ptr_create(handle, axis_go_handle_unpin_from_go);
  axis_ASSERT(handle_ptr, "Should not happen.");

  axis_value_t *c_value = axis_value_create_ptr(
      axis_shared_ptr_clone(handle_ptr), axis_value_construct_for_smart_ptr,
      axis_value_copy_for_smart_ptr, axis_value_destruct_for_smart_ptr);
  axis_ASSERT(c_value && axis_value_check_integrity(c_value),
             "Should not happen.");

  axis_shared_ptr_destroy(handle_ptr);

  return c_value;
}

bool axis_go_axis_value_to_json(axis_value_t *self, uintptr_t *json_str_len,
                              const char **json_str, axis_go_error_t *status) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Should not happen.");
  axis_ASSERT(json_str_len && json_str && status, "Should not happen.");

  axis_json_t *json = axis_value_to_json(self);
  if (json == NULL) {
    axis_string_t err_msg;
    axis_string_init_formatted(&err_msg, "the property type is %s",
                              axis_type_to_string(axis_value_get_type(self)));

    axis_go_error_set(status, axis_ERRNO_GENERIC,
                     axis_string_get_raw_str(&err_msg));

    axis_string_deinit(&err_msg);

    return false;
  }

  // The json bytes are allocated by axis_json_to_string, and will be freed after
  // the GO slice is created. The GO slice must be created in GO world, as the
  // underlying buffer to the slice must be in the GO heap, that's why we have
  // to return the json bytes and the length to GO world first. And then create
  // a slice in GO world, and copy the json bytes to the slice and destroy the
  // json bytes. It will be done in axis_go_copy_c_str_to_slice_and_free.
  bool must_free = false;
  *json_str = axis_json_to_string(json, NULL, &must_free);
  axis_json_destroy(json);

  *json_str_len = strlen(*json_str);

  return true;
}

// Note that value_addr is the bit pattern of the pointer to axis_value_t, not a
// value bridge. There is no bridge for axis_value_t, as no GO object is created
// for it.
static axis_value_t *axis_go_value_reinterpret(uintptr_t value_addr) {
  axis_ASSERT(value_addr > 0, "Should not happen.");

  // NOLINTNEXTLINE(performance-no-int-to-ptr)
  axis_value_t *self = (axis_value_t *)value_addr;
  axis_ASSERT(self && axis_value_check_integrity(self), "Should not happen.");

  return self;
}

axis_go_error_t axis_go_value_get_string(uintptr_t value_addr, void *value) {
  axis_value_t *self = axis_go_value_reinterpret(value_addr);
  axis_ASSERT(self && axis_value_check_integrity(self), "Should not happen.");
  axis_ASSERT(value, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_axis_value_get_string(self, value, &cgo_error);

  axis_value_destroy(self);

  return cgo_error;
}

axis_go_error_t axis_go_value_get_buf(uintptr_t value_addr, void *value) {
  axis_value_t *self = axis_go_value_reinterpret(value_addr);
  axis_ASSERT(self && axis_value_check_integrity(self), "Should not happen.");
  axis_ASSERT(value, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_axis_value_get_buf(self, value, &cgo_error);

  axis_value_destroy(self);

  return cgo_error;
}

void axis_go_value_destroy(uintptr_t value_addr) {
  axis_value_t *self = axis_go_value_reinterpret(value_addr);
  axis_ASSERT(self && axis_value_check_integrity(self), "Should not happen.");

  axis_value_destroy(self);
}
