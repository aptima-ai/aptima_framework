//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/lib/json.h"

#include <stdbool.h>

#include "jansson.h"
#include "ten_runtime/common/errno.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/value/type.h"

bool ten_json_check_integrity(ten_json_t *json) {
  TEN_ASSERT(json, "Invalid argument.");

  bool result = true;

  // Using whether successfully dumping JSON into a string to check if it is a
  // valid JSON object.
  // char *json_str = json_dumps(json, JSON_ENCODE_ANY);
  // result = json_str ? true : false;
  // free(json_str);

  return result;
}

TEN_TYPE ten_json_get_type(ten_json_t *json) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Invalid argument.");

  switch (json_typeof(json)) {
    case JSON_OBJECT:
      return TEN_TYPE_OBJECT;
    case JSON_ARRAY:
      return TEN_TYPE_ARRAY;
    case JSON_STRING:
      return TEN_TYPE_STRING;
    case JSON_INTEGER:
      if (ten_json_get_integer_value(json) >= 0) {
        return TEN_TYPE_UINT64;
      } else {
        return TEN_TYPE_INT64;
      }
    case JSON_REAL:
      return TEN_TYPE_FLOAT64;
    case JSON_TRUE:
    case JSON_FALSE:
      return TEN_TYPE_BOOL;
    case JSON_NULL:
      return TEN_TYPE_NULL;
    default:
      TEN_ASSERT(0, "Should not happen.");
      return TEN_TYPE_INVALID;
  }
}

const char *ten_json_object_peek_string(ten_json_t *json, const char *field) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Invalid argument.");
  TEN_ASSERT(field, "Invalid argument.");

  ten_json_t *result = ten_json_object_peek(json, field);
  if (json_is_string(result)) {
    return json_string_value(result);
  }

  return NULL;
}

int64_t ten_json_object_get_integer(ten_json_t *json, const char *field) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Invalid argument.");
  TEN_ASSERT(field, "Invalid argument.");

  ten_json_t *result = ten_json_object_peek(json, field);
  if (json_is_integer(result)) {
    return json_integer_value(result);
  }

  return 0;
}

double ten_json_object_get_real(ten_json_t *json, const char *field) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Invalid argument.");
  TEN_ASSERT(field, "Invalid argument.");

  ten_json_t *result = ten_json_object_peek(json, field);
  if (json_is_real(result)) {
    return json_real_value(result);
  }

  return 0;
}

bool ten_json_object_get_boolean(ten_json_t *json, const char *field) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Invalid argument.");
  TEN_ASSERT(field, "Invalid argument.");

  ten_json_t *result = ten_json_object_peek(json, field);
  if (result && json_is_boolean(result)) {
    return json_boolean_value(result);
  }

  return false;
}

ten_json_t *ten_json_object_peek_array(ten_json_t *json, const char *field) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Invalid argument.");
  TEN_ASSERT(field, "Invalid argument.");

  ten_json_t *result = ten_json_object_peek(json, field);
  if (json_is_array(result)) {
    return result;
  }

  return NULL;
}

ten_json_t *ten_json_object_peek_array_forcibly(ten_json_t *json,
                                                const char *field) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Invalid argument.");
  TEN_ASSERT(field, "Invalid argument.");

  ten_json_t *result = ten_json_object_peek_array(json, field);
  if (result) {
    return result;
  }

  if (ten_json_object_peek(json, field)) {
    ten_json_object_del(json, field);
  }

  result = json_array();
  assert(result);

  json_object_set_new(json, field, result);

  return result;
}

ten_json_t *ten_json_object_peek_object(ten_json_t *json, const char *field) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Invalid argument.");
  TEN_ASSERT(field, "Invalid argument.");

  ten_json_t *result = ten_json_object_peek(json, field);
  if (json_is_object(result)) {
    return result;
  }

  return NULL;
}

ten_json_t *ten_json_object_peek_object_forcibly(ten_json_t *json,
                                                 const char *field) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Invalid argument.");
  TEN_ASSERT(field, "Invalid argument.");

  ten_json_t *result = ten_json_object_peek_object(json, field);
  if (result) {
    return result;
  }

  if (ten_json_object_peek(json, field)) {
    ten_json_object_del(json, field);
  }

  result = json_object();
  assert(result);

  json_object_set_new(json, field, result);

  return result;
}

void ten_json_object_set_new(ten_json_t *json, const char *field,
                             ten_json_t *value) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Invalid argument.");
  TEN_ASSERT(field, "Invalid argument.");
  TEN_ASSERT(value && ten_json_check_integrity(value), "Invalid argument.");

  json_object_set_new(json, field, value);
}

void ten_json_array_append_new(ten_json_t *json, ten_json_t *value) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Invalid argument.");
  TEN_ASSERT(value && ten_json_check_integrity(value), "Invalid argument.");

  json_array_append_new(json, value);
}

ten_json_t *ten_json_object_peek(ten_json_t *json, const char *field) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Invalid argument.");
  TEN_ASSERT(field, "Invalid argument.");

  if (!json_is_object(json)) {
    return NULL;
  }

  return json_object_get(json, field);
}

const char *ten_json_to_string(ten_json_t *json, const char *field,
                               bool *must_free) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Invalid argument.");
  TEN_ASSERT(must_free, "Invalid argument.");

  *must_free = false;

  if (field) {
    if (json_is_object(json) && json_object_get(json, field) != NULL) {
      json_t *value = json_object_get(json, field);
      switch (value->type) {
        case JSON_STRING:
          return json_string_value(value);
        case JSON_OBJECT:
        case JSON_ARRAY:
          *must_free = true;  // json_dumps will invoke malloc
          return json_dumps(value, JSON_ENCODE_ANY);

        default:
          TEN_ASSERT(0, "Handle more types: %d", value->type);
          break;
      }
    }
  } else {
    *must_free = true;  // json_dumps will invoke malloc
    return json_dumps(json, JSON_ENCODE_ANY);
  }

  return NULL;
}

bool ten_json_object_del(ten_json_t *json, const char *field) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Invalid argument.");
  TEN_ASSERT(field, "Invalid argument.");

  if (ten_json_object_peek(json, field)) {
    // 0: success, -1: error
    return json_object_del(json, field) == 0;
  }

  return false;
}

ten_json_t *ten_json_from_string(const char *msg, ten_error_t *err) {
  json_error_t error;
  json_t *ret = NULL;

  if (!msg || !*msg) {
    return NULL;
  }
  ret = json_loads(msg, JSON_DECODE_ANY, &error);
  if (!ret) {
    if (err) {
      ten_error_set(err, TEN_ERRNO_INVALID_JSON, "%s: %s", msg, error.text);
    }
    TEN_LOGE("Failed to parse %s: %s", msg, error.text);
  }

  return ret;
}

void ten_json_destroy(ten_json_t *json) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Invalid argument.");

  if (json) {
    json_decref(json);
  }
}

bool ten_json_is_object(ten_json_t *json) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Invalid argument.");
  return json_is_object(json);
}

bool ten_json_is_array(ten_json_t *json) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Invalid argument.");
  return json_is_array(json);
}

bool ten_json_is_string(ten_json_t *json) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Invalid argument.");
  return json_is_string(json);
}

bool ten_json_is_integer(ten_json_t *json) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Invalid argument.");
  return json_is_integer(json);
}

bool ten_json_is_boolean(ten_json_t *json) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Invalid argument.");
  return json_is_boolean(json);
}

bool ten_json_is_real(ten_json_t *json) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Invalid argument.");
  return json_is_real(json);
}

bool ten_json_is_null(ten_json_t *json) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Invalid argument.");
  return json_is_null(json);
}

bool ten_json_is_true(ten_json_t *json) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Invalid argument.");
  return json_is_true(json);
}

const char *ten_json_peek_string_value(ten_json_t *json) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Invalid argument.");
  return json_string_value(json);
}

int64_t ten_json_get_integer_value(ten_json_t *json) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Invalid argument.");
  return json_integer_value(json);
}

bool ten_json_get_boolean_value(ten_json_t *json) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Invalid argument.");
  return json_boolean_value(json);
}

double ten_json_get_real_value(ten_json_t *json) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Invalid argument.");
  return json_real_value(json);
}

double ten_json_get_number_value(ten_json_t *json) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Invalid argument.");
  return json_number_value(json);
}

ten_json_t *ten_json_create_object(void) {
  json_t *json = json_object();
  TEN_ASSERT(json && ten_json_check_integrity(json), "Should not happen.");
  return json;
}

ten_json_t *ten_json_create_array(void) {
  json_t *json = json_array();
  TEN_ASSERT(json && ten_json_check_integrity(json), "Should not happen.");
  return json;
}

ten_json_t *ten_json_create_string(const char *str) {
  TEN_ASSERT(str, "Invalid argument.");

  json_t *json = json_string(str);
  TEN_ASSERT(json && ten_json_check_integrity(json), "Should not happen.");

  return json;
}

ten_json_t *ten_json_create_integer(int64_t value) {
  json_t *json = json_integer(value);
  TEN_ASSERT(json && ten_json_check_integrity(json), "Should not happen.");
  return json;
}

ten_json_t *ten_json_create_real(double value) {
  json_t *json = json_real(value);
  TEN_ASSERT(json && ten_json_check_integrity(json), "Should not happen.");
  return json;
}

ten_json_t *ten_json_create_boolean(bool value) {
  json_t *json = json_boolean(value);
  TEN_ASSERT(json && ten_json_check_integrity(json), "Should not happen.");
  return json;
}

ten_json_t *ten_json_create_true(void) {
  json_t *json = json_true();
  TEN_ASSERT(json && ten_json_check_integrity(json), "Should not happen.");
  return json;
}

ten_json_t *ten_json_create_false(void) {
  json_t *json = json_false();
  TEN_ASSERT(json && ten_json_check_integrity(json), "Should not happen.");
  return json;
}

ten_json_t *ten_json_create_null(void) {
  json_t *json = json_null();
  TEN_ASSERT(json && ten_json_check_integrity(json), "Should not happen.");
  return json;
}

size_t ten_json_array_get_size(ten_json_t *json) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Invalid argument.");
  return json_array_size(json);
}

ten_json_t *ten_json_array_peek_item(ten_json_t *json, size_t index) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Invalid argument.");
  return json_array_get(json, index);
}

void ten_json_object_update_missing(ten_json_t *json, ten_json_t *other) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Invalid argument.");
  TEN_ASSERT(other && ten_json_check_integrity(other), "Invalid argument.");
  json_object_update_missing(json, other);
}

const char *ten_json_object_iter_key(void *iter) {
  return json_object_iter_key(iter);
}

ten_json_t *ten_json_object_iter_value(void *iter) {
  return json_object_iter_value(iter);
}

void *ten_json_object_iter(ten_json_t *json) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Invalid argument.");
  return json_object_iter(json);
}

void *ten_json_object_iter_at(ten_json_t *json, const char *key) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Invalid argument.");
  return json_object_iter_at(json, key);
}

void *ten_json_object_key_to_iter(const char *key) {
  return json_object_key_to_iter(key);
}

void *ten_json_object_iter_next(ten_json_t *json, void *iter) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Invalid argument.");
  return json_object_iter_next(json, iter);
}

ten_json_t *ten_json_incref(ten_json_t *json) {
  TEN_ASSERT(json && ten_json_check_integrity(json), "Invalid argument.");
  return json_incref(json);
}
