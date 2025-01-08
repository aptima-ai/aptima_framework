//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_runtime/msg/cmd/close_app/cmd.h"

#include <stdlib.h>

#include "include_internal/axis_runtime/msg/cmd_base/cmd/close_app/cmd.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/close_app/field/field_info.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/cmd.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/macro/check.h"

static void axis_raw_cmd_close_app_destroy(axis_cmd_close_app_t *self) {
  axis_ASSERT(self, "Should not happen.");

  axis_raw_cmd_deinit(&self->cmd_hdr);
  axis_FREE(self);
}

void axis_raw_cmd_close_app_as_msg_destroy(axis_msg_t *self) {
  axis_ASSERT(self, "Should not happen.");

  axis_raw_cmd_close_app_destroy((axis_cmd_close_app_t *)self);
}

axis_cmd_close_app_t *axis_raw_cmd_close_app_create(void) {
  axis_cmd_close_app_t *raw_cmd = axis_MALLOC(sizeof(axis_cmd_close_app_t));
  axis_ASSERT(raw_cmd, "Failed to allocate memory.");

  axis_raw_cmd_init(&raw_cmd->cmd_hdr, axis_MSG_TYPE_CMD_CLOSE_APP);

  return raw_cmd;
}

axis_shared_ptr_t *axis_cmd_close_app_create(void) {
  return axis_shared_ptr_create(axis_raw_cmd_close_app_create(),
                               axis_raw_cmd_close_app_destroy);
}

axis_json_t *axis_raw_cmd_close_app_to_json(axis_msg_t *self, axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_get_type(self) == axis_MSG_TYPE_CMD_CLOSE_APP,
             "Should not happen.");

  axis_json_t *json = axis_json_create_object();
  axis_ASSERT(json, "Should not happen.");

  if (!axis_raw_cmd_close_app_loop_all_fields(
          self, axis_raw_msg_put_one_field_to_json, json, err)) {
    axis_json_destroy(json);
    return NULL;
  }

  return json;
}

bool axis_raw_cmd_close_app_loop_all_fields(
    axis_msg_t *self, axis_raw_msg_process_one_field_func_t cb, void *user_data,
    axis_error_t *err) {
  axis_ASSERT(self && axis_raw_cmd_check_integrity((axis_cmd_t *)self) && cb,
             "Should not happen.");

  for (size_t i = 0; i < axis_cmd_close_app_fields_info_size; ++i) {
    axis_msg_process_field_func_t process_field =
        axis_cmd_close_app_fields_info[i].process_field;
    if (process_field) {
      if (!process_field(self, cb, user_data, err)) {
        return false;
      }
    }
  }

  return true;
}
