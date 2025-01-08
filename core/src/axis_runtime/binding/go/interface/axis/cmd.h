//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include <stdint.h>

#include "common.h"
#include "msg.h"

typedef struct axis_go_value_t axis_go_value_t;

/**
 * @brief Create a cmd based on the cmd name.
 *
 * @param cmd_name The underlying buffer of the GO string, which is passed with
 * unsafe.Pointer in GO world, so the type of @a cmd_name is void*, not char*.
 * Only the read operation is permitted. And the buffer is managed by GO, do not
 * read it after the blocking cgo call.
 *
 * @param cmd_name_len The length of the buffer.
 *
 * @param bridge The pointer to a GO variable of type uintptr, which is used to
 * store the address of a `axis_go_msg_t` pointer. This function will allocate
 * and assign the bit pattern of a pointer to an appropriate `axis_go_msg_t`
 * instance to it, which is then returned to Go.
 */
axis_go_error_t axis_go_cmd_create_cmd(const void *cmd_name, int cmd_name_len,
                                     uintptr_t *bridge);

uintptr_t axis_go_cmd_create_cmd_result(int status_code);

int axis_go_cmd_result_get_status_code(uintptr_t bridge_addr);

axis_go_error_t axis_go_cmd_result_set_final(uintptr_t bridge_addr,
                                           bool is_final);

axis_go_error_t axis_go_cmd_result_is_final(uintptr_t bridge_addr,
                                          bool *is_final);

axis_go_error_t axis_go_cmd_result_is_completed(uintptr_t bridge_addr,
                                              bool *is_completed);

axis_go_handle_t axis_go_cmd_result_get_detail(uintptr_t bridge_addr);

axis_go_error_t axis_go_cmd_result_get_detail_json_and_size(
    uintptr_t bridge_addr, uintptr_t *json_str_len, const char **json_str);
