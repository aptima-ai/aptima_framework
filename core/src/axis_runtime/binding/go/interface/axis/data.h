//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include <stdint.h>

#include "common.h"

axis_go_error_t axis_go_data_create(const void *msg_name, int msg_name_len,
                                  uintptr_t *bridge);

axis_go_error_t axis_go_data_alloc_buf(uintptr_t bridge_addr, int size);

axis_go_error_t axis_go_data_lock_buf(uintptr_t bridge_addr, uint8_t **buf_addr,
                                    uint64_t *buf_size);

axis_go_error_t axis_go_data_unlock_buf(uintptr_t bridge_addr,
                                      const void *buf_addr);

axis_go_error_t axis_go_data_get_buf(uintptr_t bridge_addr, const void *buf_addr,
                                   uint64_t buf_size);

axis_go_error_t axis_go_data_get_buf_size(uintptr_t bridge_addr,
                                        uint64_t *buf_size);
