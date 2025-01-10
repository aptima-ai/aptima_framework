//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/msg/cmd_base/cmd_base.h"

#define axis_CMD_SIGNATURE 0x4DAF493D677D0269U

// Every command struct should starts with this.
typedef struct axis_cmd_t {
  axis_cmd_base_t cmd_base_hdr;

  axis_signature_t signature;
} axis_cmd_t;

axis_RUNTIME_PRIVATE_API void axis_raw_cmd_init(axis_cmd_t *self,
                                              axis_MSG_TYPE type);

axis_RUNTIME_PRIVATE_API void axis_raw_cmd_deinit(axis_cmd_t *self);

axis_RUNTIME_PRIVATE_API void axis_raw_cmd_copy_field(
    axis_msg_t *self, axis_msg_t *src, axis_list_t *excluded_field_ids);

axis_RUNTIME_PRIVATE_API bool axis_raw_cmd_process_field(
    axis_msg_t *self, axis_raw_msg_process_one_field_func_t cb, void *user_data,
    axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool axis_raw_cmd_check_integrity(axis_cmd_t *self);

axis_RUNTIME_API bool axis_cmd_check_integrity(axis_shared_ptr_t *self);

axis_RUNTIME_PRIVATE_API void axis_raw_cmd_destroy(axis_cmd_t *self);
