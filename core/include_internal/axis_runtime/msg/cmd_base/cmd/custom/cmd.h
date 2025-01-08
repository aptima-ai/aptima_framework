//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

#include "include_internal/axis_runtime/msg/loop_fields.h"
#include "axis_utils/container/list.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/value/value.h"

typedef struct axis_msg_t axis_msg_t;
typedef struct axis_cmd_t axis_cmd_t;

axis_RUNTIME_PRIVATE_API bool axis_raw_cmd_custom_set_axis_property(
    axis_msg_t *self, axis_list_t *paths, axis_value_t *value, axis_error_t *err);

axis_RUNTIME_PRIVATE_API void axis_raw_cmd_custom_as_msg_destroy(axis_msg_t *self);

axis_RUNTIME_PRIVATE_API axis_msg_t *axis_raw_cmd_custom_as_msg_clone(
    axis_msg_t *self, axis_list_t *excluded_field_ids);

axis_RUNTIME_PRIVATE_API axis_json_t *axis_raw_cmd_custom_to_json(
    axis_msg_t *self, axis_error_t *err);

axis_RUNTIME_API axis_shared_ptr_t *axis_cmd_custom_create_empty(void);

axis_RUNTIME_PRIVATE_API axis_cmd_t *axis_raw_cmd_custom_create(const char *name,
                                                             axis_error_t *err);

axis_RUNTIME_PRIVATE_API axis_shared_ptr_t *axis_cmd_custom_create(
    const char *name, axis_error_t *err);

axis_RUNTIME_API axis_shared_ptr_t *axis_cmd_custom_create_with_name_len(
    const char *name, size_t name_len, axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool axis_raw_cmd_custom_loop_all_fields(
    axis_msg_t *self, axis_raw_msg_process_one_field_func_t cb, void *user_data,
    axis_error_t *err);
