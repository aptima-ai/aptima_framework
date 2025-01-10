//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

#include "include_internal/axis_runtime/msg/loop_fields.h"
#include "axis_utils/container/list.h"
#include "axis_utils/value/value.h"

typedef struct axis_msg_t axis_msg_t;
typedef struct axis_error_t axis_error_t;
typedef struct axis_c_value_t axis_c_value_t;

axis_RUNTIME_API axis_list_t *axis_raw_msg_get_properties(axis_msg_t *self);

axis_RUNTIME_API bool axis_msg_del_property(axis_shared_ptr_t *self,
                                          const char *path);

axis_RUNTIME_API axis_value_t *axis_raw_msg_peek_property(axis_msg_t *self,
                                                       const char *path,
                                                       axis_error_t *err);

axis_RUNTIME_API void axis_raw_msg_properties_copy(
    axis_msg_t *self, axis_msg_t *src, axis_list_t *excluded_field_ids);

axis_RUNTIME_PRIVATE_API bool axis_raw_msg_properties_process(
    axis_msg_t *self, axis_raw_msg_process_one_field_func_t cb, void *user_data,
    axis_error_t *err);
