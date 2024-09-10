//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_utils/ten_config.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ten_utils/lib/error.h"
#include "ten_utils/value/type.h"

typedef struct json_t json_t;
typedef json_t ten_json_t;

#define ten_json_array_foreach(array, index, value)                     \
  for ((index) = 0; (index) < ten_json_array_get_size(array) &&         \
                    ((value) = ten_json_array_peek_item(array, index)); \
       (index)++)

#define ten_json_object_foreach(object, key, value)                    \
  for ((key) = ten_json_object_iter_key(ten_json_object_iter(object)); \
       (key) && ((value) = ten_json_object_iter_value(                 \
                     ten_json_object_key_to_iter(key)));               \
       (key) = ten_json_object_iter_key(ten_json_object_iter_next(     \
           object, ten_json_object_key_to_iter(key))))

TEN_UTILS_API bool ten_json_check_integrity(ten_json_t *json);

/**
 * @brief delete the item from json object specified by key.
 * @param json json object
 * @param field key name
 * @return true if key exists and delete successful, false otherwise
 */
TEN_UTILS_API bool ten_json_object_del(ten_json_t *json, const char *field);

TEN_UTILS_API TEN_TYPE ten_json_get_type(ten_json_t *json);

/**
 * @brief get string value from json object
 * @param json json object
 * @param field key
 * @return value if exists, NULL otherwise
 */
TEN_UTILS_API const char *ten_json_object_peek_string(ten_json_t *json,
                                                      const char *field);

/**
 * @brief get int value from json object
 * @param json json object
 * @param field key
 * @return value if exists, -1 otherwise
 */
TEN_UTILS_API int64_t ten_json_object_get_integer(ten_json_t *json,
                                                  const char *field);

/**
 * @brief get floating-point value from json object
 * @param json json object
 * @param field key
 * @return value if exists, 0 otherwise
 */
TEN_UTILS_API double ten_json_object_get_real(ten_json_t *json,
                                              const char *field);

/**
 * @brief get boolean value from json object
 * @param json json object
 * @param field key
 * @return value if exists, false otherwise
 */
TEN_UTILS_API bool ten_json_object_get_boolean(ten_json_t *json,
                                               const char *field);

TEN_UTILS_API ten_json_t *ten_json_object_peek_array(ten_json_t *json,
                                                     const char *field);

TEN_UTILS_API ten_json_t *ten_json_object_peek_array_forcibly(
    ten_json_t *json, const char *field);

TEN_UTILS_API ten_json_t *ten_json_object_peek_object(ten_json_t *json,
                                                      const char *field);

TEN_UTILS_API ten_json_t *ten_json_object_peek_object_forcibly(
    ten_json_t *json, const char *field);

TEN_UTILS_API void ten_json_object_set_new(ten_json_t *json, const char *field,
                                           ten_json_t *value);

TEN_UTILS_API void ten_json_array_append_new(ten_json_t *json,
                                             ten_json_t *value);

/**
 * @brief check if json object contains a field
 * @param json json object
 * @param field key
 * @return non-NULL if exists, NULL otherwise
 */
TEN_UTILS_API ten_json_t *ten_json_object_peek(ten_json_t *json,
                                               const char *field);

/**
 * @brief Get value of field from json object in string format, if the type
 *        of field is not string, dumps the value in string format.
 * @param json The json object.
 * @param field The field name.
 * @param must_free have to free after use.
 * @return The json string of field value if exists, or NULL.
 */
TEN_UTILS_API const char *ten_json_to_string(ten_json_t *json,
                                             const char *field,
                                             bool *must_free);

/**
 * @brief Get value of field from json object in string format, if the type
 *        of field is not string, dumps the value in string format.
 * @param json The json object.
 * @param field The field name.
 * @param must_free have to free after use.
 * @return The json string of field value if exists, or NULL.
 */
TEN_UTILS_API ten_json_t *ten_json_from_string(const char *msg,
                                               ten_error_t *err);

/**
 * @brief Destroy a json object
 *
 * @param json The json object
 */
TEN_UTILS_API void ten_json_destroy(ten_json_t *json);

TEN_UTILS_API bool ten_json_is_object(ten_json_t *json);

TEN_UTILS_API bool ten_json_is_array(ten_json_t *json);

TEN_UTILS_API bool ten_json_is_string(ten_json_t *json);

TEN_UTILS_API bool ten_json_is_integer(ten_json_t *json);

TEN_UTILS_API bool ten_json_is_boolean(ten_json_t *json);

TEN_UTILS_API bool ten_json_is_real(ten_json_t *json);

TEN_UTILS_API bool ten_json_is_true(ten_json_t *json);

TEN_UTILS_API bool ten_json_is_null(ten_json_t *json);

TEN_UTILS_API const char *ten_json_peek_string_value(ten_json_t *json);

TEN_UTILS_API int64_t ten_json_get_integer_value(ten_json_t *json);

TEN_UTILS_API bool ten_json_get_boolean_value(ten_json_t *json);

TEN_UTILS_API double ten_json_get_real_value(ten_json_t *json);

TEN_UTILS_API double ten_json_get_number_value(ten_json_t *json);

TEN_UTILS_API ten_json_t *ten_json_create_object(void);

TEN_UTILS_API ten_json_t *ten_json_create_array(void);

TEN_UTILS_API ten_json_t *ten_json_create_string(const char *str);

TEN_UTILS_API ten_json_t *ten_json_create_integer(int64_t value);

TEN_UTILS_API ten_json_t *ten_json_create_real(double value);

TEN_UTILS_API ten_json_t *ten_json_create_boolean(bool value);

TEN_UTILS_API ten_json_t *ten_json_create_true(void);

TEN_UTILS_API ten_json_t *ten_json_create_false(void);

TEN_UTILS_API ten_json_t *ten_json_create_null(void);

TEN_UTILS_API size_t ten_json_array_get_size(ten_json_t *json);

TEN_UTILS_API ten_json_t *ten_json_array_peek_item(ten_json_t *json,
                                                   size_t index);

TEN_UTILS_API void ten_json_object_update_missing(ten_json_t *json,
                                                  ten_json_t *other);

TEN_UTILS_API const char *ten_json_object_iter_key(void *iter);

TEN_UTILS_API ten_json_t *ten_json_object_iter_value(void *iter);

TEN_UTILS_API void *ten_json_object_iter(ten_json_t *json);

TEN_UTILS_API void *ten_json_object_iter_at(ten_json_t *json, const char *key);

TEN_UTILS_API void *ten_json_object_key_to_iter(const char *key);

TEN_UTILS_API void *ten_json_object_iter_next(ten_json_t *json, void *iter);

TEN_UTILS_API ten_json_t *ten_json_incref(ten_json_t *json);
