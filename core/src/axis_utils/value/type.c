//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/value/type.h"

#include <float.h>
#include <stddef.h>

#include "include_internal/axis_utils/value/type_info.h"
#include "axis_utils/container/list.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"

axis_TYPE axis_type_from_string(const char *type_str) {
  axis_ASSERT(type_str, "Invalid argument.");

  for (size_t i = 0; i < axis_types_info_size; ++i) {
    if (axis_types_info[i].name &&
        axis_c_string_is_equal(type_str, axis_types_info[i].name)) {
      return (axis_TYPE)i;
    }
  }

  return axis_TYPE_INVALID;
}

const char *axis_type_to_string(const axis_TYPE type) {
  return axis_types_info[type].name;
}

axis_list_t axis_type_from_json(axis_json_t *json) {
  axis_ASSERT(json, "Invalid argument.");

  axis_list_t result = axis_LIST_INIT_VAL;

  static_assert(sizeof(axis_TYPE) <= sizeof(int32_t),
                "axis_TYPE can not be larger than 32-bit.");

  if (axis_json_is_integer(json)) {
    if (axis_json_get_integer_value(json) >= INT32_MIN &&
        axis_json_get_integer_value(json) <= INT32_MAX) {
      axis_list_push_back(&result, axis_int32_listnode_create(axis_TYPE_INT32));
    }
    axis_list_push_back(&result, axis_int32_listnode_create(axis_TYPE_INT64));
  } else if (axis_json_is_real(json)) {
    if (axis_json_get_real_value(json) >= -FLT_MAX &&
        axis_json_get_real_value(json) <= FLT_MAX) {
      axis_list_push_back(&result, axis_int32_listnode_create(axis_TYPE_FLOAT32));
    }
    axis_list_push_back(&result, axis_int32_listnode_create(axis_TYPE_FLOAT64));
  } else if (axis_json_is_null(json)) {
    axis_list_push_back(&result, axis_int32_listnode_create(axis_TYPE_NULL));
  } else if (axis_json_is_string(json)) {
    axis_list_push_back(&result, axis_int32_listnode_create(axis_TYPE_STRING));
  } else if (axis_json_is_object(json)) {
    axis_list_push_back(&result, axis_int32_listnode_create(axis_TYPE_OBJECT));
  } else if (axis_json_is_array(json)) {
    axis_list_push_back(&result, axis_int32_listnode_create(axis_TYPE_ARRAY));
  } else if (axis_json_is_boolean(json)) {
    axis_list_push_back(&result, axis_int32_listnode_create(axis_TYPE_BOOL));
  } else {
    axis_ASSERT(0, "Handle more types.");
  }

  return result;
}

// When converting `axis_value_t` between different types, this function is used
// to check if the types before and after the conversion are compatible. If they
// are not compatible, the conversion will not proceed. This mechanism ensures
// that compatibility between `APTIMA` types throughout the entire `APTIMA` system is
// managed consistently by this function. Even in `APTIMA` Rust code, if type
// compatibility between `APTIMA` types needs to be checked, it will ultimately
// rely on this function.
bool axis_type_is_compatible(axis_TYPE actual, axis_TYPE expected) {
  if (actual == axis_TYPE_INVALID || expected == axis_TYPE_INVALID) {
    return false;
  }

  if (actual == expected) {
    return true;
  }

  if (axis_IS_INTEGER_TYPE(expected)) {
    return axis_IS_INTEGER_TYPE(actual);
  }

  if (axis_IS_FLOAT_TYPE(expected)) {
    return axis_IS_INTEGER_TYPE(actual) || axis_IS_FLOAT_TYPE(actual);
  }

  switch (expected) {
    case axis_TYPE_BOOL:
      return actual == axis_TYPE_BOOL;

    case axis_TYPE_PTR:
      return actual == axis_TYPE_PTR;

    case axis_TYPE_STRING:
      return actual == axis_TYPE_STRING;

    case axis_TYPE_ARRAY:
      return actual == axis_TYPE_ARRAY;

    case axis_TYPE_OBJECT:
      return actual == axis_TYPE_OBJECT;

    case axis_TYPE_BUF:
      return actual == axis_TYPE_BUF;

    case axis_TYPE_NULL:
      return actual == axis_TYPE_NULL;

    default:
      return false;
  }

  axis_ASSERT(0, "Should not happen.");
}
