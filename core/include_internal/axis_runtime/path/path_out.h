//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/path/path.h"
#include "axis_runtime/axis_env/internal/send.h"

typedef struct axis_path_out_t {
  axis_path_t base;

  axis_env_msg_result_handler_func_t result_handler;
  void *result_handler_data;
} axis_path_out_t;

axis_RUNTIME_PRIVATE_API axis_path_out_t *axis_path_out_create(
    axis_path_table_t *table, const char *cmd_name, const char *parent_cmd_id,
    const char *cmd_id, axis_loc_t *src_loc, axis_loc_t *dest_loc,
    axis_env_msg_result_handler_func_t result_handler,
    void *result_handler_data);

axis_RUNTIME_PRIVATE_API void axis_path_out_destroy(axis_path_out_t *self);
