//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/msg/cmd_base/cmd/cmd.h"

#include <stdbool.h>
#include <stdlib.h>

#include "include_internal/axis_runtime/extension/extension_info/extension_info.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd_base.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd_result/cmd.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd_result/field/field_info.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/schema_store/cmd.h"
#include "include_internal/axis_runtime/schema_store/msg.h"
#include "include_internal/axis_runtime/schema_store/store.h"
#include "include_internal/axis_utils/value/value_set.h"
#include "axis_runtime/common/status_code.h"
#include "axis_runtime/msg/cmd_result/cmd_result.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/memory.h"
#include "axis_utils/value/value.h"

static bool axis_raw_cmd_result_check_integrity(axis_cmd_result_t *self) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_CMD_STATUS_SIGNATURE) {
    return false;
  }

  if (!axis_raw_msg_is_cmd_result(&self->cmd_base_hdr.msg_hdr)) {
    return false;
  }

  return true;
}

static bool axis_cmd_result_check_integrity(axis_shared_ptr_t *self) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_raw_cmd_result_check_integrity(axis_shared_ptr_get_data(self)) ==
      false) {
    return false;
  }
  return true;
}

static axis_cmd_result_t *axis_cmd_result_get_raw_cmd(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_cmd_base_check_integrity(self), "Should not happen.");

  return (axis_cmd_result_t *)axis_shared_ptr_get_data(self);
}

void axis_raw_cmd_result_destroy(axis_cmd_result_t *self) {
  axis_ASSERT(self, "Should not happen.");

  axis_raw_cmd_base_deinit(&self->cmd_base_hdr);
  axis_signature_set(&self->signature, 0);
  axis_string_deinit(axis_value_peek_string(&self->original_cmd_name));

  axis_FREE(self);
}

static axis_cmd_result_t *axis_raw_cmd_result_create_empty(void) {
  axis_cmd_result_t *raw_cmd = axis_MALLOC(sizeof(axis_cmd_result_t));
  axis_ASSERT(raw_cmd, "Failed to allocate memory.");

  axis_raw_cmd_base_init(&raw_cmd->cmd_base_hdr, axis_MSG_TYPE_CMD_RESULT);

  axis_signature_set(&raw_cmd->signature,
                    (axis_signature_t)axis_CMD_STATUS_SIGNATURE);

  axis_value_init_int32(&raw_cmd->status_code, axis_STATUS_CODE_INVALID);

  // We will get the original cmd type later.
  axis_value_init_int32(&raw_cmd->original_cmd_type, axis_MSG_TYPE_INVALID);
  axis_value_init_string(&raw_cmd->original_cmd_name);
  axis_value_init_bool(&raw_cmd->is_final, true);
  axis_value_init_bool(&raw_cmd->is_completed, true);

  return raw_cmd;
}

static axis_cmd_result_t *axis_raw_cmd_result_create(
    const axis_STATUS_CODE status_code) {
  axis_cmd_result_t *raw_cmd = axis_raw_cmd_result_create_empty();

  axis_value_set_int32(&raw_cmd->status_code, status_code);

  return raw_cmd;
}

axis_shared_ptr_t *axis_cmd_result_create(const axis_STATUS_CODE status_code) {
  return axis_shared_ptr_create(axis_raw_cmd_result_create(status_code),
                               axis_raw_cmd_result_destroy);
}

static axis_cmd_result_t *axis_raw_cmd_result_create_from_raw_cmd(
    const axis_STATUS_CODE status_code, axis_cmd_t *original_cmd) {
  axis_cmd_result_t *cmd = axis_raw_cmd_result_create(status_code);

  if (original_cmd) {
    // @{
    axis_raw_cmd_base_set_cmd_id(
        (axis_cmd_base_t *)cmd,
        axis_string_get_raw_str(
            axis_raw_cmd_base_get_cmd_id((axis_cmd_base_t *)original_cmd)));
    axis_raw_cmd_base_set_seq_id(
        (axis_cmd_base_t *)cmd,
        axis_string_get_raw_str(
            axis_raw_cmd_base_get_seq_id((axis_cmd_base_t *)original_cmd)));
    // @}

    axis_raw_cmd_result_set_original_cmd_type(
        cmd, axis_raw_msg_get_type((axis_msg_t *)original_cmd));

    // There are only 2 possible values of destination count of
    // 'original_cmd':
    // - 0
    //   The original_cmd is sent to a extension, and the cmd result is
    //   generated from that extension. Because TEN runtime would clear the
    //   destination locations of the original_cmd, the destination count
    //   would therefore be 0.
    // - 1
    //   In all other situations, the destination count of original_cmd should
    //   be 1, and the source location of the cmd result would be that
    //   destination of the original_cmd, therefore, we handle it here.
    axis_ASSERT(axis_raw_msg_get_dest_cnt((axis_msg_t *)original_cmd) <= 1,
               "Should not happen.");
    if (axis_raw_msg_get_dest_cnt((axis_msg_t *)original_cmd) == 1) {
      axis_loc_t *dest_loc =
          axis_raw_msg_get_first_dest_loc((axis_msg_t *)original_cmd);
      axis_raw_msg_set_src_to_loc((axis_msg_t *)cmd, dest_loc);
    }

    axis_raw_msg_clear_and_set_dest_to_loc(
        (axis_msg_t *)cmd, axis_raw_msg_get_src_loc((axis_msg_t *)original_cmd));
  }

  return cmd;
}

static axis_cmd_result_t *axis_raw_cmd_result_create_from_cmd(
    const axis_STATUS_CODE status_code, axis_shared_ptr_t *original_cmd) {
  return axis_raw_cmd_result_create_from_raw_cmd(
      status_code, original_cmd ? axis_shared_ptr_get_data(original_cmd) : NULL);
}

axis_shared_ptr_t *axis_cmd_result_create_from_cmd(
    const axis_STATUS_CODE status_code, axis_shared_ptr_t *original_cmd) {
  return axis_shared_ptr_create(
      axis_raw_cmd_result_create_from_cmd(status_code, original_cmd),
      axis_raw_cmd_result_destroy);
}

axis_STATUS_CODE axis_raw_cmd_result_get_status_code(axis_cmd_result_t *self) {
  axis_ASSERT(self && axis_raw_msg_get_type((axis_msg_t *)self) ==
                         axis_MSG_TYPE_CMD_RESULT,
             "Should not happen.");

  return axis_value_get_int32(&self->status_code, NULL);
}

axis_STATUS_CODE axis_cmd_result_get_status_code(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_msg_get_type(self) == axis_MSG_TYPE_CMD_RESULT,
             "Should not happen.");

  return axis_raw_cmd_result_get_status_code(axis_shared_ptr_get_data(self));
}

bool axis_raw_cmd_result_set_final(axis_cmd_result_t *self, bool is_final,
                                  axis_UNUSED axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_get_type((axis_msg_t *)self) ==
                         axis_MSG_TYPE_CMD_RESULT,
             "Should not happen.");

  axis_value_set_bool(&self->is_final, is_final);

  return true;
}

bool axis_raw_cmd_result_set_completed(axis_cmd_result_t *self, bool is_completed,
                                      axis_UNUSED axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_get_type((axis_msg_t *)self) ==
                         axis_MSG_TYPE_CMD_RESULT,
             "Should not happen.");

  axis_value_set_bool(&self->is_completed, is_completed);

  return true;
}

bool axis_cmd_result_set_completed(axis_shared_ptr_t *self, bool is_completed,
                                  axis_error_t *err) {
  axis_ASSERT(self && axis_msg_get_type(self) == axis_MSG_TYPE_CMD_RESULT,
             "Should not happen.");

  return axis_raw_cmd_result_set_completed(
      (axis_cmd_result_t *)axis_msg_get_raw_msg(self), is_completed, err);
}

bool axis_cmd_result_set_final(axis_shared_ptr_t *self, bool is_final,
                              axis_error_t *err) {
  axis_ASSERT(self && axis_msg_get_type(self) == axis_MSG_TYPE_CMD_RESULT,
             "Should not happen.");

  return axis_raw_cmd_result_set_final(
      (axis_cmd_result_t *)axis_msg_get_raw_msg(self), is_final, err);
}

bool axis_raw_cmd_result_is_final(axis_cmd_result_t *self, axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_get_type((axis_msg_t *)self) ==
                         axis_MSG_TYPE_CMD_RESULT,
             "Should not happen.");

  return axis_value_get_bool(&self->is_final, err);
}

bool axis_raw_cmd_result_is_completed(axis_cmd_result_t *self, axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_get_type((axis_msg_t *)self) ==
                         axis_MSG_TYPE_CMD_RESULT,
             "Should not happen.");

  return axis_value_get_bool(&self->is_completed, err);
}

bool axis_cmd_result_is_final(axis_shared_ptr_t *self, axis_error_t *err) {
  axis_ASSERT(self && axis_msg_get_type(self) == axis_MSG_TYPE_CMD_RESULT,
             "Should not happen.");

  return axis_raw_cmd_result_is_final(
      (axis_cmd_result_t *)axis_msg_get_raw_msg(self), err);
}

bool axis_cmd_result_is_completed(axis_shared_ptr_t *self, axis_error_t *err) {
  axis_ASSERT(self && axis_msg_get_type(self) == axis_MSG_TYPE_CMD_RESULT,
             "Should not happen.");

  return axis_raw_cmd_result_is_completed(
      (axis_cmd_result_t *)axis_msg_get_raw_msg(self), err);
}

void axis_raw_cmd_result_set_status_code(axis_cmd_result_t *self,
                                        axis_STATUS_CODE status_code) {
  axis_ASSERT(self && axis_raw_cmd_result_check_integrity(self),
             "Invalid argument.");
  axis_ASSERT(status_code != axis_STATUS_CODE_INVALID, "Invalid argument.");

  axis_value_set_int32(&self->status_code, status_code);
}

void axis_cmd_result_set_status_code(axis_shared_ptr_t *self,
                                    axis_STATUS_CODE status_code) {
  axis_ASSERT(self && axis_cmd_result_check_integrity(self), "Invalid argument.");
  axis_ASSERT(status_code != axis_STATUS_CODE_INVALID, "Invalid argument.");

  axis_cmd_result_t *cmd_result = axis_cmd_result_get_raw_cmd(self);
  axis_raw_cmd_result_set_status_code(cmd_result, status_code);
}

static axis_json_t *axis_raw_cmd_result_put_field_to_json(axis_cmd_result_t *self,
                                                        axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_get_type((axis_msg_t *)self) ==
                         axis_MSG_TYPE_CMD_RESULT,
             "Should not happen.");

  axis_json_t *json = axis_json_create_object();
  axis_ASSERT(json, "Should not happen.");

  if (!axis_raw_cmd_result_loop_all_fields(
          (axis_msg_t *)self, axis_raw_msg_put_one_field_to_json, json, err)) {
    axis_json_destroy(json);
    return NULL;
  }

  return json;
}

axis_json_t *axis_cmd_result_to_json(axis_shared_ptr_t *self, axis_error_t *err) {
  axis_ASSERT(self && axis_msg_get_type(self) == axis_MSG_TYPE_CMD_RESULT,
             "Should not happen.");
  return axis_raw_cmd_result_put_field_to_json(axis_shared_ptr_get_data(self),
                                              err);
}

void axis_raw_cmd_result_set_original_cmd_type(axis_cmd_result_t *self,
                                              axis_MSG_TYPE type) {
  axis_ASSERT(self && axis_raw_msg_get_type((axis_msg_t *)self) ==
                         axis_MSG_TYPE_CMD_RESULT,
             "Should not happen.");
  axis_value_set_int32(&self->original_cmd_type, type);
}

axis_MSG_TYPE axis_raw_cmd_result_get_original_cmd_type(axis_cmd_result_t *self) {
  axis_ASSERT(self && axis_raw_msg_get_type((axis_msg_t *)self) ==
                         axis_MSG_TYPE_CMD_RESULT,
             "Should not happen.");

  return axis_value_get_int32(&self->original_cmd_type, NULL);
}

void axis_cmd_result_set_original_cmd_type(axis_shared_ptr_t *self,
                                          axis_MSG_TYPE type) {
  axis_ASSERT(self && axis_msg_get_type(self) == axis_MSG_TYPE_CMD_RESULT,
             "Should not happen.");
  axis_raw_cmd_result_set_original_cmd_type(axis_cmd_result_get_raw_cmd(self),
                                           type);
}

axis_MSG_TYPE axis_cmd_result_get_original_cmd_type(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_msg_get_type(self) == axis_MSG_TYPE_CMD_RESULT,
             "Should not happen.");
  return axis_raw_cmd_result_get_original_cmd_type(
      axis_cmd_result_get_raw_cmd(self));
}

axis_msg_t *axis_raw_cmd_result_as_msg_clone(
    axis_msg_t *self, axis_UNUSED axis_list_t *excluded_field_ids) {
  axis_ASSERT(self && axis_raw_cmd_base_check_integrity((axis_cmd_base_t *)self),
             "Should not happen.");

  axis_cmd_result_t *cloned_cmd = axis_raw_cmd_result_create_empty();

  for (size_t i = 0; i < axis_cmd_result_fields_info_size; ++i) {
    axis_msg_copy_field_func_t copy_field =
        axis_cmd_result_fields_info[i].copy_field;
    if (copy_field) {
      copy_field((axis_msg_t *)cloned_cmd, self, NULL);
    }
  }

  return (axis_msg_t *)cloned_cmd;
}

bool axis_raw_cmd_result_validate_schema(axis_msg_t *status_msg,
                                        axis_schema_store_t *schema_store,
                                        bool is_msg_out, axis_error_t *err) {
  axis_ASSERT(status_msg && axis_raw_msg_check_integrity(status_msg),
             "Invalid argument.");
  axis_ASSERT(axis_raw_msg_get_type(status_msg) == axis_MSG_TYPE_CMD_RESULT,
             "Invalid argument.");
  axis_ASSERT(schema_store && axis_schema_store_check_integrity(schema_store),
             "Invalid argument.");
  axis_ASSERT(err && axis_error_check_integrity(err), "Invalid argument.");

  axis_cmd_result_t *cmd_result = (axis_cmd_result_t *)status_msg;
  axis_ASSERT(cmd_result && axis_raw_cmd_result_check_integrity(cmd_result),
             "Invalid argument.");

  const char *original_cmd_name =
      axis_value_peek_raw_str(&cmd_result->original_cmd_name, err);
  axis_ASSERT(original_cmd_name && strlen(original_cmd_name),
             "Invalid argument.");

  bool result = true;

  do {
    // The status `out` is responding to the cmd `in`, ex: we will call
    // `return_status` in `on_cmd`. The schema of status `out` defines within
    // the corresponding cmd `in`. So we need to reverse the `is_msg_out` here.
    axis_msg_schema_t *original_msg_schema = axis_schema_store_get_msg_schema(
        schema_store, axis_raw_msg_get_type(status_msg), original_cmd_name,
        !is_msg_out);
    if (!original_msg_schema) {
      break;
    }

    axis_cmd_schema_t *original_cmd_schema =
        (axis_cmd_schema_t *)original_msg_schema;
    axis_ASSERT(original_cmd_schema &&
                   axis_cmd_schema_check_integrity(original_cmd_schema),
               "Invalid argument.");

    if (!axis_cmd_schema_adjust_cmd_result_properties(
            original_cmd_schema, &status_msg->properties, err)) {
      result = false;
      break;
    }

    if (!axis_cmd_schema_validate_cmd_result_properties(
            original_cmd_schema, &status_msg->properties, err)) {
      result = false;
      break;
    }
  } while (0);

  return result;
}

static void axis_raw_cmd_result_set_original_cmd_name(
    axis_cmd_result_t *self, const char *original_cmd_name) {
  axis_ASSERT(self && axis_raw_cmd_result_check_integrity(self),
             "Invalid argument.");
  axis_ASSERT(original_cmd_name && strlen(original_cmd_name),
             "Invalid argument.");

  axis_string_set_from_c_str(axis_value_peek_string(&self->original_cmd_name),
                            original_cmd_name, strlen(original_cmd_name));
}

void axis_cmd_result_set_original_cmd_name(axis_shared_ptr_t *self,
                                          const char *original_cmd_name) {
  axis_ASSERT(self && axis_cmd_result_check_integrity(self), "Invalid argument.");
  axis_ASSERT(original_cmd_name && strlen(original_cmd_name),
             "Invalid argument.");

  axis_raw_cmd_result_set_original_cmd_name(
      (axis_cmd_result_t *)axis_msg_get_raw_msg(self), original_cmd_name);
}

bool axis_raw_cmd_result_loop_all_fields(axis_msg_t *self,
                                        axis_raw_msg_process_one_field_func_t cb,
                                        void *user_data, axis_error_t *err) {
  axis_ASSERT(
      self && axis_raw_cmd_base_check_integrity((axis_cmd_base_t *)self) && cb,
      "Should not happen.");

  for (size_t i = 0; i < axis_cmd_result_fields_info_size; ++i) {
    axis_msg_process_field_func_t process_field =
        axis_cmd_result_fields_info[i].process_field;
    if (process_field) {
      if (!process_field(self, cb, user_data, err)) {
        return false;
      }
    }
  }

  return true;
}
