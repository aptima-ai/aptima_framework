//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include "axis_utils/container/hash_table.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/value/value.h"

#define axis_SCHEMA_SIGNATURE 0x4D9FEA8F6273C974U
#define axis_SCHEMA_ERROR_SIGNATURE 0x32B696D4FC8FFD09U

typedef struct axis_schema_keyword_type_t axis_schema_keyword_type_t;

// A schema definition is something that describes the structure of a TEN value.
//
// The followings are some schema examples:
//
// {
//   "type": "object",
//   "properties": {
//     "a": {
//       "type": "string"
//     },
//     "b": {
//       "type": "uint16"
//     }
//   }
// }
//
// {
//   "type": "array",
//   "items": {
//     "type": "int64"
//   }
// }
//
// It mainly consists of the following three parts:
//
// - The type of the schema, which is corresponding to the type of the value.
// And it is represented by a axis_schema_keyword_type_t.
//
// - The children of the schema, ex: the properties of an object, the items of
// an array. There is no children for a primitive type.
//
// - The validation rules, which are represented by the axis_schema_keyword_t.
// Ex: the minimum and maximum value of an integer, the length of a string.
//
// The schema structure for each type of value is different, so we prefer to
// define different schema types. The axis_schema_t is the base class of all the
// schema types. The relationship between the schema types is shown below:
//
//                         axis_schema_t
//                               ^
//                               |
//                ---------------+----------------
//                |              |               |
// axis_schema_primitive_t axis_schema_object_t axis_schema_array_t
typedef struct axis_schema_t {
  axis_signature_t signature;

  // Key is axis_SCHEMA_KEYWORD, the type of the value is axis_schema_keyword_t.
  // All keywords bound to the schema are stored in the 'keywords' map, and all
  // of them will be destroyed when the map is destroyed.
  axis_hashtable_t keywords;  // axis_SCHEMA_KEYWORD -> axis_schema_keyword_t

  // This 'keyword_type' field is a caching mechanism, and the
  // axis_schema_keyword_type_t it points to is a axis_schema_keyword_t, which
  // exists in the above 'keywords' field. By directly accessing the
  // corresponding schema_keyword_t through this keyword_type field, it avoids
  // searching the 'keywords' field above, thus improving efficiency. A similar
  // mechanism can also be seen in other derivative schema_t of axis_schema_t,
  // such as axis_schema_object_t and axis_schema_array_t. Essentially, the
  // 'keywords' above play the role of resource management, while the individual
  // fields below serve as caches.
  axis_schema_keyword_type_t *keyword_type;
} axis_schema_t;

// Internal use.
//
// The error context to be used during the schema validation process. An example
// of schema is as follows:
//
// {
//   "type": "object",
//   "properties": {
//     "a": {
//       "type": "array",
//       "items": {
//         "type": "int32"
//       }
//     }
//   }
// }
//
// And the value to be validated is as follows:
//
// {
//   "a": [1, "2", 3]
// }
//
// Verify each value according to its corresponding schema in DFS order, until
// an error is encountered. After the validation, the error message should
// display which value is invalid, ex: it will be `a[1]` in this case. We need
// to record the path of the schema during the process, besides the error
// message. We can not use axis_error_t to record the path, because there is no
// space to achieve this. Neither can we use the `axis_schema_t` to record the
// path, because we need to know the index, if the value is an array; but there
// is no index information in the axis_schema_array_t because each item in the
// array shares the same schema.
typedef struct axis_schema_error_t {
  axis_signature_t signature;
  axis_error_t *err;
  axis_string_t path;
} axis_schema_error_t;

axis_UTILS_PRIVATE_API bool axis_schema_error_check_integrity(
    axis_schema_error_t *self);

axis_UTILS_PRIVATE_API void axis_schema_error_init(axis_schema_error_t *self,
                                                 axis_error_t *err);

axis_UTILS_PRIVATE_API void axis_schema_error_deinit(axis_schema_error_t *self);

axis_UTILS_PRIVATE_API void axis_schema_error_reset(axis_schema_error_t *self);

axis_UTILS_PRIVATE_API bool axis_schema_check_integrity(axis_schema_t *self);

axis_UTILS_PRIVATE_API void axis_schema_init(axis_schema_t *self);

axis_UTILS_PRIVATE_API void axis_schema_deinit(axis_schema_t *self);

axis_UTILS_PRIVATE_API axis_schema_t *axis_schema_create_from_json_str(
    const char *json_str, const char **err_msg);

axis_UTILS_PRIVATE_API bool axis_schema_adjust_and_validate_json_str(
    axis_schema_t *self, const char *json_str, const char **err_msg);

axis_UTILS_API axis_schema_t *axis_schema_create_from_json(axis_json_t *json);

axis_UTILS_API axis_schema_t *axis_schema_create_from_value(axis_value_t *value);

axis_UTILS_API void axis_schema_destroy(axis_schema_t *self);

axis_UTILS_PRIVATE_API bool axis_schema_validate_value_with_schema_error(
    axis_schema_t *self, axis_value_t *value, axis_schema_error_t *schema_err);

axis_UTILS_API bool axis_schema_validate_value(axis_schema_t *self,
                                             axis_value_t *value,
                                             axis_error_t *err);

axis_UTILS_PRIVATE_API bool axis_schema_adjust_value_type_with_schema_error(
    axis_schema_t *self, axis_value_t *value, axis_schema_error_t *schema_err);

axis_UTILS_API bool axis_schema_adjust_value_type(axis_schema_t *self,
                                                axis_value_t *value,
                                                axis_error_t *err);

axis_UTILS_PRIVATE_API bool axis_schema_is_compatible_with_schema_error(
    axis_schema_t *self, axis_schema_t *target, axis_schema_error_t *schema_err);

axis_UTILS_API bool axis_schema_is_compatible(axis_schema_t *self,
                                            axis_schema_t *target,
                                            axis_error_t *err);
