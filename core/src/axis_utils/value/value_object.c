//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_runtime/common/errno.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_is.h"
#include "axis_utils/value/value_kv.h"

axis_value_t *axis_value_object_peek(axis_value_t *self, const char *key) {
  axis_ASSERT(self && axis_value_check_integrity(self) && key,
             "Invalid argument.");

  if (!axis_value_is_object(self)) {
    axis_ASSERT(0, "Invalid argument.");
    return NULL;
  }

  axis_list_foreach (&self->content.object, iter) {
    axis_value_kv_t *kv = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(kv && axis_value_kv_check_integrity(kv), "Invalid argument.");

    if (axis_string_is_equal_c_str(&kv->key, key)) {
      return kv->value;
    }
  }

  return NULL;
}

bool axis_value_object_get_bool(axis_value_t *self, const char *key,
                               axis_error_t *err) {
  axis_ASSERT(self && axis_value_check_integrity(self) && key,
             "Invalid argument.");

  axis_value_t *v = axis_value_object_peek(self, key);
  if (!v) {
    if (err) {
      axis_error_set(err, axis_ERRNO_GENERIC, "%s does not exist.", key);
    }
    return false;
  }

  bool result = axis_value_get_bool(v, err);
  bool success = axis_error_is_success(err);

  if (success) {
    return result;
  } else {
    if (err) {
      axis_error_set(err, axis_ERRNO_GENERIC,
                    "Failed to get boolean value from %s", key);
    }
    return false;
  }
}

const char *axis_value_object_peek_string(axis_value_t *self, const char *key) {
  axis_ASSERT(self && axis_value_check_integrity(self) && key,
             "Invalid argument.");

  axis_value_t *v = axis_value_object_peek(self, key);
  if (!v) {
    return NULL;
  }

  if (!axis_value_is_string(v)) {
    return NULL;
  }

  const char *result = axis_value_peek_raw_str(v, NULL);
  return result;
}

axis_list_t *axis_value_object_peek_array(axis_value_t *self, const char *key) {
  axis_ASSERT(self && axis_value_check_integrity(self) && key,
             "Invalid argument.");

  axis_value_t *v = axis_value_object_peek(self, key);
  if (!v) {
    return NULL;
  }

  if (!axis_value_is_array(v)) {
    return NULL;
  }

  return axis_value_peek_array(v);
}

bool axis_value_object_move(axis_value_t *self, const char *key,
                           axis_value_t *value) {
  if (!self || !key || !value) {
    axis_ASSERT(0, "Invalid argument.");
    return false;
  }

  if (!axis_value_is_object(self)) {
    axis_ASSERT(0, "Invalid argument.");
    return false;
  }

  bool found = false;

  axis_list_foreach (&self->content.object, iter) {
    axis_value_kv_t *kv = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(kv && axis_value_kv_check_integrity(kv), "Invalid argument.");

    if (axis_string_is_equal_c_str(&kv->key, key)) {
      found = true;

      if (kv->value) {
        axis_value_destroy(kv->value);
        kv->value = value;
      }
    }
  }

  if (!found) {
    axis_value_kv_t *kv = axis_value_kv_create(key, value);
    axis_ASSERT(kv, "Failed to create value_kv.");

    axis_list_push_ptr_back(
        &self->content.object, kv,
        (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);
  }

  return true;
}
