//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include <stdint.h>

#include "common.h"

ten_go_status_t ten_go_data_create(const void *msg_name, int msg_name_len,
                                   uintptr_t *bridge);

ten_go_status_t ten_go_data_alloc_buf(uintptr_t bridge_addr, int size);

ten_go_status_t ten_go_data_lock_buf(uintptr_t bridge_addr, uint8_t **buf_addr,
                                     uint64_t *buf_size);

ten_go_status_t ten_go_data_unlock_buf(uintptr_t bridge_addr,
                                       const void *buf_addr);

ten_go_status_t ten_go_data_get_buf(uintptr_t bridge_addr, const void *buf_addr,
                                    int buf_size);
