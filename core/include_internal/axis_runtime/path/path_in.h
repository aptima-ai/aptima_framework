//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/path/path.h"

typedef struct axis_path_in_t {
  axis_path_t base;
} axis_path_in_t;

axis_RUNTIME_PRIVATE_API axis_path_in_t *axis_path_in_create(
    axis_path_table_t *table, const char *cmd_name, const char *parent_cmd_id,
    const char *cmd_id, axis_loc_t *src_loc, axis_loc_t *dest_loc,
    axis_msg_conversion_t *result_conversion);

axis_RUNTIME_PRIVATE_API void axis_path_in_destroy(axis_path_in_t *self);
