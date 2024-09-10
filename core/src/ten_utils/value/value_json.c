//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include <stdint.h>
#include <stdlib.h>

#include "ten_utils/container/list.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/value/type.h"
#include "ten_utils/value/value.h"
#include "ten_utils/value/value_get.h"
#include "ten_utils/value/value_is.h"
#include "ten_utils/value/value_kv.h"

static bool ten_value_init_from_json(ten_value_t *self, ten_json_t *json) {
  TEN_ASSERT(self && json, "Invalid argument.");

  if (!self || !json) {
    return NULL;
  }

  switch (ten_json_get_type(json)) {
    case TEN_TYPE_UINT64:
      return ten_value_init_uint64(self, ten_json_get_integer_value(json));
    case TEN_TYPE_INT64:
      return ten_value_init_int64(self, ten_json_get_integer_value(json));
    case TEN_TYPE_FLOAT64:
      return ten_value_init_float64(self, ten_json_get_real_value(json));
    case TEN_TYPE_BOOL:
      if (ten_json_get_boolean_value(json)) {
        return ten_value_init_bool(self, true);
      } else {
        return ten_value_init_bool(self, false);
      }
      break;
    case TEN_TYPE_NULL:
      return ten_value_init_null(self);
    case TEN_TYPE_STRING:
      return ten_value_init_string_with_size(
          self, ten_json_peek_string_value(json),
          strlen(ten_json_peek_string_value(json)));
    case TEN_TYPE_ARRAY: {
      ten_value_init_array_with_move(self, NULL);

      // Loop each item in the JSON array and convert them to ten_value_t.
      size_t i = 0;
      ten_json_t *item_json = NULL;
      ten_json_array_foreach(json, i, item_json) {
        ten_value_t *item = ten_value_from_json(item_json);
        TEN_ASSERT(item && ten_value_check_integrity(item),
                   "Invalid argument.");

        if (item == NULL) {
          // Something wrong, we should destroy the value and return NULL.
          ten_value_deinit(self);
          return false;
        }

        ten_list_push_ptr_back(
            &self->content.array, item,
            (ten_ptr_listnode_destroy_func_t)ten_value_destroy);
      }

      return true;
    }
    case TEN_TYPE_OBJECT: {
      ten_value_init_object_with_move(self, NULL);

      // Loop each item in the JSON object and convert them to ten_value_kv_t.
      const char *key = NULL;
      ten_json_t *value_json = NULL;
      ten_json_object_foreach(json, key, value_json) {
        ten_value_kv_t *kv = ten_value_kv_from_json(key, value_json);
        TEN_ASSERT(kv && ten_value_kv_check_integrity(kv), "Invalid argument.");

        if (kv == NULL) {
          // Something wrong, we should destroy the value and return NULL.
          ten_value_deinit(self);
          return false;
        }

        ten_list_push_ptr_back(
            &self->content.object, kv,
            (ten_ptr_listnode_destroy_func_t)ten_value_kv_destroy);
      }

      return true;
    }
    default:
      TEN_ASSERT(0, "Should not happen.");
      return false;
  }
}

ten_value_t *ten_value_from_json(ten_json_t *json) {
  TEN_ASSERT(json, "Invalid argument.");

  if (json == NULL) {
    return NULL;
  }

  ten_value_t *value = ten_value_create_invalid();
  if (!ten_value_init_from_json(value, json)) {
    ten_value_destroy(value);
    return NULL;
  }
  return value;
}

static ten_json_t *ten_value_array_to_json(ten_value_t *self) {
  if (self == NULL) {
    TEN_ASSERT(0, "Invalid argument.");
    return NULL;
  }

  TEN_ASSERT(ten_value_check_integrity(self), "Invalid argument.");

  if (!ten_value_is_array(self)) {
    TEN_ASSERT(0, "Invalid argument: %d", ten_value_get_type(self));
    return NULL;
  }

  ten_json_t *json = ten_json_create_array();
  if (json == NULL) {
    TEN_ASSERT(0, "Failed to create a json array.");
    return NULL;
  }

  // Loop each item in the array and convert them to JSON.
  ten_list_foreach (&self->content.array, iter) {
    ten_value_t *item = ten_ptr_listnode_get(iter.node);
    if (!item) {
      TEN_ASSERT(0, "Failed to get item from the array.");
      ten_json_destroy(json);
      return NULL;
    }
    TEN_ASSERT(ten_value_check_integrity(item), "Invalid argument.");

    ten_json_array_append_new(json, ten_value_to_json(item));
  }

  return json;
}

static ten_json_t *ten_value_object_to_json(ten_value_t *self) {
  if (self == NULL) {
    TEN_ASSERT(0, "Invalid argument.");
    return NULL;
  }

  TEN_ASSERT(ten_value_check_integrity(self), "Invalid argument.");

  if (!ten_value_is_object(self)) {
    TEN_ASSERT(0, "Invalid argument: %d", ten_value_get_type(self));
    return NULL;
  }

  ten_json_t *json = ten_json_create_object();
  if (json == NULL) {
    TEN_ASSERT(0, "Failed to create a json object.");
    return NULL;
  }

  ten_list_foreach (&self->content.object, iter) {
    ten_value_kv_t *item = ten_ptr_listnode_get(iter.node);
    if (!item) {
      TEN_ASSERT(0, "Failed to get item from the object.");
      ten_json_destroy(json);
      return NULL;
    }
    TEN_ASSERT(ten_value_kv_check_integrity(item), "Invalid argument.");

    ten_value_kv_to_json(json, item);
  }

  return json;
}

static ten_json_t *ten_value_invalid_to_json(ten_value_t *self) {
  TEN_ASSERT(self && ten_value_check_integrity(self), "Invalid argument.");
  TEN_ASSERT(ten_value_is_invalid(self), "Invalid argument: %d",
             ten_value_get_type(self));

  TEN_ASSERT(0, "Invalid argument.");
  return NULL;
}

static ten_json_t *ten_value_int8_to_json(ten_value_t *self) {
  if (self == NULL) {
    TEN_ASSERT(0, "Invalid argument.");
    return NULL;
  }

  TEN_ASSERT(ten_value_check_integrity(self), "Invalid argument.");

  if (!ten_value_is_int8(self)) {
    TEN_ASSERT(0, "Invalid argument: %d", ten_value_get_type(self));
    return NULL;
  }

  return ten_json_create_integer(self->content.int8);
}

static ten_json_t *ten_value_int16_to_json(ten_value_t *self) {
  if (self == NULL) {
    TEN_ASSERT(0, "Invalid argument.");
    return NULL;
  }

  TEN_ASSERT(ten_value_check_integrity(self), "Invalid argument.");

  if (!ten_value_is_int16(self)) {
    TEN_ASSERT(0, "Invalid argument: %d", ten_value_get_type(self));
    return NULL;
  }

  return ten_json_create_integer(self->content.int16);
}

static ten_json_t *ten_value_int32_to_json(ten_value_t *self) {
  if (self == NULL) {
    TEN_ASSERT(0, "Invalid argument.");
    return NULL;
  }

  TEN_ASSERT(ten_value_check_integrity(self), "Invalid argument.");

  if (!ten_value_is_int32(self)) {
    TEN_ASSERT(0, "Invalid argument: %d", ten_value_get_type(self));
    return NULL;
  }

  return ten_json_create_integer(self->content.int32);
}

static ten_json_t *ten_value_int64_to_json(ten_value_t *self) {
  if (self == NULL) {
    TEN_ASSERT(0, "Invalid argument.");
    return NULL;
  }

  TEN_ASSERT(ten_value_check_integrity(self), "Invalid argument.");

  if (!ten_value_is_int64(self)) {
    TEN_ASSERT(0, "Invalid argument: %d", ten_value_get_type(self));
    return NULL;
  }

  return ten_json_create_integer(self->content.int64);
}

static ten_json_t *ten_value_uint8_to_json(ten_value_t *self) {
  if (self == NULL) {
    TEN_ASSERT(0, "Invalid argument.");
    return NULL;
  }

  TEN_ASSERT(ten_value_check_integrity(self), "Invalid argument.");

  if (!ten_value_is_uint8(self)) {
    TEN_ASSERT(0, "Invalid argument: %d", ten_value_get_type(self));
    return NULL;
  }

  return ten_json_create_integer(self->content.uint8);
}

static ten_json_t *ten_value_uint16_to_json(ten_value_t *self) {
  if (self == NULL) {
    TEN_ASSERT(0, "Invalid argument.");
    return NULL;
  }

  TEN_ASSERT(ten_value_check_integrity(self), "Invalid argument.");

  if (!ten_value_is_uint16(self)) {
    TEN_ASSERT(0, "Invalid argument: %d", ten_value_get_type(self));
    return NULL;
  }

  return ten_json_create_integer(self->content.uint16);
}

static ten_json_t *ten_value_uint32_to_json(ten_value_t *self) {
  if (self == NULL) {
    TEN_ASSERT(0, "Invalid argument.");
    return NULL;
  }

  TEN_ASSERT(ten_value_check_integrity(self), "Invalid argument.");

  if (!ten_value_is_uint32(self)) {
    TEN_ASSERT(0, "Invalid argument: %d", ten_value_get_type(self));
    return NULL;
  }

  return ten_json_create_integer(self->content.uint32);
}

static ten_json_t *ten_value_uint64_to_json(ten_value_t *self) {
  TEN_ASSERT(self && ten_value_check_integrity(self), "Invalid argument.");
  TEN_ASSERT(ten_value_is_uint64(self), "Invalid argument.");

  // FIXME(Liu): the jansson library does not support uint64_t, it's just a work
  // around here.
  if (self->content.uint64 > INT64_MAX) {
    TEN_ASSERT(0, "The value is too large to convert to int64.");
  }

  return ten_json_create_integer((int64_t)self->content.uint64);
}

static ten_json_t *ten_value_float32_to_json(ten_value_t *self) {
  if (self == NULL) {
    TEN_ASSERT(0, "Invalid argument.");
    return NULL;
  }

  TEN_ASSERT(ten_value_check_integrity(self), "Invalid argument.");

  if (!ten_value_is_float32(self)) {
    TEN_ASSERT(0, "Invalid argument: %d", ten_value_get_type(self));
    return NULL;
  }

  return ten_json_create_real(self->content.float32);
}

static ten_json_t *ten_value_float64_to_json(ten_value_t *self) {
  if (self == NULL) {
    TEN_ASSERT(0, "Invalid argument.");
    return NULL;
  }

  TEN_ASSERT(ten_value_check_integrity(self), "Invalid argument.");

  if (!ten_value_is_float64(self)) {
    TEN_ASSERT(0, "Invalid argument: %d", ten_value_get_type(self));
    return NULL;
  }

  return ten_json_create_real(self->content.float64);
}

static ten_json_t *ten_value_string_to_json(ten_value_t *self) {
  if (self == NULL) {
    TEN_ASSERT(0, "Invalid argument.");
    return NULL;
  }

  TEN_ASSERT(ten_value_check_integrity(self), "Invalid argument.");

  if (!ten_value_is_string(self)) {
    TEN_ASSERT(0, "Invalid argument: %d", ten_value_get_type(self));
    return NULL;
  }

  return ten_json_create_string(ten_string_get_raw_str(&self->content.string));
}

static ten_json_t *ten_value_bool_to_json(ten_value_t *self) {
  if (self == NULL) {
    TEN_ASSERT(0, "Invalid argument.");
    return NULL;
  }

  TEN_ASSERT(ten_value_check_integrity(self), "Invalid argument.");

  if (!ten_value_is_bool(self)) {
    TEN_ASSERT(0, "Invalid argument: %d", ten_value_get_type(self));
    return NULL;
  }

  return (self->content.boolean == true) ? ten_json_create_true()
                                         : ten_json_create_false();
}

static ten_json_t *ten_value_null_to_json(ten_value_t *self) {
  if (self == NULL) {
    TEN_ASSERT(0, "Invalid argument.");
    return NULL;
  }

  TEN_ASSERT(ten_value_check_integrity(self), "Invalid argument.");

  if (!ten_value_is_null(self)) {
    TEN_ASSERT(0, "Invalid argument: %d", ten_value_get_type(self));
    return NULL;
  }

  return ten_json_create_null();
}

static ten_json_t *ten_value_ptr_to_json(ten_value_t *self) {
  if (self == NULL) {
    TEN_ASSERT(0, "Invalid argument.");
    return NULL;
  }

  TEN_ASSERT(ten_value_check_integrity(self), "Invalid argument.");

  if (!ten_value_is_ptr(self)) {
    TEN_ASSERT(0, "Invalid argument: %d", ten_value_get_type(self));
    return NULL;
  }

  // TODO(Wei): Currently, return 'null', but the correct approach might be to
  // convert the pointer 'value' itself to a string or uint64_t, and then
  // serialize it into JSON. If using uint64_t, the JSON library needs to be
  // able to handle uint64_t values.
  return ten_json_create_null();
}

static ten_json_t *ten_value_buf_to_json(ten_value_t *self) {
  if (self == NULL) {
    TEN_ASSERT(0, "Invalid argument.");
    return NULL;
  }

  TEN_ASSERT(ten_value_check_integrity(self), "Invalid argument.");

  if (!ten_value_is_buf(self)) {
    TEN_ASSERT(0, "Invalid argument: %d", ten_value_get_type(self));
    return NULL;
  }

  // TODO(Wei): Currently, return 'null', but the correct approach is to convert
  // the buf 'content' itself to a base64 encoded string, and then serialize it
  // into JSON.
  return ten_json_create_null();
}

ten_json_t *ten_value_to_json(ten_value_t *self) {
  if (self == NULL) {
    TEN_ASSERT(0, "Invalid argument.");
    return NULL;
  }

  TEN_ASSERT(ten_value_check_integrity(self), "Invalid argument.");

  switch (ten_value_get_type(self)) {
    case TEN_TYPE_INVALID:
      return NULL;
    case TEN_TYPE_INT8:
      return ten_value_int8_to_json(self);
    case TEN_TYPE_INT16:
      return ten_value_int16_to_json(self);
    case TEN_TYPE_INT32:
      return ten_value_int32_to_json(self);
    case TEN_TYPE_INT64:
      return ten_value_int64_to_json(self);
    case TEN_TYPE_UINT8:
      return ten_value_uint8_to_json(self);
    case TEN_TYPE_UINT16:
      return ten_value_uint16_to_json(self);
    case TEN_TYPE_UINT32:
      return ten_value_uint32_to_json(self);
    case TEN_TYPE_UINT64:
      return ten_value_uint64_to_json(self);
    case TEN_TYPE_FLOAT32:
      return ten_value_float32_to_json(self);
    case TEN_TYPE_FLOAT64:
      return ten_value_float64_to_json(self);
    case TEN_TYPE_PTR:
      return ten_value_ptr_to_json(self);
    case TEN_TYPE_BUF:
      return ten_value_buf_to_json(self);
    case TEN_TYPE_STRING:
      return ten_value_string_to_json(self);
    case TEN_TYPE_OBJECT:
      return ten_value_object_to_json(self);
    case TEN_TYPE_ARRAY:
      return ten_value_array_to_json(self);
    case TEN_TYPE_NULL:
      return ten_value_null_to_json(self);
    case TEN_TYPE_BOOL:
      return ten_value_bool_to_json(self);
    default:
      TEN_ASSERT(0, "Invalid argument.");
      break;
  }
}
