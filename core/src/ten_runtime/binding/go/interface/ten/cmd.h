//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include <stdint.h>

#include "common.h"
#include "msg.h"

typedef struct ten_go_value_t ten_go_value_t;

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
 * store the address of a `ten_go_msg_t` pointer. This function will allocate
 * and assign the bit pattern of a pointer to an appropriate `ten_go_msg_t`
 * instance to it, which is then returned to Go.
 */
ten_go_status_t ten_go_cmd_create_cmd(const void *cmd_name, int cmd_name_len,
                                      uintptr_t *bridge);

/**
 * @brief Create a custom cmd based on a json string.
 *
 * @param json_bytes Points to the buffer containing the JSON string. This
 * buffer originates from a Go slice and is passed as an `unsafe.Pointer`, hence
 * the type `void*`. It is important to note that only read operations are safe
 * on this buffer. Since the buffer's lifecycle is managed by Go, avoid reading
 * from it after the cgo call returns, as this could lead to undefined behavior.
 *
 * @param json_bytes_len The length of the buffer.
 *
 * @param bridge The pointer to a GO variable of type uintptr, which is used to
 * store the address of a `ten_go_msg_t` pointer. This function will allocate
 * and assign the bit pattern of a pointer to an appropriate `ten_go_msg_t`
 * instance to it, which is then returned to Go.
 */
ten_go_status_t ten_go_cmd_create_cmd_from_json(const void *json_bytes,
                                                int json_bytes_len,
                                                uintptr_t *bridge);

uintptr_t ten_go_cmd_create_cmd_result(int status_code);

int ten_go_cmd_result_get_status_code(uintptr_t bridge_addr);

ten_go_handle_t ten_go_cmd_result_get_detail(uintptr_t bridge_addr);

ten_go_status_t ten_go_cmd_result_get_detail_json_and_size(
    uintptr_t bridge_addr, uintptr_t *json_str_len, const char **json_str);
