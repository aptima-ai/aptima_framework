//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/json.h"

#include <stdbool.h>

#include "jansson.h"
#include "axis_runtime/common/errno.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/log/log.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/value/type.h"

bool axis_json_check_integrity(axis_json_t *json) {
  axis_ASSERT(json, "Invalid argument.");

  bool result = true;

  // Using whether successfully dumping JSON into a string to check if it is a
  // valid JSON object.
  // char *json_str = json_dumps(json, JSON_ENCODE_ANY);
  // result = json_str ? true : false;
  // free(json_str);

  return result;
}

axis_TYPE axis_json_get_type(axis_json_t *json) {
  axis_ASSERT(json && axis_json_check_integrity(json), "Invalid argument.");

  switch (json_typeof(json)) {
    case JSON_OBJECT:
      return axis_TYPE_OBJECT;
    case JSON_ARRAY:
      return axis_TYPE_ARRAY;
    case JSON_STRING:
      return axis_TYPE_STRING;
    case JSON_INTEGER:
      if (axis_json_get_integer_value(json) >= 0) {
        return axis_TYPE_UINT64;
      } else {
        return axis_TYPE_INT64;
      }
    case JSON_REAL:
      return axis_TYPE_FLOAT64;
    case JSON_TRUE:
    case JSON_FALSE:
      return axis_TYPE_BOOL;
    case JSON_NULL:
      return axis_TYPE_NULL;
    default:
      axis_ASSERT(0, "Should not happen.");
      return axis_TYPE_INVALID;
  }
}

const char *axis_json_object_peek_string(axis_json_t *json, const char *field) {
  axis_ASSERT(json && axis_json_check_integrity(json), "Invalid argument.");
  axis_ASSERT(field, "Invalid argument.");

  axis_json_t *result = axis_json_object_peek(json, field);
  if (json_is_string(result)) {
    return json_string_value(result);
  }

  return NULL;
}

int64_t axis_json_object_get_integer(axis_json_t *json, const char *field) {
  axis_ASSERT(json && axis_json_check_integrity(json), "Invalid argument.");
  axis_ASSERT(field, "Invalid argument.");

  axis_json_t *result = axis_json_object_peek(json, field);
  if (json_is_integer(result)) {
    return json_integer_value(result);
  }

  return 0;
}

double axis_json_object_get_real(axis_json_t *json, const char *field) {
  axis_ASSERT(json && axis_json_check_integrity(json), "Invalid argument.");
  axis_ASSERT(field, "Invalid argument.");

  axis_json_t *result = axis_json_object_peek(json, field);
  if (json_is_real(result)) {
    return json_real_value(result);
  }

  return 0;
}

bool axis_json_object_get_boolean(axis_json_t *json, const char *field) {
  axis_ASSERT(json && axis_json_check_integrity(json), "Invalid argument.");
  axis_ASSERT(field, "Invalid argument.");

  axis_json_t *result = axis_json_object_peek(json, field);
  if (result && json_is_boolean(result)) {
    return json_boolean_value(result);
  }

  return false;
}

axis_json_t *axis_json_object_peek_array(axis_json_t *json, const char *field) {
  axis_ASSERT(json && axis_json_check_integrity(json), "Invalid argument.");
  axis_ASSERT(field, "Invalid argument.");

  axis_json_t *result = axis_json_object_peek(json, field);
  if (json_is_array(result)) {
    return result;
  }

  return NULL;
}

axis_json_t *axis_json_object_peek_array_forcibly(axis_json_t *json,
                                                const char *field) {
  axis_ASSERT(json && axis_json_check_integrity(json), "Invalid argument.");
  axis_ASSERT(field, "Invalid argument.");

  axis_json_t *result = axis_json_object_peek_array(json, field);
  if (result) {
    return result;
  }

  if (axis_json_object_peek(json, field)) {
    axis_json_object_del(json, field);
  }

  result = json_array();
  assert(result);

  json_object_set_new(json, field, result);

  return result;
}

axis_json_t *axis_json_object_peek_object(axis_json_t *json, const char *field) {
  axis_ASSERT(json && axis_json_check_integrity(json), "Invalid argument.");
  axis_ASSERT(field, "Invalid argument.");

  axis_json_t *result = axis_json_object_peek(json, field);
  if (json_is_object(result)) {
    return result;
  }

  return NULL;
}

axis_json_t *axis_json_object_peek_object_forcibly(axis_json_t *json,
                                                 const char *field) {
  axis_ASSERT(json && axis_json_check_integrity(json), "Invalid argument.");
  axis_ASSERT(field, "Invalid argument.");

  axis_json_t *result = axis_json_object_peek_object(json, field);
  if (result) {
    return result;
  }

  if (axis_json_object_peek(json, field)) {
    axis_json_object_del(json, field);
  }

  result = json_object();
  assert(result);

  json_object_set_new(json, field, result);

  return result;
}

void axis_json_object_set_new(axis_json_t *json, const char *field,
                             axis_json_t *value) {
  axis_ASSERT(json && axis_json_check_integrity(json), "Invalid argument.");
  axis_ASSERT(field, "Invalid argument.");
  axis_ASSERT(value && axis_json_check_integrity(value), "Invalid argument.");

  json_object_set_new(json, field, value);
}

void axis_json_array_append_new(axis_json_t *json, axis_json_t *value) {
  axis_ASSERT(json && axis_json_check_integrity(json), "Invalid argument.");
  axis_ASSERT(value && axis_json_check_integrity(value), "Invalid argument.");

  json_array_append_new(json, value);
}

axis_json_t *axis_json_object_peek(axis_json_t *json, const char *field) {
  axis_ASSERT(json && axis_json_check_integrity(json), "Invalid argument.");
  axis_ASSERT(field, "Invalid argument.");

  if (!json_is_object(json)) {
    return NULL;
  }

  return json_object_get(json, field);
}

const char *axis_json_to_string(axis_json_t *json, const char *field,
                               bool *must_free) {
  axis_ASSERT(json && axis_json_check_integrity(json), "Invalid argument.");
  axis_ASSERT(must_free, "Invalid argument.");

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
          axis_ASSERT(0, "Handle more types: %d", value->type);
          break;
      }
    }
  } else {
    *must_free = true;  // json_dumps will invoke malloc
    return json_dumps(json, JSON_ENCODE_ANY);
  }

  return NULL;
}

bool axis_json_object_del(axis_json_t *json, const char *field) {
  axis_ASSERT(json && axis_json_check_integrity(json), "Invalid argument.");
  axis_ASSERT(field, "Invalid argument.");

  if (axis_json_object_peek(json, field)) {
    // 0: success, -1: error
    return json_object_del(json, field) == 0;
  }

  return false;
}

axis_json_t *axis_json_from_string(const char *msg, axis_error_t *err) {
  json_error_t error;
  json_t *ret = NULL;

  if (!msg || !*msg) {
    return NULL;
  }
  ret = json_loads(msg, JSON_DECODE_ANY, &error);
  if (!ret) {
    if (err) {
      axis_error_set(err, axis_ERRNO_INVALID_JSON, "%s: %s", msg, error.text);
    }
    axis_LOGE("Failed to parse %s: %s", msg, error.text);
  }

  return ret;
}

void axis_json_destroy(axis_json_t *json) {
  axis_ASSERT(json && axis_json_check_integrity(json), "Invalid argument.");

  if (json) {
    json_decref(json);
  }
}

bool axis_json_is_object(axis_json_t *json) {
  axis_ASSERT(json && axis_json_check_integrity(json), "Invalid argument.");
  return json_is_object(json);
}

bool axis_json_is_array(axis_json_t *json) {
  axis_ASSERT(json && axis_json_check_integrity(json), "Invalid argument.");
  return json_is_array(json);
}

bool axis_json_is_string(axis_json_t *json) {
  axis_ASSERT(json && axis_json_check_integrity(json), "Invalid argument.");
  return json_is_string(json);
}

bool axis_json_is_integer(axis_json_t *json) {
  axis_ASSERT(json && axis_json_check_integrity(json), "Invalid argument.");
  return json_is_integer(json);
}

bool axis_json_is_boolean(axis_json_t *json) {
  axis_ASSERT(json && axis_json_check_integrity(json), "Invalid argument.");
  return json_is_boolean(json);
}

bool axis_json_is_real(axis_json_t *json) {
  axis_ASSERT(json && axis_json_check_integrity(json), "Invalid argument.");
  return json_is_real(json);
}

bool axis_json_is_null(axis_json_t *json) {
  axis_ASSERT(json && axis_json_check_integrity(json), "Invalid argument.");
  return json_is_null(json);
}

bool axis_json_is_true(axis_json_t *json) {
  axis_ASSERT(json && axis_json_check_integrity(json), "Invalid argument.");
  return json_is_true(json);
}

const char *axis_json_peek_string_value(axis_json_t *json) {
  axis_ASSERT(json && axis_json_check_integrity(json), "Invalid argument.");
  return json_string_value(json);
}

int64_t axis_json_get_integer_value(axis_json_t *json) {
  axis_ASSERT(json && axis_json_check_integrity(json), "Invalid argument.");
  return json_integer_value(json);
}

bool axis_json_get_boolean_value(axis_json_t *json) {
  axis_ASSERT(json && axis_json_check_integrity(json), "Invalid argument.");
  return json_boolean_value(json);
}

double axis_json_get_real_value(axis_json_t *json) {
  axis_ASSERT(json && axis_json_check_integrity(json), "Invalid argument.");
  return json_real_value(json);
}

double axis_json_get_number_value(axis_json_t *json) {
  axis_ASSERT(json && axis_json_check_integrity(json), "Invalid argument.");
  return json_number_value(json);
}

axis_json_t *axis_json_create_object(void) {
  json_t *json = json_object();
  axis_ASSERT(json && axis_json_check_integrity(json), "Should not happen.");
  return json;
}

axis_json_t *axis_json_create_array(void) {
  json_t *json = json_array();
  axis_ASSERT(json && axis_json_check_integrity(json), "Should not happen.");
  return json;
}

axis_json_t *axis_json_create_string(const char *str) {
  axis_ASSERT(str, "Invalid argument.");

  json_t *json = json_string(str);
  axis_ASSERT(json && axis_json_check_integrity(json), "Should not happen.");

  return json;
}

axis_json_t *axis_json_create_integer(int64_t value) {
  json_t *json = json_integer(value);
  axis_ASSERT(json && axis_json_check_integrity(json), "Should not happen.");
  return json;
}

axis_json_t *axis_json_create_real(double value) {
  json_t *json = json_real(value);
  axis_ASSERT(json && axis_json_check_integrity(json), "Should not happen.");
  return json;
}

axis_json_t *axis_json_create_boolean(bool value) {
  json_t *json = json_boolean(value);
  axis_ASSERT(json && axis_json_check_integrity(json), "Should not happen.");
  return json;
}

axis_json_t *axis_json_create_true(void) {
  json_t *json = json_true();
  axis_ASSERT(json && axis_json_check_integrity(json), "Should not happen.");
  return json;
}

axis_json_t *axis_json_create_false(void) {
  json_t *json = json_false();
  axis_ASSERT(json && axis_json_check_integrity(json), "Should not happen.");
  return json;
}

axis_json_t *axis_json_create_null(void) {
  json_t *json = json_null();
  axis_ASSERT(json && axis_json_check_integrity(json), "Should not happen.");
  return json;
}

size_t axis_json_array_get_size(axis_json_t *json) {
  axis_ASSERT(json && axis_json_check_integrity(json), "Invalid argument.");
  return json_array_size(json);
}

axis_json_t *axis_json_array_peek_item(axis_json_t *json, size_t index) {
  axis_ASSERT(json && axis_json_check_integrity(json), "Invalid argument.");
  return json_array_get(json, index);
}

void axis_json_object_update_missing(axis_json_t *json, axis_json_t *other) {
  axis_ASSERT(json && axis_json_check_integrity(json), "Invalid argument.");
  axis_ASSERT(other && axis_json_check_integrity(other), "Invalid argument.");
  json_object_update_missing(json, other);
}

const char *axis_json_object_iter_key(void *iter) {
  return json_object_iter_key(iter);
}

axis_json_t *axis_json_object_iter_value(void *iter) {
  return json_object_iter_value(iter);
}

void *axis_json_object_iter(axis_json_t *json) {
  axis_ASSERT(json && axis_json_check_integrity(json), "Invalid argument.");
  return json_object_iter(json);
}

void *axis_json_object_iter_at(axis_json_t *json, const char *key) {
  axis_ASSERT(json && axis_json_check_integrity(json), "Invalid argument.");
  return json_object_iter_at(json, key);
}

void *axis_json_object_key_to_iter(const char *key) {
  return json_object_key_to_iter(key);
}

void *axis_json_object_iter_next(axis_json_t *json, void *iter) {
  axis_ASSERT(json && axis_json_check_integrity(json), "Invalid argument.");
  return json_object_iter_next(json, iter);
}

axis_json_t *axis_json_incref(axis_json_t *json) {
  axis_ASSERT(json && axis_json_check_integrity(json), "Invalid argument.");
  return json_incref(json);
}
