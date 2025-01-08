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

typedef struct axis_msg_t axis_msg_t;
typedef struct axis_error_t axis_error_t;

axis_RUNTIME_PRIVATE_API void axis_cmd_start_graph_copy_extensions_info(
    axis_msg_t *self, axis_msg_t *src, axis_list_t *excluded_field_ids);

axis_RUNTIME_PRIVATE_API bool axis_cmd_start_graph_process_extensions_info(
    axis_msg_t *self, axis_raw_msg_process_one_field_func_t cb, void *user_data,
    axis_error_t *err);
