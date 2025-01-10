//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//

#include "include_internal/axis_runtime/msg/field/properties.h"

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "axis_runtime/common/errno.h"
#include "axis_utils/container/list.h"
#include "axis_utils/container/list_node.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_kv.h"
#include "axis_utils/value/value_merge.h"

void axis_raw_msg_properties_copy(axis_msg_t *self, axis_msg_t *src,
                                 axis_UNUSED axis_list_t *excluded_field_ids) {
  axis_ASSERT(src && axis_raw_msg_check_integrity(src), "Should not happen.");
  axis_ASSERT(axis_list_size(axis_raw_msg_get_properties(self)) == 0,
             "Should not happen.");

  axis_value_object_merge_with_clone(&self->properties, &src->properties);
}

axis_list_t *axis_raw_msg_get_properties(axis_msg_t *self) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");

  axis_value_t *properties_value = &self->properties;
  axis_ASSERT(axis_value_is_object(properties_value), "Should not happen.");

  axis_list_t *properties = &properties_value->content.object;
  axis_ASSERT(properties && axis_list_check_integrity(properties),
             "Should not happen.");

  return properties;
}

static axis_list_t *axis_msg_get_properties(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");

  axis_msg_t *raw_msg = axis_msg_get_raw_msg(self);
  axis_ASSERT(raw_msg, "Should not happen.");

  return axis_raw_msg_get_properties(raw_msg);
}

static bool axis_raw_msg_is_property_exist(axis_msg_t *self, const char *path,
                                          axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(path && strlen(path), "path should not be empty.");

  if (!path || !strlen(path)) {
    return false;
  }

  return axis_raw_msg_peek_property(self, path, err) != NULL;
}

bool axis_msg_is_property_exist(axis_shared_ptr_t *self, const char *path,
                               axis_error_t *err) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(path && strlen(path), "path should not be empty.");

  if (!path || !strlen(path)) {
    if (err) {
      axis_error_set(err, axis_ERRNO_INVALID_ARGUMENT,
                    "path should not be empty.");
    }
    return false;
  }

  return axis_raw_msg_is_property_exist(axis_shared_ptr_get_data(self), path,
                                       err);
}

bool axis_msg_del_property(axis_shared_ptr_t *self, const char *path) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Should not happen.");
  axis_ASSERT(path && strlen(path), "path should not be empty.");

  if (!path || !strlen(path)) {
    return false;
  }

  axis_list_foreach (axis_msg_get_properties(self), iter) {
    axis_value_kv_t *kv = (axis_value_kv_t *)axis_ptr_listnode_get(iter.node);
    axis_ASSERT(kv, "Should not happen.");

    if (axis_string_is_equal_c_str(&kv->key, path)) {
      axis_list_remove_node(axis_msg_get_properties(self), iter.node);
      return true;
    }
  }

  return false;
}

bool axis_raw_msg_properties_process(axis_msg_t *self,
                                    axis_raw_msg_process_one_field_func_t cb,
                                    void *user_data, axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");

  axis_value_t *properties_value = &self->properties;
  axis_ASSERT(axis_value_is_object(properties_value), "Should not happen.");

  axis_msg_field_process_data_t properties_field;
  axis_msg_field_process_data_init(&properties_field, axis_STR_PROPERTIES,
                                  properties_value, true);

  bool rc = cb(self, &properties_field, user_data, err);

  // Note: The properties may be changed in the callback function.

  return rc;
}