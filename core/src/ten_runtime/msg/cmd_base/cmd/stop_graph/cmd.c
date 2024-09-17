//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/msg/cmd_base/cmd/stop_graph/cmd.h"

#include <stdlib.h>

#include "include_internal/ten_runtime/msg/cmd_base/cmd/cmd.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd/stop_graph/field/field_info.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "ten_utils/macro/check.h"
#include "ten_runtime/msg/cmd/stop_graph/cmd.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/lib/smart_ptr.h"
#include "ten_utils/lib/string.h"

static ten_cmd_stop_graph_t *get_raw_cmd(ten_shared_ptr_t *self) {
  TEN_ASSERT(self && ten_cmd_base_check_integrity(self), "Should not happen.");
  return (ten_cmd_stop_graph_t *)ten_shared_ptr_get_data(self);
}

static void ten_raw_cmd_stop_graph_destroy(ten_cmd_stop_graph_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  ten_raw_cmd_deinit(&self->cmd_hdr);

  ten_string_deinit(&self->graph_name);

  TEN_FREE(self);
}

void ten_raw_cmd_stop_graph_as_msg_destroy(ten_msg_t *self) {
  TEN_ASSERT(self, "Should not happen.");
  ten_raw_cmd_stop_graph_destroy((ten_cmd_stop_graph_t *)self);
}

ten_cmd_stop_graph_t *ten_raw_cmd_stop_graph_create(void) {
  ten_cmd_stop_graph_t *raw_cmd = TEN_MALLOC(sizeof(ten_cmd_stop_graph_t));
  TEN_ASSERT(raw_cmd, "Failed to allocate memory.");

  ten_raw_cmd_init(&raw_cmd->cmd_hdr, TEN_MSG_TYPE_CMD_STOP_GRAPH);

  ten_string_init(&raw_cmd->graph_name);

  return raw_cmd;
}

ten_shared_ptr_t *ten_cmd_stop_graph_create(void) {
  return ten_shared_ptr_create(ten_raw_cmd_stop_graph_create(),
                               ten_raw_cmd_stop_graph_destroy);
}

static bool ten_raw_cmd_stop_graph_init_from_json(ten_cmd_stop_graph_t *self,
                                                  ten_json_t *json,
                                                  ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_cmd_check_integrity((ten_cmd_t *)self),
             "Should not happen.");
  TEN_ASSERT(json && ten_json_check_integrity(json), "Should not happen.");

  for (size_t i = 0; i < ten_cmd_stop_graph_fields_info_size; ++i) {
    ten_msg_get_field_from_json_func_t get_field_from_json =
        ten_cmd_stop_graph_fields_info[i].get_field_from_json;
    if (get_field_from_json) {
      if (!get_field_from_json(&(self->cmd_hdr.cmd_base_hdr.msg_hdr), json,
                               err)) {
        return false;
      }
    }
  }
  return true;
}

bool ten_raw_cmd_stop_graph_as_msg_init_from_json(ten_msg_t *self,
                                                  ten_json_t *json,
                                                  ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_cmd_check_integrity((ten_cmd_t *)self),
             "Should not happen.");
  TEN_ASSERT(json && ten_json_check_integrity(json), "Should not happen.");

  return ten_raw_cmd_stop_graph_init_from_json((ten_cmd_stop_graph_t *)self,
                                               json, err);
}

static ten_cmd_stop_graph_t *ten_raw_cmd_stop_graph_create_from_json(
    ten_json_t *json, ten_error_t *err) {
  TEN_ASSERT(
      json && ten_msg_json_get_msg_type(json) == TEN_MSG_TYPE_CMD_STOP_GRAPH,
      "Should not happen.");

  ten_cmd_stop_graph_t *cmd = ten_raw_cmd_stop_graph_create();
  TEN_ASSERT(cmd && ten_raw_cmd_check_integrity((ten_cmd_t *)cmd),
             "Should not happen.");

  if (!ten_raw_cmd_stop_graph_init_from_json(cmd, json, err)) {
    ten_raw_cmd_stop_graph_destroy(cmd);
    return NULL;
  }

  return cmd;
}

ten_msg_t *ten_raw_cmd_stop_graph_as_msg_create_from_json(ten_json_t *json,
                                                          ten_error_t *err) {
  TEN_ASSERT(
      json && ten_msg_json_get_msg_type(json) == TEN_MSG_TYPE_CMD_STOP_GRAPH,
      "Should not happen.");

  return (ten_msg_t *)ten_raw_cmd_stop_graph_create_from_json(json, err);
}

ten_json_t *ten_raw_cmd_stop_graph_to_json(ten_msg_t *self, ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_get_type((ten_msg_t *)self) ==
                         TEN_MSG_TYPE_CMD_STOP_GRAPH,
             "Should not happen.");

  ten_json_t *json = ten_json_create_object();
  TEN_ASSERT(json, "Should not happen.");

  for (size_t i = 0; i < ten_cmd_stop_graph_fields_info_size; ++i) {
    ten_msg_put_field_to_json_func_t put_field_to_json =
        ten_cmd_stop_graph_fields_info[i].put_field_to_json;
    if (put_field_to_json) {
      if (!put_field_to_json(
              &(((ten_cmd_stop_graph_t *)self)->cmd_hdr.cmd_base_hdr.msg_hdr),
              json, err)) {
        ten_json_destroy(json);
        return NULL;
      }
    }
  }

  return json;
}

ten_string_t *ten_raw_cmd_stop_graph_get_graph_name(
    ten_cmd_stop_graph_t *self) {
  TEN_ASSERT(self && ten_raw_cmd_check_integrity((ten_cmd_t *)self) &&
                 ten_raw_msg_get_type((ten_msg_t *)self) ==
                     TEN_MSG_TYPE_CMD_STOP_GRAPH,
             "Should not happen.");

  return &self->graph_name;
}

ten_string_t *ten_cmd_stop_graph_get_graph_name(ten_shared_ptr_t *self) {
  TEN_ASSERT(self && ten_msg_get_type(self) == TEN_MSG_TYPE_CMD_STOP_GRAPH,
             "Should not happen.");

  return ten_raw_cmd_stop_graph_get_graph_name(get_raw_cmd(self));
}
