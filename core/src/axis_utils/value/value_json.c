//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <stdint.h>
#include <stdlib.h>

#include "include_internal/axis_utils/value/value_set.h"
#include "axis_utils/container/list.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/log/log.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/value/type.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_get.h"
#include "axis_utils/value/value_is.h"
#include "axis_utils/value/value_kv.h"

bool axis_value_set_from_json(axis_value_t *self, axis_json_t *json) {
  axis_ASSERT(self && json, "Invalid argument.");

  if (!self || !json) {
    return false;
  }

  switch (axis_value_get_type(self)) {
    case axis_TYPE_INVALID:
    case axis_TYPE_NULL:
      if (axis_json_is_null(json)) {
        // Do nothing.
      }
      break;
    case axis_TYPE_BOOL:
      if (axis_json_is_boolean(json)) {
        return axis_value_set_bool(self, axis_json_get_boolean_value(json));
      }
      break;
    case axis_TYPE_INT8:
      if (axis_json_is_integer(json) &&
          axis_json_get_integer_value(json) >= INT8_MIN &&
          axis_json_get_integer_value(json) <= INT8_MAX) {
        return axis_value_set_int8(self,
                                  (int8_t)axis_json_get_integer_value(json));
      }
      break;
    case axis_TYPE_INT16:
      if (axis_json_is_integer(json) &&
          axis_json_get_integer_value(json) >= INT16_MIN &&
          axis_json_get_integer_value(json) <= INT16_MAX) {
        return axis_value_set_int16(self,
                                   (int16_t)axis_json_get_integer_value(json));
      }
      break;
    case axis_TYPE_INT32:
      if (axis_json_is_integer(json) &&
          axis_json_get_integer_value(json) >= INT32_MIN &&
          axis_json_get_integer_value(json) <= INT32_MAX) {
        return axis_value_set_int32(self,
                                   (int32_t)axis_json_get_integer_value(json));
      }
      break;
    case axis_TYPE_INT64:
      if (axis_json_is_integer(json)) {
        return axis_value_set_int64(self, axis_json_get_integer_value(json));
      }
      break;
    case axis_TYPE_UINT8:
      if (axis_json_is_integer(json) && axis_json_get_integer_value(json) >= 0 &&
          axis_json_get_integer_value(json) <= UINT8_MAX) {
        return axis_value_set_uint8(self,
                                   (uint8_t)axis_json_get_integer_value(json));
      }
      break;
    case axis_TYPE_UINT16:
      if (axis_json_is_integer(json) && axis_json_get_integer_value(json) >= 0 &&
          axis_json_get_integer_value(json) <= UINT16_MAX) {
        return axis_value_set_uint16(self,
                                    (uint16_t)axis_json_get_integer_value(json));
      }
      break;
    case axis_TYPE_UINT32:
      if (axis_json_is_integer(json) && axis_json_get_integer_value(json) >= 0 &&
          axis_json_get_integer_value(json) <= UINT32_MAX) {
        return axis_value_set_uint32(self,
                                    (uint32_t)axis_json_get_integer_value(json));
      }
      break;
    case axis_TYPE_UINT64:
      if (axis_json_is_integer(json) && axis_json_get_integer_value(json) >= 0) {
        return axis_value_set_uint64(self,
                                    (uint64_t)axis_json_get_integer_value(json));
      }
      break;
    case axis_TYPE_FLOAT32:
      if (axis_json_is_real(json)) {
        return axis_value_set_float32(self,
                                     (float)axis_json_get_real_value(json));
      }
      break;
    case axis_TYPE_FLOAT64:
      if (axis_json_is_real(json)) {
        return axis_value_set_float64(self, axis_json_get_real_value(json));
      }
      break;
    case axis_TYPE_STRING:
      if (axis_json_is_string(json)) {
        return axis_value_set_string(self, axis_json_peek_string_value(json));
      }
      break;
    case axis_TYPE_ARRAY:
      if (axis_json_is_array(json)) {
        // Loop each item in the JSON array and convert them to axis_value_t.
        axis_list_t array = axis_LIST_INIT_VAL;
        size_t i = 0;
        axis_json_t *item_json = NULL;
        axis_json_array_foreach(json, i, item_json) {
          axis_value_t *item = axis_value_from_json(item_json);
          axis_ASSERT(item && axis_value_check_integrity(item),
                     "Invalid argument.");

          if (!item) {
            // Something wrong, we should return false.
            axis_list_clear(&array);
            return false;
          }

          axis_list_push_ptr_back(
              &array, item, (axis_ptr_listnode_destroy_func_t)axis_value_destroy);
        }

        return axis_value_set_array_with_move(self, &array);
      }
      break;
    case axis_TYPE_OBJECT:
      if (axis_json_is_object(json)) {
        // Loop each item in the JSON object and convert them to axis_value_kv_t.
        axis_list_t object = axis_LIST_INIT_VAL;
        const char *key = NULL;
        axis_json_t *value_json = NULL;
        axis_json_object_foreach(json, key, value_json) {
          axis_value_kv_t *kv = axis_value_kv_from_json(key, value_json);
          axis_ASSERT(kv && axis_value_kv_check_integrity(kv),
                     "Invalid argument.");

          if (!kv) {
            axis_list_clear(&object);
            return false;
          }

          axis_list_push_ptr_back(
              &object, kv,
              (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);
        }

        return axis_value_set_object_with_move(self, &object);
      }
      break;
    case axis_TYPE_PTR:
    case axis_TYPE_BUF:
      axis_LOGE("Not implemented yet.");
      break;
  }

  return false;
}

static bool axis_value_init_from_json(axis_value_t *self, axis_json_t *json) {
  axis_ASSERT(self && json, "Invalid argument.");

  if (!self || !json) {
    return false;
  }

  switch (axis_json_get_type(json)) {
    case axis_TYPE_UINT64:
      return axis_value_init_uint64(self, axis_json_get_integer_value(json));
    case axis_TYPE_INT64:
      return axis_value_init_int64(self, axis_json_get_integer_value(json));
    case axis_TYPE_FLOAT64:
      return axis_value_init_float64(self, axis_json_get_real_value(json));
    case axis_TYPE_BOOL:
      if (axis_json_get_boolean_value(json)) {
        return axis_value_init_bool(self, true);
      } else {
        return axis_value_init_bool(self, false);
      }
      break;
    case axis_TYPE_NULL:
      return axis_value_init_null(self);
    case axis_TYPE_STRING:
      return axis_value_init_string_with_size(
          self, axis_json_peek_string_value(json),
          strlen(axis_json_peek_string_value(json)));
    case axis_TYPE_ARRAY: {
      axis_value_init_array_with_move(self, NULL);

      // Loop each item in the JSON array and convert them to axis_value_t.
      size_t i = 0;
      axis_json_t *item_json = NULL;
      axis_json_array_foreach(json, i, item_json) {
        axis_value_t *item = axis_value_from_json(item_json);
        axis_ASSERT(item && axis_value_check_integrity(item),
                   "Invalid argument.");

        if (item == NULL) {
          // Something wrong, we should destroy the value and return NULL.
          axis_value_deinit(self);
          return false;
        }

        axis_list_push_ptr_back(
            &self->content.array, item,
            (axis_ptr_listnode_destroy_func_t)axis_value_destroy);
      }

      return true;
    }
    case axis_TYPE_OBJECT: {
      axis_value_init_object_with_move(self, NULL);

      // Loop each item in the JSON object and convert them to axis_value_kv_t.
      const char *key = NULL;
      axis_json_t *value_json = NULL;
      axis_json_object_foreach(json, key, value_json) {
        axis_value_kv_t *kv = axis_value_kv_from_json(key, value_json);
        axis_ASSERT(kv && axis_value_kv_check_integrity(kv), "Invalid argument.");

        if (kv == NULL) {
          // Something wrong, we should destroy the value and return NULL.
          axis_value_deinit(self);
          return false;
        }

        axis_list_push_ptr_back(
            &self->content.object, kv,
            (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);
      }

      return true;
    }
    default:
      axis_ASSERT(0, "Should not happen.");
      return false;
  }
}

axis_value_t *axis_value_from_json(axis_json_t *json) {
  axis_ASSERT(json, "Invalid argument.");

  if (json == NULL) {
    return NULL;
  }

  axis_value_t *value = axis_value_create_invalid();
  if (!axis_value_init_from_json(value, json)) {
    axis_value_destroy(value);
    return NULL;
  }
  return value;
}

static axis_json_t *axis_value_array_to_json(axis_value_t *self) {
  if (self == NULL) {
    axis_ASSERT(0, "Invalid argument.");
    return NULL;
  }

  axis_ASSERT(axis_value_check_integrity(self), "Invalid argument.");

  if (!axis_value_is_array(self)) {
    axis_ASSERT(0, "Invalid argument: %d", axis_value_get_type(self));
    return NULL;
  }

  axis_json_t *json = axis_json_create_array();
  if (json == NULL) {
    axis_ASSERT(0, "Failed to create a json array.");
    return NULL;
  }

  // Loop each item in the array and convert them to JSON.
  axis_list_foreach (&self->content.array, iter) {
    axis_value_t *item = axis_ptr_listnode_get(iter.node);
    if (!item) {
      axis_ASSERT(0, "Failed to get item from the array.");
      axis_json_destroy(json);
      return NULL;
    }
    axis_ASSERT(axis_value_check_integrity(item), "Invalid argument.");

    axis_json_array_append_new(json, axis_value_to_json(item));
  }

  return json;
}

static axis_json_t *axis_value_object_to_json(axis_value_t *self) {
  if (self == NULL) {
    axis_ASSERT(0, "Invalid argument.");
    return NULL;
  }

  axis_ASSERT(axis_value_check_integrity(self), "Invalid argument.");

  if (!axis_value_is_object(self)) {
    axis_ASSERT(0, "Invalid argument: %d", axis_value_get_type(self));
    return NULL;
  }

  axis_json_t *json = axis_json_create_object();
  if (json == NULL) {
    axis_ASSERT(0, "Failed to create a json object.");
    return NULL;
  }

  axis_list_foreach (&self->content.object, iter) {
    axis_value_kv_t *item = axis_ptr_listnode_get(iter.node);
    if (!item) {
      axis_ASSERT(0, "Failed to get item from the object.");
      axis_json_destroy(json);
      return NULL;
    }
    axis_ASSERT(axis_value_kv_check_integrity(item), "Invalid argument.");

    axis_value_kv_to_json(json, item);
  }

  return json;
}

static axis_json_t *axis_value_invalid_to_json(axis_value_t *self) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");
  axis_ASSERT(axis_value_is_invalid(self), "Invalid argument: %d",
             axis_value_get_type(self));

  axis_ASSERT(0, "Invalid argument.");
  return NULL;
}

static axis_json_t *axis_value_int8_to_json(axis_value_t *self) {
  if (self == NULL) {
    axis_ASSERT(0, "Invalid argument.");
    return NULL;
  }

  axis_ASSERT(axis_value_check_integrity(self), "Invalid argument.");

  if (!axis_value_is_int8(self)) {
    axis_ASSERT(0, "Invalid argument: %d", axis_value_get_type(self));
    return NULL;
  }

  return axis_json_create_integer(self->content.int8);
}

static axis_json_t *axis_value_int16_to_json(axis_value_t *self) {
  if (self == NULL) {
    axis_ASSERT(0, "Invalid argument.");
    return NULL;
  }

  axis_ASSERT(axis_value_check_integrity(self), "Invalid argument.");

  if (!axis_value_is_int16(self)) {
    axis_ASSERT(0, "Invalid argument: %d", axis_value_get_type(self));
    return NULL;
  }

  return axis_json_create_integer(self->content.int16);
}

static axis_json_t *axis_value_int32_to_json(axis_value_t *self) {
  if (self == NULL) {
    axis_ASSERT(0, "Invalid argument.");
    return NULL;
  }

  axis_ASSERT(axis_value_check_integrity(self), "Invalid argument.");

  if (!axis_value_is_int32(self)) {
    axis_ASSERT(0, "Invalid argument: %d", axis_value_get_type(self));
    return NULL;
  }

  return axis_json_create_integer(self->content.int32);
}

static axis_json_t *axis_value_int64_to_json(axis_value_t *self) {
  if (self == NULL) {
    axis_ASSERT(0, "Invalid argument.");
    return NULL;
  }

  axis_ASSERT(axis_value_check_integrity(self), "Invalid argument.");

  if (!axis_value_is_int64(self)) {
    axis_ASSERT(0, "Invalid argument: %d", axis_value_get_type(self));
    return NULL;
  }

  return axis_json_create_integer(self->content.int64);
}

static axis_json_t *axis_value_uint8_to_json(axis_value_t *self) {
  if (self == NULL) {
    axis_ASSERT(0, "Invalid argument.");
    return NULL;
  }

  axis_ASSERT(axis_value_check_integrity(self), "Invalid argument.");

  if (!axis_value_is_uint8(self)) {
    axis_ASSERT(0, "Invalid argument: %d", axis_value_get_type(self));
    return NULL;
  }

  return axis_json_create_integer(self->content.uint8);
}

static axis_json_t *axis_value_uint16_to_json(axis_value_t *self) {
  if (self == NULL) {
    axis_ASSERT(0, "Invalid argument.");
    return NULL;
  }

  axis_ASSERT(axis_value_check_integrity(self), "Invalid argument.");

  if (!axis_value_is_uint16(self)) {
    axis_ASSERT(0, "Invalid argument: %d", axis_value_get_type(self));
    return NULL;
  }

  return axis_json_create_integer(self->content.uint16);
}

static axis_json_t *axis_value_uint32_to_json(axis_value_t *self) {
  if (self == NULL) {
    axis_ASSERT(0, "Invalid argument.");
    return NULL;
  }

  axis_ASSERT(axis_value_check_integrity(self), "Invalid argument.");

  if (!axis_value_is_uint32(self)) {
    axis_ASSERT(0, "Invalid argument: %d", axis_value_get_type(self));
    return NULL;
  }

  return axis_json_create_integer(self->content.uint32);
}

static axis_json_t *axis_value_uint64_to_json(axis_value_t *self) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");
  axis_ASSERT(axis_value_is_uint64(self), "Invalid argument.");

  // FIXME(Liu): the jansson library does not support uint64_t, it's just a work
  // around here.
  if (self->content.uint64 > INT64_MAX) {
    axis_ASSERT(0, "The value is too large to convert to int64.");
  }

  return axis_json_create_integer((int64_t)self->content.uint64);
}

static axis_json_t *axis_value_float32_to_json(axis_value_t *self) {
  if (self == NULL) {
    axis_ASSERT(0, "Invalid argument.");
    return NULL;
  }

  axis_ASSERT(axis_value_check_integrity(self), "Invalid argument.");

  if (!axis_value_is_float32(self)) {
    axis_ASSERT(0, "Invalid argument: %d", axis_value_get_type(self));
    return NULL;
  }

  return axis_json_create_real(self->content.float32);
}

static axis_json_t *axis_value_float64_to_json(axis_value_t *self) {
  if (self == NULL) {
    axis_ASSERT(0, "Invalid argument.");
    return NULL;
  }

  axis_ASSERT(axis_value_check_integrity(self), "Invalid argument.");

  if (!axis_value_is_float64(self)) {
    axis_ASSERT(0, "Invalid argument: %d", axis_value_get_type(self));
    return NULL;
  }

  return axis_json_create_real(self->content.float64);
}

static axis_json_t *axis_value_string_to_json(axis_value_t *self) {
  if (self == NULL) {
    axis_ASSERT(0, "Invalid argument.");
    return NULL;
  }

  axis_ASSERT(axis_value_check_integrity(self), "Invalid argument.");

  if (!axis_value_is_string(self)) {
    axis_ASSERT(0, "Invalid argument: %d", axis_value_get_type(self));
    return NULL;
  }

  return axis_json_create_string(axis_string_get_raw_str(&self->content.string));
}

static axis_json_t *axis_value_bool_to_json(axis_value_t *self) {
  if (self == NULL) {
    axis_ASSERT(0, "Invalid argument.");
    return NULL;
  }

  axis_ASSERT(axis_value_check_integrity(self), "Invalid argument.");

  if (!axis_value_is_bool(self)) {
    axis_ASSERT(0, "Invalid argument: %d", axis_value_get_type(self));
    return NULL;
  }

  return (self->content.boolean == true) ? axis_json_create_true()
                                         : axis_json_create_false();
}

static axis_json_t *axis_value_null_to_json(axis_value_t *self) {
  if (self == NULL) {
    axis_ASSERT(0, "Invalid argument.");
    return NULL;
  }

  axis_ASSERT(axis_value_check_integrity(self), "Invalid argument.");

  if (!axis_value_is_null(self)) {
    axis_ASSERT(0, "Invalid argument: %d", axis_value_get_type(self));
    return NULL;
  }

  return axis_json_create_null();
}

static axis_json_t *axis_value_ptr_to_json(axis_value_t *self) {
  if (self == NULL) {
    axis_ASSERT(0, "Invalid argument.");
    return NULL;
  }

  axis_ASSERT(axis_value_check_integrity(self), "Invalid argument.");

  if (!axis_value_is_ptr(self)) {
    axis_ASSERT(0, "Invalid argument: %d", axis_value_get_type(self));
    return NULL;
  }

  // TODO(Wei): Currently, return 'null', but the correct approach might be to
  // convert the pointer 'value' itself to a string or uint64_t, and then
  // serialize it into JSON. If using uint64_t, the JSON library needs to be
  // able to handle uint64_t values.
  return axis_json_create_null();
}

static axis_json_t *axis_value_buf_to_json(axis_value_t *self) {
  if (self == NULL) {
    axis_ASSERT(0, "Invalid argument.");
    return NULL;
  }

  axis_ASSERT(axis_value_check_integrity(self), "Invalid argument.");

  if (!axis_value_is_buf(self)) {
    axis_ASSERT(0, "Invalid argument: %d", axis_value_get_type(self));
    return NULL;
  }

  // TODO(Wei): Currently, return 'null', but the correct approach is to convert
  // the buf 'content' itself to a base64 encoded string, and then serialize it
  // into JSON.
  return axis_json_create_null();
}

axis_json_t *axis_value_to_json(axis_value_t *self) {
  if (self == NULL) {
    axis_ASSERT(0, "Invalid argument.");
    return NULL;
  }

  axis_ASSERT(axis_value_check_integrity(self), "Invalid argument.");

  switch (axis_value_get_type(self)) {
    case axis_TYPE_INVALID:
      return NULL;
    case axis_TYPE_INT8:
      return axis_value_int8_to_json(self);
    case axis_TYPE_INT16:
      return axis_value_int16_to_json(self);
    case axis_TYPE_INT32:
      return axis_value_int32_to_json(self);
    case axis_TYPE_INT64:
      return axis_value_int64_to_json(self);
    case axis_TYPE_UINT8:
      return axis_value_uint8_to_json(self);
    case axis_TYPE_UINT16:
      return axis_value_uint16_to_json(self);
    case axis_TYPE_UINT32:
      return axis_value_uint32_to_json(self);
    case axis_TYPE_UINT64:
      return axis_value_uint64_to_json(self);
    case axis_TYPE_FLOAT32:
      return axis_value_float32_to_json(self);
    case axis_TYPE_FLOAT64:
      return axis_value_float64_to_json(self);
    case axis_TYPE_PTR:
      return axis_value_ptr_to_json(self);
    case axis_TYPE_BUF:
      return axis_value_buf_to_json(self);
    case axis_TYPE_STRING:
      return axis_value_string_to_json(self);
    case axis_TYPE_OBJECT:
      return axis_value_object_to_json(self);
    case axis_TYPE_ARRAY:
      return axis_value_array_to_json(self);
    case axis_TYPE_NULL:
      return axis_value_null_to_json(self);
    case axis_TYPE_BOOL:
      return axis_value_bool_to_json(self);
    default:
      axis_ASSERT(0, "Invalid argument.");
      break;
  }
}
