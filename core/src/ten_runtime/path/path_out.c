//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "include_internal/ten_runtime/path/path_out.h"

#include "ten_utils/lib/alloc.h"
#include "ten_utils/macro/check.h"

ten_path_out_t *ten_path_out_create(
    ten_path_table_t *table, const char *cmd_name, const char *parent_cmd_id,
    const char *cmd_id, ten_loc_t *src_loc, ten_loc_t *dest_loc,
    ten_env_cmd_result_handler_func_t result_handler,
    void *result_handler_data) {
  ten_path_out_t *self = (ten_path_out_t *)TEN_MALLOC(sizeof(ten_path_out_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  ten_path_init((ten_path_t *)self, table, TEN_PATH_OUT, cmd_name,
                parent_cmd_id, cmd_id, src_loc, dest_loc);

  self->result_handler = result_handler;
  self->result_handler_data = result_handler_data;

  self->base.result_conversion = NULL;

  return self;
}

void ten_path_out_destroy(ten_path_out_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  ten_path_deinit((ten_path_t *)self);
  TEN_FREE(self);
}
