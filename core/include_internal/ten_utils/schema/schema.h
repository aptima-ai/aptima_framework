//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_utils/ten_config.h"

#include "ten_utils/container/hash_table.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/lib/signature.h"
#include "ten_utils/value/value.h"

#define TEN_SCHEMA_SIGNATURE 0x4D9FEA8F6273C974U

typedef struct ten_schema_keyword_type_t ten_schema_keyword_type_t;

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
// And it is represented by a ten_schema_keyword_type_t.
//
// - The children of the schema, ex: the properties of an object, the items of
// an array. There is no children for a primitive type.
//
// - The validation rules, which are represented by the ten_schema_keyword_t.
// Ex: the minimum and maximum value of an integer, the length of a string.
//
// The schema structure for each type of value is different, so we prefer to
// define different schema types. The ten_schema_t is the base class of all the
// schema types. The relationship between the schema types is shown below:
//
//                         ten_schema_t
//                               ^
//                               |
//                ---------------+----------------
//                |              |               |
// ten_schema_primitive_t ten_schema_object_t ten_schema_array_t
typedef struct ten_schema_t {
  ten_signature_t signature;

  // Key is TEN_SCHEMA_KEYWORD, the type of the value is ten_schema_keyword_t.
  // All keywords bound to the schema are stored in the 'keywords' map, and all
  // of them will be destroyed when the map is destroyed.
  ten_hashtable_t keywords;  // TEN_SCHEMA_KEYWORD -> ten_schema_keyword_t

  // This 'keyword_type' field is a caching mechanism, and the
  // ten_schema_keyword_type_t it points to is a ten_schema_keyword_t, which
  // exists in the above 'keywords' field. By directly accessing the
  // corresponding schema_keyword_t through this keyword_type field, it avoids
  // searching the 'keywords' field above, thus improving efficiency. A similar
  // mechanism can also be seen in other derivative schema_t of ten_schema_t,
  // such as ten_schema_object_t and ten_schema_array_t. Essentially, the
  // 'keywords' above play the role of resource management, while the individual
  // fields below serve as caches.
  ten_schema_keyword_type_t *keyword_type;
} ten_schema_t;

TEN_UTILS_PRIVATE_API bool ten_schema_check_integrity(ten_schema_t *self);

TEN_UTILS_PRIVATE_API void ten_schema_init(ten_schema_t *self);

TEN_UTILS_PRIVATE_API void ten_schema_deinit(ten_schema_t *self);

TEN_UTILS_PRIVATE_API ten_schema_t *ten_schema_create_from_json_string(
    const char *json_string, const char **err_msg);

TEN_UTILS_PRIVATE_API bool ten_schema_adjust_and_validate_json_string(
    ten_schema_t *self, const char *json_string, const char **err_msg);

TEN_UTILS_API ten_schema_t *ten_schema_create_from_json(ten_json_t *json);

TEN_UTILS_API ten_schema_t *ten_schema_create_from_value(ten_value_t *value);

TEN_UTILS_API void ten_schema_destroy(ten_schema_t *self);

TEN_UTILS_API bool ten_schema_validate_value(ten_schema_t *self,
                                             ten_value_t *value,
                                             ten_error_t *err);

TEN_UTILS_API bool ten_schema_adjust_value_type(ten_schema_t *self,
                                                ten_value_t *value,
                                                ten_error_t *err);

TEN_UTILS_API bool ten_schema_is_compatible(ten_schema_t *self,
                                            ten_schema_t *target,
                                            ten_error_t *err);
