//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/msg/cmd_base/cmd/cmd.h"

#include <stdlib.h>
#include <string.h>

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/custom/cmd.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/custom/field/field_info.h"
#include "include_internal/axis_runtime/msg/field/field_info.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_utils/value/value_path.h"
#include "include_internal/axis_utils/value/value_set.h"
#include "axis_utils/container/list.h"
#include "axis_utils/container/list_node.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/value/value.h"

static void axis_raw_cmd_custom_destroy(axis_cmd_t *self) {
  axis_ASSERT(
      self && axis_raw_msg_get_type((axis_msg_t *)self) == axis_MSG_TYPE_CMD,
      "Should not happen.");

  axis_raw_cmd_deinit(self);
  axis_FREE(self);
}

void axis_raw_cmd_custom_as_msg_destroy(axis_msg_t *self) {
  axis_ASSERT(self && axis_raw_msg_get_type(self) == axis_MSG_TYPE_CMD,
             "Should not happen.");

  axis_raw_cmd_custom_destroy((axis_cmd_t *)self);
}

static axis_cmd_t *axis_raw_cmd_custom_create_empty(void) {
  axis_cmd_t *raw_cmd = axis_MALLOC(sizeof(axis_cmd_t));
  axis_ASSERT(raw_cmd, "Failed to allocate memory.");

  axis_raw_cmd_init(raw_cmd, axis_MSG_TYPE_CMD);

  return raw_cmd;
}

axis_shared_ptr_t *axis_cmd_custom_create_empty(void) {
  return axis_shared_ptr_create(axis_raw_cmd_custom_create_empty(),
                               axis_raw_cmd_custom_destroy);
}

axis_cmd_t *axis_raw_cmd_custom_create(const char *name, axis_error_t *err) {
  axis_ASSERT(name, "Should not happen.");

  axis_cmd_t *cmd = axis_raw_cmd_custom_create_empty();
  axis_ASSERT(cmd && axis_raw_cmd_check_integrity(cmd), "Should not happen.");

  axis_raw_msg_set_name((axis_msg_t *)cmd, name, err);

  return cmd;
}

static axis_cmd_t *axis_raw_cmd_custom_create_with_name_len(const char *name,
                                                          size_t name_len,
                                                          axis_error_t *err) {
  axis_ASSERT(name, "Should not happen.");

  axis_cmd_t *cmd = axis_raw_cmd_custom_create_empty();
  axis_ASSERT(cmd && axis_raw_cmd_check_integrity(cmd), "Should not happen.");

  axis_raw_msg_set_name_with_len((axis_msg_t *)cmd, name, name_len, err);

  return cmd;
}

axis_shared_ptr_t *axis_cmd_custom_create(const char *name, axis_error_t *err) {
  axis_ASSERT(name, "Should not happen.");
  return axis_shared_ptr_create(axis_raw_cmd_custom_create(name, err),
                               axis_raw_cmd_custom_destroy);
}

axis_shared_ptr_t *axis_cmd_custom_create_with_name_len(const char *name,
                                                      size_t name_len,
                                                      axis_error_t *err) {
  axis_ASSERT(name, "Should not happen.");
  return axis_shared_ptr_create(
      axis_raw_cmd_custom_create_with_name_len(name, name_len, err),
      axis_raw_cmd_custom_destroy);
}

axis_json_t *axis_raw_cmd_custom_to_json(axis_msg_t *self, axis_error_t *err) {
  axis_ASSERT(self && axis_raw_cmd_check_integrity((axis_cmd_t *)self) &&
                 axis_raw_msg_get_type(self) == axis_MSG_TYPE_CMD,
             "Should not happen.");

  axis_json_t *json = axis_json_create_object();
  axis_ASSERT(json, "Should not happen.");

  if (!axis_raw_cmd_custom_loop_all_fields(
          self, axis_raw_msg_put_one_field_to_json, json, err)) {
    axis_json_destroy(json);
    return NULL;
  }

  return json;
}

axis_msg_t *axis_raw_cmd_custom_as_msg_clone(axis_msg_t *self,
                                           axis_list_t *excluded_field_ids) {
  axis_ASSERT(self && axis_raw_cmd_base_check_integrity((axis_cmd_base_t *)self) &&
                 axis_raw_msg_get_type(self) == axis_MSG_TYPE_CMD,
             "Should not happen.");

  axis_cmd_t *new_cmd = axis_MALLOC(sizeof(axis_cmd_t));
  axis_ASSERT(new_cmd, "Failed to allocate memory.");

  axis_raw_cmd_init(new_cmd, axis_MSG_TYPE_CMD);

  for (size_t i = 0; i < axis_cmd_custom_fields_info_size; ++i) {
    if (excluded_field_ids) {
      bool skip = false;

      axis_list_foreach (excluded_field_ids, iter) {
        if (axis_cmd_custom_fields_info[i].field_id ==
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
        axis_cmd_custom_fields_info[i].copy_field;
    if (copy_field) {
      copy_field((axis_msg_t *)new_cmd, self, excluded_field_ids);
    }
  }

  return (axis_msg_t *)new_cmd;
}

bool axis_raw_cmd_custom_set_axis_property(axis_msg_t *self, axis_list_t *paths,
                                         axis_value_t *value, axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Invalid argument.");
  axis_ASSERT(paths && axis_list_check_integrity(paths),
             "path should not be empty.");
  axis_ASSERT(value && axis_value_check_integrity(value), "Invalid argument.");

  bool success = true;

  axis_error_t tmp_err;
  bool use_tmp_err = false;
  if (!err) {
    use_tmp_err = true;
    axis_error_init(&tmp_err);
    err = &tmp_err;
  }

  axis_list_foreach (paths, item_iter) {
    axis_value_path_item_t *item = axis_ptr_listnode_get(item_iter.node);
    axis_ASSERT(item, "Invalid argument.");

    switch (item->type) {
      case axis_VALUE_PATH_ITEM_TYPE_OBJECT_ITEM: {
        if (!strcmp(axis_STR_NAME,
                    axis_string_get_raw_str(&item->obj_item_str))) {
          if (axis_value_is_string(value)) {
            axis_value_set_string_with_size(
                &self->name, axis_value_peek_raw_str(value, &tmp_err),
                strlen(axis_value_peek_raw_str(value, &tmp_err)));
            success = true;
          } else {
            success = false;
          }
        }
        break;
      }

      default:
        break;
    }
  }

  if (use_tmp_err) {
    axis_error_deinit(&tmp_err);
  }

  return success;
}

bool axis_raw_cmd_custom_loop_all_fields(axis_msg_t *self,
                                        axis_raw_msg_process_one_field_func_t cb,
                                        void *user_data, axis_error_t *err) {
  axis_ASSERT(self && axis_raw_cmd_check_integrity((axis_cmd_t *)self) && cb,
             "Should not happen.");

  for (size_t i = 0; i < axis_cmd_custom_fields_info_size; ++i) {
    axis_msg_process_field_func_t process_field =
        axis_cmd_custom_fields_info[i].process_field;
    if (process_field) {
      if (!process_field(self, cb, user_data, err)) {
        return false;
      }
    }
  }

  return true;
}
