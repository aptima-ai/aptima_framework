//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <stdbool.h>

#include "include_internal/ten_runtime/msg/cmd_base/cmd/cmd.h"
#include "ten_utils/container/list.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/value/value.h"

typedef struct ten_msg_t ten_msg_t;

typedef struct ten_cmd_timer_t {
  ten_cmd_t cmd_hdr;

  uint32_t timer_id;
  uint64_t timeout_in_us;

  // TEN_TIMER_INFINITE means "forever"
  // TEN_TIMER_CANCEL means "cancel the timer with 'timer_id'"
  int32_t times;
} ten_cmd_timer_t;

TEN_RUNTIME_PRIVATE_API bool ten_raw_cmd_timer_set_ten_property(
    ten_msg_t *self, ten_list_t *paths, ten_value_t *value, ten_error_t *err);

TEN_RUNTIME_PRIVATE_API bool ten_raw_cmd_timer_check_type_and_name(
    ten_msg_t *self, const char *type_str, const char *name_str,
    ten_error_t *err);

TEN_RUNTIME_PRIVATE_API void ten_raw_cmd_timer_set_timer_id(
    ten_cmd_timer_t *self, uint32_t timer_id);

TEN_RUNTIME_PRIVATE_API void ten_raw_cmd_timer_set_times(ten_cmd_timer_t *self,
                                                         int32_t times);

TEN_RUNTIME_PRIVATE_API uint32_t
ten_raw_cmd_timer_get_timer_id(ten_cmd_timer_t *self);

TEN_RUNTIME_PRIVATE_API uint32_t
ten_cmd_timer_get_timer_id(ten_shared_ptr_t *self);

TEN_RUNTIME_PRIVATE_API void ten_cmd_timer_set_timer_id(ten_shared_ptr_t *self,
                                                        uint32_t timer_id);

TEN_RUNTIME_PRIVATE_API uint64_t
ten_raw_cmd_timer_get_timeout_in_us(ten_cmd_timer_t *self);

TEN_RUNTIME_PRIVATE_API uint64_t
ten_cmd_timer_get_timeout_in_us(ten_shared_ptr_t *self);

TEN_RUNTIME_PRIVATE_API int32_t
ten_raw_cmd_timer_get_times(ten_cmd_timer_t *self);

TEN_RUNTIME_PRIVATE_API int32_t ten_cmd_timer_get_times(ten_shared_ptr_t *self);

TEN_RUNTIME_PRIVATE_API void ten_cmd_timer_set_times(ten_shared_ptr_t *self,
                                                     int32_t times);

TEN_RUNTIME_PRIVATE_API bool ten_raw_cmd_timer_as_msg_init_from_json(
    ten_msg_t *self, ten_json_t *json, ten_error_t *err);

TEN_RUNTIME_PRIVATE_API void ten_raw_cmd_timer_as_msg_destroy(ten_msg_t *self);

TEN_RUNTIME_PRIVATE_API ten_msg_t *ten_raw_cmd_timer_as_msg_create_from_json(
    ten_json_t *json, ten_error_t *err);

TEN_RUNTIME_PRIVATE_API ten_json_t *ten_raw_cmd_timer_as_msg_to_json(
    ten_msg_t *self, ten_error_t *err);

TEN_RUNTIME_PRIVATE_API ten_cmd_timer_t *ten_raw_cmd_timer_create(void);

TEN_RUNTIME_API ten_shared_ptr_t *ten_cmd_timer_create(void);
