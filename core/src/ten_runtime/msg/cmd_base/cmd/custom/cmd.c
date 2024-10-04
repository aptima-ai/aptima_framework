//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "ten_runtime/msg/cmd/custom/cmd.h"

#include <stdlib.h>
#include <string.h>

#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd/cmd.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd/custom/cmd.h"
#include "include_internal/ten_runtime/msg/cmd_base/cmd/custom/field/field_info.h"
#include "include_internal/ten_runtime/msg/field/field_info.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "include_internal/ten_utils/value/value_path.h"
#include "ten_runtime/common/errno.h"
#include "ten_utils/container/list.h"
#include "ten_utils/container/list_node.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/lib/smart_ptr.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"
#include "ten_utils/value/value.h"

static void ten_raw_cmd_custom_destroy(ten_cmd_t *self) {
  TEN_ASSERT(
      self && ten_raw_msg_get_type((ten_msg_t *)self) == TEN_MSG_TYPE_CMD,
      "Should not happen.");

  ten_raw_cmd_deinit(self);
  TEN_FREE(self);
}

void ten_raw_cmd_custom_as_msg_destroy(ten_msg_t *self) {
  TEN_ASSERT(self && ten_raw_msg_get_type(self) == TEN_MSG_TYPE_CMD,
             "Should not happen.");

  ten_raw_cmd_custom_destroy((ten_cmd_t *)self);
}

static ten_cmd_t *ten_raw_cmd_custom_create_empty(void) {
  ten_cmd_t *raw_cmd = TEN_MALLOC(sizeof(ten_cmd_t));
  TEN_ASSERT(raw_cmd, "Failed to allocate memory.");

  ten_raw_cmd_init(raw_cmd, TEN_MSG_TYPE_CMD);

  return raw_cmd;
}

ten_shared_ptr_t *ten_cmd_custom_create_empty(void) {
  return ten_shared_ptr_create(ten_raw_cmd_custom_create_empty(),
                               ten_raw_cmd_custom_destroy);
}

ten_cmd_t *ten_raw_cmd_custom_create(const char *cmd_name) {
  TEN_ASSERT(cmd_name, "Should not happen.");

  ten_cmd_t *cmd = ten_raw_cmd_custom_create_empty();
  TEN_ASSERT(cmd && ten_raw_cmd_check_integrity(cmd), "Should not happen.");

  ten_raw_msg_set_name((ten_msg_t *)cmd, cmd_name, NULL);

  return cmd;
}

ten_shared_ptr_t *ten_cmd_custom_create(const char *cmd_name) {
  TEN_ASSERT(cmd_name, "Should not happen.");

  ten_cmd_t *cmd = ten_raw_cmd_custom_create(cmd_name);
  return ten_shared_ptr_create(cmd, ten_raw_cmd_custom_destroy);
}

static bool ten_raw_cmd_custom_init_from_json(ten_cmd_t *self, ten_json_t *json,
                                              TEN_UNUSED ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_cmd_check_integrity((ten_cmd_t *)self),
             "Should not happen.");
  TEN_ASSERT(json && ten_json_check_integrity(json), "Should not happen.");

  for (size_t i = 0; i < ten_cmd_custom_fields_info_size; ++i) {
    ten_msg_get_field_from_json_func_t get_field_from_json =
        ten_cmd_custom_fields_info[i].get_field_from_json;
    if (get_field_from_json) {
      if (!get_field_from_json((ten_msg_t *)self, json, err)) {
        return false;
      }
    }
  }

  return true;
}

bool ten_raw_cmd_custom_as_msg_init_from_json(ten_msg_t *self, ten_json_t *json,
                                              ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_cmd_check_integrity((ten_cmd_t *)self),
             "Should not happen.");
  TEN_ASSERT(json && ten_json_check_integrity(json), "Should not happen.");

  return ten_raw_cmd_custom_init_from_json((ten_cmd_t *)self, json, err);
}

ten_cmd_t *ten_raw_cmd_custom_create_from_json(ten_json_t *json,
                                               ten_error_t *err) {
  TEN_ASSERT(json, "Should not happen.");

  ten_cmd_t *cmd = ten_raw_cmd_custom_create_empty();
  TEN_ASSERT(cmd && ten_raw_cmd_check_integrity((ten_cmd_t *)cmd),
             "Should not happen.");

  if (!ten_raw_cmd_custom_init_from_json(cmd, json, err)) {
    ten_raw_cmd_custom_destroy(cmd);
    return NULL;
  }

  return cmd;
}

ten_msg_t *ten_raw_cmd_custom_as_msg_create_from_json(ten_json_t *json,
                                                      ten_error_t *err) {
  TEN_ASSERT(json, "Should not happen.");

  return (ten_msg_t *)ten_raw_cmd_custom_create_from_json(json, err);
}

// This hack is only used by msgpack when serializing/deserializing the connect
// command. Eventually, we should remove this hack.
static void ten_raw_cmd_custom_to_json_msgpack_serialization_hack(
    ten_msg_t *self, ten_json_t *json) {
  TEN_ASSERT(self && ten_raw_cmd_check_integrity((ten_cmd_t *)self) &&
                 ten_raw_msg_get_type(self) == TEN_MSG_TYPE_CMD,
             "Should not happen.");
  TEN_ASSERT(json, "Should not happen.");

  ten_value_t *json_value =
      ten_raw_msg_peek_property(self, TEN_STR_MSGPACK_SERIALIZATION_HACK, NULL);
  if (json_value) {
    // If there is a 'json' attached to this custom command, add all the
    // fields in that json to the json returned.

    ten_json_t *payload_json =
        ten_json_from_string(ten_value_peek_c_str(json_value), NULL);
    TEN_ASSERT(ten_json_check_integrity(payload_json), "Should not happen.");

    ten_json_object_update_missing(json, payload_json);

    if (ten_json_object_peek(payload_json, TEN_STR_NAME)) {
      ten_json_object_set_new(
          json, TEN_STR_NAME,
          ten_json_create_string(
              ten_json_object_peek_string(payload_json, TEN_STR_NAME)));
    }

    ten_json_destroy(payload_json);
  }
}

ten_json_t *ten_raw_cmd_custom_to_json(ten_msg_t *self, ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_cmd_check_integrity((ten_cmd_t *)self) &&
                 ten_raw_msg_get_type(self) == TEN_MSG_TYPE_CMD,
             "Should not happen.");

  ten_json_t *json = ten_json_create_object();
  TEN_ASSERT(json, "Should not happen.");

  for (size_t i = 0; i < ten_cmd_custom_fields_info_size; ++i) {
    ten_msg_put_field_to_json_func_t put_field_to_json =
        ten_cmd_custom_fields_info[i].put_field_to_json;
    if (put_field_to_json) {
      if (!put_field_to_json(self, json, err)) {
        ten_json_destroy(json);
        return NULL;
      }
    }
  }

  ten_raw_cmd_custom_to_json_msgpack_serialization_hack(self, json);

  return json;
}

ten_msg_t *ten_raw_cmd_custom_as_msg_clone(ten_msg_t *self,
                                           ten_list_t *excluded_field_ids) {
  TEN_ASSERT(self && ten_raw_cmd_base_check_integrity((ten_cmd_base_t *)self) &&
                 ten_raw_msg_get_type(self) == TEN_MSG_TYPE_CMD,
             "Should not happen.");

  ten_cmd_t *new_cmd = TEN_MALLOC(sizeof(ten_cmd_t));
  TEN_ASSERT(new_cmd, "Failed to allocate memory.");

  ten_raw_cmd_init(new_cmd, TEN_MSG_TYPE_CMD);

  for (size_t i = 0; i < ten_cmd_custom_fields_info_size; ++i) {
    if (excluded_field_ids) {
      bool skip = false;

      ten_list_foreach (excluded_field_ids, iter) {
        if (ten_cmd_custom_fields_info[i].field_id ==
            ten_int32_listnode_get(iter.node)) {
          skip = true;
          break;
        }
      }

      if (skip) {
        continue;
      }
    }

    ten_msg_copy_field_func_t copy_field =
        ten_cmd_custom_fields_info[i].copy_field;
    if (copy_field) {
      copy_field((ten_msg_t *)new_cmd, self, excluded_field_ids);
    }
  }

  return (ten_msg_t *)new_cmd;
}

bool ten_raw_cmd_custom_set_ten_property(ten_msg_t *self, ten_list_t *paths,
                                         ten_value_t *value, ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self), "Invalid argument.");
  TEN_ASSERT(paths && ten_list_check_integrity(paths),
             "path should not be empty.");
  TEN_ASSERT(value && ten_value_check_integrity(value), "Invalid argument.");

  bool success = true;

  ten_error_t tmp_err;
  bool use_tmp_err = false;
  if (!err) {
    use_tmp_err = true;
    ten_error_init(&tmp_err);
    err = &tmp_err;
  }

  ten_list_foreach (paths, item_iter) {
    ten_value_path_item_t *item = ten_ptr_listnode_get(item_iter.node);
    TEN_ASSERT(item, "Invalid argument.");

    switch (item->type) {
      case TEN_VALUE_PATH_ITEM_TYPE_OBJECT_ITEM: {
        if (!strcmp(TEN_STR_NAME,
                    ten_string_get_raw_str(&item->obj_item_str))) {
          if (ten_value_is_string(value)) {
            ten_string_init_from_c_str(&self->name,
                                       ten_value_peek_string(value),
                                       strlen(ten_value_peek_string(value)));
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
    ten_error_deinit(&tmp_err);
  }

  return success;
}

bool ten_raw_cmd_custom_check_type_and_name(ten_msg_t *self,
                                            const char *type_str,
                                            const char *name_str,
                                            ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self), "Invalid argument.");

  if (type_str) {
    if (strcmp(type_str, TEN_STR_CMD) != 0) {
      if (err) {
        ten_error_set(err, TEN_ERRNO_GENERIC,
                      "Incorrect message type for cmd: %s", type_str);
      }
      return false;
    }
  }

  if (name_str) {
    if (strncmp(name_str, TEN_STR_MSG_NAME_TEN_NAMESPACE_PREFIX,
                strlen(TEN_STR_MSG_NAME_TEN_NAMESPACE_PREFIX)) == 0) {
      if (err) {
        ten_error_set(err, TEN_ERRNO_GENERIC,
                      "Incorrect message name for cmd: %s", name_str);
      }
      return false;
    }
  }

  return true;
}
