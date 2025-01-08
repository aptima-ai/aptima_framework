//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_runtime/binding/go/interface/ten/data.h"

#include "include_internal/axis_runtime/binding/go/internal/common.h"
#include "include_internal/axis_runtime/binding/go/msg/msg.h"
#include "include_internal/axis_runtime/msg/data/data.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "axis_runtime/binding/go/interface/ten/msg.h"
#include "axis_runtime/common/errno.h"
#include "axis_runtime/msg/data/data.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/macro/check.h"

axis_go_error_t axis_go_data_create(const void *name, int name_len,
                                  uintptr_t *bridge) {
  axis_ASSERT(bridge, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_shared_ptr_t *c_data =
      axis_data_create_with_name_len(name, name_len, NULL);
  axis_ASSERT(c_data, "Should not happen.");

  axis_go_msg_t *data_bridge = axis_go_msg_create(c_data);
  axis_ASSERT(data_bridge && axis_go_msg_check_integrity(data_bridge),
             "Should not happen.");

  uintptr_t addr = (uintptr_t)data_bridge;
  *bridge = addr;

  // The ownership of the C message instance is transferred into the GO message
  // instance.
  axis_shared_ptr_destroy(c_data);

  return cgo_error;
}

axis_go_error_t axis_go_data_alloc_buf(uintptr_t bridge_addr, int size) {
  axis_ASSERT(bridge_addr && size, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_msg_t *data_bridge = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(data_bridge && axis_go_msg_check_integrity(data_bridge),
             "Invalid argument.");

  axis_shared_ptr_t *c_data = axis_go_msg_c_msg(data_bridge);
  uint8_t *data = axis_data_alloc_buf(c_data, size);
  if (!data) {
    axis_go_error_set(&cgo_error, axis_ERRNO_GENERIC,
                     "failed to allocate memory");
  }

  return cgo_error;
}

axis_go_error_t axis_go_data_lock_buf(uintptr_t bridge_addr, uint8_t **buf_addr,
                                    uint64_t *buf_size) {
  axis_ASSERT(bridge_addr && buf_size, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_msg_t *data_bridge = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(data_bridge && axis_go_msg_check_integrity(data_bridge),
             "Invalid argument.");

  axis_shared_ptr_t *c_data = axis_go_msg_c_msg(data_bridge);
  uint8_t *c_data_data = axis_data_peek_buf(c_data)->data;

  axis_error_t c_err;
  axis_error_init(&c_err);

  if (!axis_msg_add_locked_res_buf(c_data, c_data_data, &c_err)) {
    axis_go_error_set(&cgo_error, axis_error_errno(&c_err),
                     axis_error_errmsg(&c_err));
  } else {
    *buf_addr = c_data_data;
    *buf_size = axis_data_peek_buf(c_data)->size;
  }

  axis_error_deinit(&c_err);

  return cgo_error;
}

axis_go_error_t axis_go_data_unlock_buf(uintptr_t bridge_addr,
                                      const void *buf_addr) {
  axis_ASSERT(bridge_addr && buf_addr, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_msg_t *data_bridge = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(data_bridge && axis_go_msg_check_integrity(data_bridge),
             "Invalid argument.");

  axis_shared_ptr_t *c_data = axis_go_msg_c_msg(data_bridge);

  axis_error_t c_err;
  axis_error_init(&c_err);

  bool result = axis_msg_remove_locked_res_buf(c_data, buf_addr, &c_err);
  if (!result) {
    axis_go_error_set(&cgo_error, axis_error_errno(&c_err),
                     axis_error_errmsg(&c_err));
  }

  axis_error_deinit(&c_err);

  return cgo_error;
}

axis_go_error_t axis_go_data_get_buf(uintptr_t bridge_addr, const void *buf_addr,
                                   uint64_t buf_size) {
  axis_ASSERT(bridge_addr && buf_addr && buf_size > 0, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_msg_t *data_bridge = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(data_bridge && axis_go_msg_check_integrity(data_bridge),
             "Invalid argument.");

  axis_shared_ptr_t *c_data = axis_go_msg_c_msg(data_bridge);
  uint64_t size = axis_data_peek_buf(c_data)->size;
  if (buf_size < size) {
    axis_go_error_set(&cgo_error, axis_ERRNO_GENERIC, "buffer is not enough");
  } else {
    uint8_t *data = axis_data_peek_buf(c_data)->data;
    memcpy((void *)buf_addr, data, size);
  }

  return cgo_error;
}

axis_go_error_t axis_go_data_get_buf_size(uintptr_t bridge_addr,
                                        uint64_t *buf_size) {
  axis_ASSERT(bridge_addr, "Invalid argument.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_go_msg_t *data_bridge = axis_go_msg_reinterpret(bridge_addr);
  axis_ASSERT(data_bridge && axis_go_msg_check_integrity(data_bridge),
             "Invalid argument.");

  axis_shared_ptr_t *c_data = axis_go_msg_c_msg(data_bridge);
  uint64_t size = axis_data_peek_buf(c_data)->size;

  *buf_size = size;

  return cgo_error;
}
