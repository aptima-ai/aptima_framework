//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/msg/cmd_base/cmd/cmd.h"

typedef struct axis_cmd_timeout_t {
  axis_cmd_t cmd_hdr;

  axis_value_t timer_id;  // uint32
} axis_cmd_timeout_t;

axis_RUNTIME_PRIVATE_API void axis_cmd_timeout_set_timer_id(
    axis_shared_ptr_t *self, uint32_t timer_id);

axis_RUNTIME_PRIVATE_API axis_shared_ptr_t *axis_cmd_timeout_create(
    uint32_t timer_id);

axis_RUNTIME_PRIVATE_API void axis_raw_cmd_timeout_as_msg_destroy(
    axis_msg_t *self);

axis_RUNTIME_PRIVATE_API void axis_raw_cmd_timeout_set_timer_id(
    axis_cmd_timeout_t *self, uint32_t timer_id);

axis_RUNTIME_PRIVATE_API uint32_t
axis_raw_cmd_timeout_get_timer_id(axis_cmd_timeout_t *self);

axis_RUNTIME_API uint32_t axis_cmd_timeout_get_timer_id(axis_shared_ptr_t *self);

axis_RUNTIME_PRIVATE_API bool axis_raw_cmd_timeout_loop_all_fields(
    axis_msg_t *self, axis_raw_msg_process_one_field_func_t cb, void *user_data,
    axis_error_t *err);
