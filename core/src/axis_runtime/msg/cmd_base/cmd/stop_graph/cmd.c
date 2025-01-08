//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/msg/cmd_base/cmd/stop_graph/cmd.h"

#include <stdlib.h>

#include "include_internal/axis_runtime/msg/cmd_base/cmd/cmd.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/stop_graph/field/field_info.h"
#include "include_internal/axis_runtime/msg/field/field_info.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_utils/value/value_set.h"
#include "axis_runtime/msg/cmd/stop_graph/cmd.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"

static axis_cmd_stop_graph_t *get_raw_cmd(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_cmd_base_check_integrity(self), "Should not happen.");
  return (axis_cmd_stop_graph_t *)axis_shared_ptr_get_data(self);
}

static void axis_raw_cmd_stop_graph_destroy(axis_cmd_stop_graph_t *self) {
  axis_ASSERT(self, "Should not happen.");

  axis_raw_cmd_deinit(&self->cmd_hdr);

  axis_value_deinit(&self->graph_id);

  axis_FREE(self);
}

void axis_raw_cmd_stop_graph_as_msg_destroy(axis_msg_t *self) {
  axis_ASSERT(self, "Should not happen.");
  axis_raw_cmd_stop_graph_destroy((axis_cmd_stop_graph_t *)self);
}

axis_cmd_stop_graph_t *axis_raw_cmd_stop_graph_create(void) {
  axis_cmd_stop_graph_t *raw_cmd = axis_MALLOC(sizeof(axis_cmd_stop_graph_t));
  axis_ASSERT(raw_cmd, "Failed to allocate memory.");

  axis_raw_cmd_init(&raw_cmd->cmd_hdr, axis_MSG_TYPE_CMD_STOP_GRAPH);

  axis_value_init_string(&raw_cmd->graph_id);

  return raw_cmd;
}

axis_shared_ptr_t *axis_cmd_stop_graph_create(void) {
  return axis_shared_ptr_create(axis_raw_cmd_stop_graph_create(),
                               axis_raw_cmd_stop_graph_destroy);
}

axis_json_t *axis_raw_cmd_stop_graph_to_json(axis_msg_t *self, axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_get_type((axis_msg_t *)self) ==
                         axis_MSG_TYPE_CMD_STOP_GRAPH,
             "Should not happen.");

  axis_json_t *json = axis_json_create_object();
  axis_ASSERT(json, "Should not happen.");

  if (!axis_raw_cmd_stop_graph_loop_all_fields(
          self, axis_raw_msg_put_one_field_to_json, json, err)) {
    axis_json_destroy(json);
    return NULL;
  }

  return json;
}

static const char *axis_raw_cmd_stop_graph_get_graph_id(
    axis_cmd_stop_graph_t *self) {
  axis_ASSERT(self && axis_raw_cmd_check_integrity((axis_cmd_t *)self) &&
                 axis_raw_msg_get_type((axis_msg_t *)self) ==
                     axis_MSG_TYPE_CMD_STOP_GRAPH,
             "Should not happen.");

  axis_string_t *graph_id = axis_value_peek_string(&self->graph_id);
  return axis_string_get_raw_str(graph_id);
}

const char *axis_cmd_stop_graph_get_graph_id(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_msg_get_type(self) == axis_MSG_TYPE_CMD_STOP_GRAPH,
             "Should not happen.");

  return axis_raw_cmd_stop_graph_get_graph_id(get_raw_cmd(self));
}

static bool axis_raw_cmd_stop_graph_set_graph_id(axis_cmd_stop_graph_t *self,
                                                const char *graph_id) {
  axis_ASSERT(self && axis_raw_cmd_check_integrity((axis_cmd_t *)self) &&
                 axis_raw_msg_get_type((axis_msg_t *)self) ==
                     axis_MSG_TYPE_CMD_STOP_GRAPH,
             "Should not happen.");

  return axis_value_set_string(&self->graph_id, graph_id);
}

bool axis_cmd_stop_graph_set_graph_id(axis_shared_ptr_t *self,
                                     const char *graph_id) {
  axis_ASSERT(self && axis_msg_get_type(self) == axis_MSG_TYPE_CMD_STOP_GRAPH,
             "Should not happen.");

  return axis_raw_cmd_stop_graph_set_graph_id(get_raw_cmd(self), graph_id);
}

bool axis_raw_cmd_stop_graph_loop_all_fields(
    axis_msg_t *self, axis_raw_msg_process_one_field_func_t cb, void *user_data,
    axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");

  for (size_t i = 0; i < axis_cmd_stop_graph_fields_info_size; ++i) {
    axis_msg_process_field_func_t process_field =
        axis_cmd_stop_graph_fields_info[i].process_field;
    if (process_field) {
      if (!process_field(self, cb, user_data, err)) {
        return false;
      }
    }
  }

  return true;
}
