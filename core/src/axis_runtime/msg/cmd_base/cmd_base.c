//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/msg/cmd_base/cmd_base.h"

#include <string.h>
#include <sys/types.h>

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/connection/connection.h"
#include "include_internal/axis_runtime/msg/cmd_base/field/field_info.h"
#include "include_internal/axis_runtime/msg/field/field_info.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/remote/remote.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/lib/uuid.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_get.h"

bool axis_raw_cmd_base_check_integrity(axis_cmd_base_t *self) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_CMD_BASE_SIGNATURE) {
    return false;
  }

  if (!axis_raw_msg_is_cmd_and_result(&self->msg_hdr)) {
    return false;
  }

  return true;
}

axis_cmd_base_t *axis_cmd_base_get_raw_cmd_base(axis_shared_ptr_t *self) {
  axis_ASSERT(self, "Should not happen.");
  return (axis_cmd_base_t *)axis_shared_ptr_get_data(self);
}

bool axis_cmd_base_check_integrity(axis_shared_ptr_t *self) {
  axis_ASSERT(self, "Should not happen.");

  return axis_raw_cmd_base_check_integrity(axis_cmd_base_get_raw_cmd_base(self));
}

static void axis_raw_cmd_base_init_empty(axis_cmd_base_t *self) {
  axis_ASSERT(self, "Should not happen.");

  axis_raw_msg_init(&self->msg_hdr, axis_MSG_TYPE_INVALID);

  axis_signature_set(&self->signature, (axis_signature_t)axis_CMD_BASE_SIGNATURE);

  axis_string_init(&self->parent_cmd_id);
  axis_value_init_string(&self->cmd_id);
  axis_value_init_string(&self->seq_id);

  self->original_connection = NULL;

  self->result_handler = NULL;
  self->result_handler_data = NULL;
}

void axis_raw_cmd_base_init(axis_cmd_base_t *self, axis_MSG_TYPE type) {
  axis_ASSERT(self, "Should not happen.");

  axis_raw_cmd_base_init_empty(self);

  self->msg_hdr.type = type;

  switch (type) {
    case axis_MSG_TYPE_CMD_START_GRAPH:
      axis_string_set_formatted(axis_value_peek_string(&self->msg_hdr.name), "%s",
                               axis_STR_MSG_NAME_axis_START_GRAPH);
      break;

    case axis_MSG_TYPE_CMD_TIMEOUT:
      axis_string_set_formatted(axis_value_peek_string(&self->msg_hdr.name), "%s",
                               axis_STR_MSG_NAME_axis_TIMEOUT);
      break;

    case axis_MSG_TYPE_CMD_TIMER:
      axis_string_set_formatted(axis_value_peek_string(&self->msg_hdr.name), "%s",
                               axis_STR_MSG_NAME_axis_TIMER);
      break;

    case axis_MSG_TYPE_CMD_STOP_GRAPH:
      axis_string_set_formatted(axis_value_peek_string(&self->msg_hdr.name), "%s",
                               axis_STR_MSG_NAME_axis_STOP_GRAPH);
      break;

    case axis_MSG_TYPE_CMD_CLOSE_APP:
      axis_string_set_formatted(axis_value_peek_string(&self->msg_hdr.name), "%s",
                               axis_STR_MSG_NAME_axis_CLOSE_APP);
      break;

    case axis_MSG_TYPE_CMD_RESULT:
      axis_string_set_formatted(axis_value_peek_string(&self->msg_hdr.name), "%s",
                               axis_STR_MSG_NAME_axis_RESULT);
      break;

    default:
      break;
  }
}

void axis_raw_cmd_base_deinit(axis_cmd_base_t *self) {
  axis_ASSERT(self && axis_raw_cmd_base_check_integrity(self),
             "Should not happen.");

  axis_signature_set(&self->signature, 0);

  axis_raw_msg_deinit(&self->msg_hdr);

  axis_value_deinit(&self->cmd_id);
  axis_value_deinit(&self->seq_id);

  axis_string_deinit(&self->parent_cmd_id);

  self->original_connection = NULL;
}

void axis_raw_cmd_base_copy_field(axis_msg_t *self, axis_msg_t *src,
                                 axis_list_t *excluded_field_ids) {
  axis_ASSERT(
      src && axis_raw_cmd_base_check_integrity((axis_cmd_base_t *)src) && self,
      "Should not happen.");

  for (size_t i = 0; i < axis_cmd_base_fields_info_size; ++i) {
    if (excluded_field_ids) {
      bool skip = false;

      axis_list_foreach (excluded_field_ids, iter) {
        if (axis_cmd_base_fields_info[i].field_id ==
            axis_int32_listnode_get(iter.node)) {
          skip = true;
          break;
        }
      }

      if (skip) {
        continue;
      }
    }

    axis_msg_copy_field_func_t copy_field =
        axis_cmd_base_fields_info[i].copy_field;
    if (copy_field) {
      copy_field(self, src, excluded_field_ids);
    }
  }
}

bool axis_raw_cmd_base_process_field(axis_msg_t *self,
                                    axis_raw_msg_process_one_field_func_t cb,
                                    void *user_data, axis_error_t *err) {
  axis_ASSERT(
      self && axis_raw_cmd_base_check_integrity((axis_cmd_base_t *)self) && cb,
      "Should not happen.");

  for (size_t i = 0; i < axis_cmd_base_fields_info_size; ++i) {
    axis_msg_process_field_func_t process_field =
        axis_cmd_base_fields_info[i].process_field;
    if (process_field) {
      if (!process_field(self, cb, user_data, err)) {
        return false;
      }
    }
  }

  return true;
}

static axis_string_t *axis_raw_cmd_base_gen_cmd_id_if_empty(
    axis_cmd_base_t *self) {
  axis_ASSERT(self && axis_raw_cmd_base_check_integrity(self),
             "Should not happen.");

  if (axis_string_is_empty(axis_value_peek_string(&self->cmd_id))) {
    axis_uuid4_gen_string(axis_value_peek_string(&self->cmd_id));
  }

  return axis_value_peek_string(&self->cmd_id);
}

axis_string_t *axis_cmd_base_gen_cmd_id_if_empty(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_msg_is_cmd_and_result(self), "Should not happen.");
  return axis_raw_cmd_base_gen_cmd_id_if_empty(
      axis_cmd_base_get_raw_cmd_base(self));
}

const char *axis_raw_cmd_base_gen_new_cmd_id_forcibly(axis_cmd_base_t *self) {
  axis_ASSERT(self && axis_raw_cmd_base_check_integrity(self),
             "Should not happen.");

  axis_string_t *cmd_id = axis_value_peek_string(&self->cmd_id);

  axis_string_clear(cmd_id);
  axis_uuid4_gen_string(cmd_id);

  return axis_string_get_raw_str(cmd_id);
}

const char *axis_cmd_base_gen_new_cmd_id_forcibly(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_msg_is_cmd_and_result(self), "Should not happen.");
  return axis_raw_cmd_base_gen_new_cmd_id_forcibly(
      axis_cmd_base_get_raw_cmd_base(self));
}

void axis_raw_cmd_base_set_cmd_id(axis_cmd_base_t *self, const char *cmd_id) {
  axis_ASSERT(self && axis_raw_cmd_base_check_integrity(self) && cmd_id,
             "Should not happen.");
  axis_string_set_formatted(axis_value_peek_string(&self->cmd_id), "%s", cmd_id);
}

axis_string_t *axis_raw_cmd_base_get_cmd_id(axis_cmd_base_t *self) {
  axis_ASSERT(self && axis_raw_cmd_base_check_integrity(self),
             "Should not happen.");
  return axis_value_peek_string(&self->cmd_id);
}

void axis_raw_cmd_base_save_cmd_id_to_parent_cmd_id(axis_cmd_base_t *self) {
  axis_ASSERT(self && axis_raw_cmd_base_check_integrity(self),
             "Should not happen.");

  axis_string_copy(&self->parent_cmd_id, axis_value_peek_string(&self->cmd_id));
}

void axis_cmd_base_save_cmd_id_to_parent_cmd_id(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_msg_is_cmd_and_result(self), "Should not happen.");
  axis_raw_cmd_base_save_cmd_id_to_parent_cmd_id(
      axis_cmd_base_get_raw_cmd_base(self));
}

void axis_raw_cmd_base_set_seq_id(axis_cmd_base_t *self, const char *seq_id) {
  axis_ASSERT(self && axis_raw_cmd_base_check_integrity(self) && seq_id,
             "Should not happen.");
  axis_string_set_formatted(axis_value_peek_string(&self->seq_id), "%s", seq_id);
}

static bool axis_raw_cmd_base_cmd_id_is_empty(axis_cmd_base_t *self) {
  axis_ASSERT(self && axis_raw_cmd_base_check_integrity(self),
             "Should not happen.");
  return axis_string_is_empty(axis_value_peek_string(&self->cmd_id));
}

bool axis_cmd_base_cmd_id_is_empty(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_cmd_base_check_integrity(self), "Should not happen.");
  return axis_raw_cmd_base_cmd_id_is_empty(axis_shared_ptr_get_data(self));
}

static axis_connection_t *axis_raw_cmd_base_get_origin_connection(
    axis_cmd_base_t *self) {
  axis_ASSERT(self && axis_raw_cmd_base_check_integrity(self),
             "Should not happen.");
  return self->original_connection;
}

axis_connection_t *axis_cmd_base_get_original_connection(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_cmd_base_check_integrity(self), "Should not happen.");
  return axis_raw_cmd_base_get_origin_connection(axis_shared_ptr_get_data(self));
}

static void axis_raw_cmd_base_set_origin_connection(
    axis_cmd_base_t *self, axis_connection_t *connection) {
  axis_ASSERT(self && axis_raw_cmd_base_check_integrity(self) && connection &&
                 axis_connection_check_integrity(connection, true),
             "Should not happen.");

  self->original_connection = connection;
}

void axis_cmd_base_set_original_connection(axis_shared_ptr_t *self,
                                          axis_connection_t *connection) {
  axis_ASSERT(self && axis_cmd_base_check_integrity(self) && connection,
             "Should not happen.");
  axis_raw_cmd_base_set_origin_connection(axis_shared_ptr_get_data(self),
                                         connection);
}

const char *axis_cmd_base_get_cmd_id(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_cmd_base_check_integrity(self), "Should not happen.");
  return axis_string_get_raw_str(
      axis_raw_cmd_base_get_cmd_id(axis_shared_ptr_get_data(self)));
}

static axis_string_t *axis_raw_cmd_base_get_parent_cmd_id(axis_cmd_base_t *self) {
  axis_ASSERT(self && axis_raw_cmd_base_check_integrity(self),
             "Should not happen.");
  return &self->parent_cmd_id;
}

const char *axis_cmd_base_get_parent_cmd_id(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_cmd_base_check_integrity(self), "Should not happen.");

  axis_string_t *parent_cmd_id =
      axis_raw_cmd_base_get_parent_cmd_id(axis_shared_ptr_get_data(self));
  if (axis_string_is_empty(parent_cmd_id)) {
    return NULL;
  } else {
    return axis_string_get_raw_str(parent_cmd_id);
  }
}

void axis_cmd_base_set_cmd_id(axis_shared_ptr_t *self, const char *cmd_id) {
  axis_ASSERT(self && axis_cmd_base_check_integrity(self), "Should not happen.");
  axis_raw_cmd_base_set_cmd_id(axis_shared_ptr_get_data(self), cmd_id);
}

static void axis_raw_cmd_base_reset_parent_cmd_id(axis_cmd_base_t *self) {
  axis_ASSERT(self && axis_raw_cmd_base_check_integrity(self),
             "Should not happen.");
  axis_string_clear(&self->parent_cmd_id);
}

void axis_cmd_base_reset_parent_cmd_id(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_cmd_base_check_integrity(self), "Should not happen.");
  axis_raw_cmd_base_reset_parent_cmd_id(axis_shared_ptr_get_data(self));
}

axis_string_t *axis_raw_cmd_base_get_seq_id(axis_cmd_base_t *self) {
  axis_ASSERT(self && axis_raw_cmd_base_check_integrity(self),
             "Should not happen.");
  return axis_value_peek_string(&self->seq_id);
}

const char *axis_cmd_base_get_seq_id(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_cmd_base_check_integrity(self), "Should not happen.");
  return axis_string_get_raw_str(
      axis_raw_cmd_base_get_seq_id(axis_shared_ptr_get_data(self)));
}

void axis_cmd_base_set_seq_id(axis_shared_ptr_t *self, const char *seq_id) {
  axis_ASSERT(self && axis_cmd_base_check_integrity(self), "Should not happen.");
  axis_raw_cmd_base_set_seq_id(axis_shared_ptr_get_data(self), seq_id);
}

static void axis_raw_cmd_base_set_result_handler(
    axis_cmd_base_t *self, axis_env_msg_result_handler_func_t result_handler,
    void *result_handler_data) {
  axis_ASSERT(self && axis_raw_cmd_base_check_integrity(self),
             "Should not happen.");

  self->result_handler = result_handler;
  self->result_handler_data = result_handler_data;
}

void axis_cmd_base_set_result_handler(
    axis_shared_ptr_t *self, axis_env_msg_result_handler_func_t result_handler,
    void *result_handler_data) {
  axis_ASSERT(self && axis_cmd_base_check_integrity(self), "Should not happen.");
  axis_raw_cmd_base_set_result_handler(axis_cmd_base_get_raw_cmd_base(self),
                                      result_handler, result_handler_data);
}

bool axis_cmd_base_comes_from_client_outside(axis_shared_ptr_t *self) {
  axis_ASSERT(
      self && axis_msg_check_integrity(self) && axis_msg_is_cmd_and_result(self),
      "Invalid argument.");

  const char *src_uri = axis_msg_get_src_app_uri(self);
  const char *cmd_id = axis_cmd_base_get_cmd_id(self);

  // The 'command ID' plays a critical role in the TEN world, so when TEN world
  // receives a command, no matter from where it receives, TEN runtime will
  // check if it contains a command ID, and assign a new command ID to it if
  // there is none in it.
  //
  // And that will give us a simple rule to determine if a command is coming
  // from the outside of the TEN world if the following assumption is true.
  //
  //    "When clients send a command to the TEN world, it can _not_
  //     specify the command ID of that command."
  //
  // Note: This is one of the few important assumptions and restrictions of the
  // TEN world.
  //
  // If the command is coming from outside of the TEN world, when that command
  // is coming to the TEN runtime, TEN runtime will assign a new command ID to
  // it, and set the source URI of that command to this command ID, in other
  // words, TEN runtime will use that command ID as the identity of the client.
  //
  // Therefore, it is a reliable way to determine if the command is coming
  // from the outside of the TEN world through checking if the src_uri and the
  // command ID of the command are equal.
  return axis_c_string_is_equal(src_uri, cmd_id);
}
