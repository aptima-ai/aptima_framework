//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

#include "include_internal/axis_runtime/msg/cmd_base/cmd/cmd.h"
#include "axis_utils/container/list.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/value/value.h"

typedef struct axis_msg_t axis_msg_t;

typedef struct axis_cmd_timer_t {
  axis_cmd_t cmd_hdr;

  axis_value_t timer_id;       // uint32
  axis_value_t timeout_in_us;  // uint64

  // axis_TIMER_INFINITE means "forever"
  // axis_TIMER_CANCEL means "cancel the timer with 'timer_id'"
  axis_value_t times;  // int32
} axis_cmd_timer_t;

axis_RUNTIME_PRIVATE_API bool axis_raw_cmd_timer_set_axis_property(
    axis_msg_t *self, axis_list_t *paths, axis_value_t *value, axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool axis_raw_cmd_timer_set_timer_id(
    axis_cmd_timer_t *self, uint32_t timer_id);

axis_RUNTIME_API bool axis_raw_cmd_timer_set_times(axis_cmd_timer_t *self,
                                                 int32_t times);

axis_RUNTIME_PRIVATE_API uint32_t
axis_raw_cmd_timer_get_timer_id(axis_cmd_timer_t *self);

axis_RUNTIME_PRIVATE_API uint32_t
axis_cmd_timer_get_timer_id(axis_shared_ptr_t *self);

axis_RUNTIME_API bool axis_cmd_timer_set_timer_id(axis_shared_ptr_t *self,
                                                uint32_t timer_id);

axis_RUNTIME_PRIVATE_API uint64_t
axis_raw_cmd_timer_get_timeout_in_us(axis_cmd_timer_t *self);

axis_RUNTIME_PRIVATE_API uint64_t
axis_cmd_timer_get_timeout_in_us(axis_shared_ptr_t *self);

axis_RUNTIME_API bool axis_cmd_timer_set_timeout_in_us(axis_shared_ptr_t *self,
                                                     uint64_t timeout_in_us);

axis_RUNTIME_PRIVATE_API int32_t
axis_raw_cmd_timer_get_times(axis_cmd_timer_t *self);

axis_RUNTIME_PRIVATE_API int32_t axis_cmd_timer_get_times(axis_shared_ptr_t *self);

axis_RUNTIME_API bool axis_cmd_timer_set_times(axis_shared_ptr_t *self,
                                             int32_t times);

axis_RUNTIME_PRIVATE_API void axis_raw_cmd_timer_as_msg_destroy(axis_msg_t *self);

axis_RUNTIME_PRIVATE_API axis_cmd_timer_t *axis_raw_cmd_timer_create(void);

axis_RUNTIME_API axis_shared_ptr_t *axis_cmd_timer_create(void);

axis_RUNTIME_PRIVATE_API bool axis_raw_cmd_timer_loop_all_fields(
    axis_msg_t *self, axis_raw_msg_process_one_field_func_t cb, void *user_data,
    axis_error_t *err);
