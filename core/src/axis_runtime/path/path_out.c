//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/path/path_out.h"

#include "axis_utils/lib/alloc.h"
#include "axis_utils/macro/check.h"

axis_path_out_t *axis_path_out_create(
    axis_path_table_t *table, const char *cmd_name, const char *parent_cmd_id,
    const char *cmd_id, axis_loc_t *src_loc, axis_loc_t *dest_loc,
    axis_env_msg_result_handler_func_t result_handler,
    void *result_handler_data) {
  axis_path_out_t *self = (axis_path_out_t *)axis_MALLOC(sizeof(axis_path_out_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_path_init((axis_path_t *)self, table, axis_PATH_OUT, cmd_name,
                parent_cmd_id, cmd_id, src_loc, dest_loc);

  self->result_handler = result_handler;
  self->result_handler_data = result_handler_data;

  self->base.result_conversion = NULL;

  return self;
}

void axis_path_out_destroy(axis_path_out_t *self) {
  axis_ASSERT(self, "Should not happen.");

  axis_path_deinit((axis_path_t *)self);
  axis_FREE(self);
}
