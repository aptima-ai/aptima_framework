//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_runtime/binding/go/interface/ten/data.h"

#include "include_internal/ten_runtime/binding/go/internal/common.h"
#include "include_internal/ten_runtime/binding/go/msg/msg.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "ten_runtime/binding/go/interface/ten/msg.h"
#include "ten_runtime/common/errno.h"
#include "ten_runtime/msg/data/data.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/macro/check.h"

ten_go_status_t ten_go_data_create(const void *msg_name, int msg_name_len,
                                   uintptr_t *bridge) {
  TEN_ASSERT(bridge, "Should not happen.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_shared_ptr_t *c_data = ten_data_create();
  TEN_ASSERT(c_data, "Should not happen.");

  ten_msg_set_name_with_size(c_data, msg_name, msg_name_len, NULL);

  ten_go_msg_t *data_bridge = ten_go_msg_create(c_data);
  TEN_ASSERT(data_bridge && ten_go_msg_check_integrity(data_bridge),
             "Should not happen.");

  uintptr_t addr = (uintptr_t)data_bridge;
  *bridge = addr;

  return status;
}

ten_go_status_t ten_go_data_alloc_buf(uintptr_t bridge_addr, int size) {
  TEN_ASSERT(bridge_addr > 0 && size > 0, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_go_msg_t *data_bridge = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(data_bridge && ten_go_msg_check_integrity(data_bridge),
             "Invalid argument.");

  ten_shared_ptr_t *c_data = ten_go_msg_c_msg(data_bridge);
  uint8_t *data = ten_data_alloc_buf(c_data, size);
  if (!data) {
    ten_go_status_set(&status, TEN_ERRNO_GENERIC, "failed to allocate memory");
  }

  return status;
}

ten_go_status_t ten_go_data_lock_buf(uintptr_t bridge_addr, uint8_t **buf_addr,
                                     uint64_t *buf_size) {
  TEN_ASSERT(bridge_addr > 0 && buf_size, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_go_msg_t *data_bridge = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(data_bridge && ten_go_msg_check_integrity(data_bridge),
             "Invalid argument.");

  ten_shared_ptr_t *c_data = ten_go_msg_c_msg(data_bridge);
  uint8_t *c_data_data = ten_data_peek_buf(c_data)->data;

  ten_error_t c_err;
  ten_error_init(&c_err);

  if (!ten_msg_add_locked_res_buf(c_data, c_data_data, &c_err)) {
    ten_go_status_set(&status, ten_error_errno(&c_err),
                      ten_error_errmsg(&c_err));
  } else {
    *buf_addr = c_data_data;
    *buf_size = ten_data_peek_buf(c_data)->size;
  }

  ten_error_deinit(&c_err);

  return status;
}

ten_go_status_t ten_go_data_unlock_buf(uintptr_t bridge_addr,
                                       const void *buf_addr) {
  TEN_ASSERT(bridge_addr > 0 && buf_addr, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_go_msg_t *data_bridge = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(data_bridge && ten_go_msg_check_integrity(data_bridge),
             "Invalid argument.");

  ten_shared_ptr_t *c_data = ten_go_msg_c_msg(data_bridge);

  ten_error_t c_err;
  ten_error_init(&c_err);

  bool result = ten_msg_remove_locked_res_buf(c_data, buf_addr, &c_err);
  if (!result) {
    ten_go_status_set(&status, ten_error_errno(&c_err),
                      ten_error_errmsg(&c_err));
  }

  ten_error_deinit(&c_err);

  return status;
}

ten_go_status_t ten_go_data_get_buf(uintptr_t bridge_addr, const void *buf_addr,
                                    int buf_size) {
  TEN_ASSERT(bridge_addr > 0 && buf_addr && buf_size > 0, "Invalid argument.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_go_msg_t *data_bridge = ten_go_msg_reinterpret(bridge_addr);
  TEN_ASSERT(data_bridge && ten_go_msg_check_integrity(data_bridge),
             "Invalid argument.");

  ten_shared_ptr_t *c_data = ten_go_msg_c_msg(data_bridge);
  uint64_t size = ten_data_peek_buf(c_data)->size;
  if (buf_size < size) {
    ten_go_status_set(&status, TEN_ERRNO_GENERIC, "buffer is not enough");
  } else {
    uint8_t *data = ten_data_peek_buf(c_data)->data;
    memcpy((void *)buf_addr, data, size);
  }

  return status;
}
