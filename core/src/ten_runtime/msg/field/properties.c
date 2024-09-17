//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//

#include "include_internal/ten_runtime/msg/field/properties.h"

#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "ten_utils/macro/check.h"
#include "ten_runtime/common/errno.h"
#include "ten_utils/container/list.h"
#include "ten_utils/container/list_node.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/lib/smart_ptr.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/macro/mark.h"
#include "ten_utils/value/value.h"
#include "ten_utils/value/value_merge.h"

bool ten_raw_msg_properties_to_json(ten_msg_t *self, ten_json_t *json,
                                    ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self) && json,
             "Should not happen.");

  ten_list_foreach (ten_raw_msg_get_properties(self), iter) {
    ten_value_kv_t *kv = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(kv, "Should not happen.");

    ten_json_object_set_new(json,
                            ten_string_get_raw_str(ten_value_kv_get_key(kv)),
                            ten_value_to_json(ten_value_kv_get_value(kv)));
  }

  return true;
}

bool ten_raw_msg_properties_from_json(ten_msg_t *self, ten_json_t *json,
                                      TEN_UNUSED ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self) && json,
             "Should not happen.");

  const char *key = NULL;
  ten_json_t *item = NULL;
  ten_json_object_foreach(json, key, item) {
    if (ten_c_string_is_equal(key, TEN_STR_UNDERLINE_TEN)) {
      // The "ten" section is reserved for internal usage.
      continue;
    }

    ten_list_push_ptr_back(
        ten_raw_msg_get_properties(self),
        ten_value_kv_create(key, ten_value_from_json(item)),
        (ten_ptr_listnode_destroy_func_t)ten_value_kv_destroy);
  }

  return true;
}

void ten_raw_msg_properties_copy(ten_msg_t *self, ten_msg_t *src,
                                 TEN_UNUSED ten_list_t *excluded_field_ids) {
  TEN_ASSERT(src && ten_raw_msg_check_integrity(src), "Should not happen.");
  TEN_ASSERT(ten_list_size(ten_raw_msg_get_properties(self)) == 0,
             "Should not happen.");

  ten_value_object_merge_with_clone(&self->properties, &src->properties);
}

ten_list_t *ten_raw_msg_get_properties(ten_msg_t *self) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self), "Should not happen.");

  ten_value_t *properties_value = &self->properties;
  TEN_ASSERT(ten_value_is_object(properties_value), "Should not happen.");

  ten_list_t *properties = &properties_value->content.object;
  TEN_ASSERT(properties && ten_list_check_integrity(properties),
             "Should not happen.");

  return properties;
}

static ten_list_t *ten_msg_get_properties(ten_shared_ptr_t *self) {
  TEN_ASSERT(self && ten_msg_check_integrity(self), "Should not happen.");

  ten_msg_t *raw_msg = ten_msg_get_raw_msg(self);
  TEN_ASSERT(raw_msg, "Should not happen.");

  return ten_raw_msg_get_properties(raw_msg);
}

static bool ten_raw_msg_is_property_exist(ten_msg_t *self, const char *path,
                                          ten_error_t *err) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(path && strlen(path), "path should not be empty.");

  if (!path || !strlen(path)) {
    return false;
  }

  return ten_raw_msg_peek_property(self, path, err) != NULL;
}

bool ten_msg_is_property_exist(ten_shared_ptr_t *self, const char *path,
                               ten_error_t *err) {
  TEN_ASSERT(self && ten_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(path && strlen(path), "path should not be empty.");

  if (!path || !strlen(path)) {
    if (err) {
      ten_error_set(err, TEN_ERRNO_INVALID_ARGUMENT,
                    "path should not be empty.");
    }
    return false;
  }

  return ten_raw_msg_is_property_exist(ten_shared_ptr_get_data(self), path,
                                       err);
}

bool ten_msg_del_property(ten_shared_ptr_t *self, const char *path) {
  TEN_ASSERT(self && ten_msg_check_integrity(self), "Should not happen.");
  TEN_ASSERT(path && strlen(path), "path should not be empty.");

  if (!path || !strlen(path)) {
    return false;
  }

  ten_list_foreach (ten_msg_get_properties(self), iter) {
    ten_value_kv_t *kv = (ten_value_kv_t *)ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(kv, "Should not happen.");

    if (ten_string_is_equal_c_str(&kv->key, path)) {
      ten_list_remove_node(ten_msg_get_properties(self), iter.node);
      return true;
    }
  }

  return false;
}
