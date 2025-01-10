//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "aptima_utils/lib/error.h"
#include "aptima_utils/value/type.h"

typedef struct json_t json_t;
typedef json_t aptima_json_t;

#define aptima_json_array_foreach(array, index, value)                     \
  for ((index) = 0; (index) < aptima_json_array_get_size(array) &&         \
                    ((value) = aptima_json_array_peek_item(array, index)); \
       (index)++)

#define aptima_json_object_foreach(object, key, value)                    \
  for ((key) = aptima_json_object_iter_key(aptima_json_object_iter(object)); \
       (key) && ((value) = aptima_json_object_iter_value(                 \
                     aptima_json_object_key_to_iter(key)));               \
       (key) = aptima_json_object_iter_key(aptima_json_object_iter_next(     \
           object, aptima_json_object_key_to_iter(key))))

aptima_UTILS_API bool aptima_json_check_integrity(aptima_json_t *json);

/**
 * @brief delete the item from json object specified by key.
 * @param json json object
 * @param field key name
 * @return true if key exists and delete successful, false otherwise
 */
aptima_UTILS_API bool aptima_json_object_del(aptima_json_t *json, const char *field);

aptima_UTILS_API aptima_TYPE aptima_json_get_type(aptima_json_t *json);

/**
 * @brief get string value from json object
 * @param json json object
 * @param field key
 * @return value if exists, NULL otherwise
 */
aptima_UTILS_API const char *aptima_json_object_peek_string(aptima_json_t *json,
                                                      const char *field);

/**
 * @brief get int value from json object
 * @param json json object
 * @param field key
 * @return value if exists, -1 otherwise
 */
aptima_UTILS_API int64_t aptima_json_object_get_integer(aptima_json_t *json,
                                                  const char *field);

/**
 * @brief get floating-point value from json object
 * @param json json object
 * @param field key
 * @return value if exists, 0 otherwise
 */
aptima_UTILS_API double aptima_json_object_get_real(aptima_json_t *json,
                                              const char *field);

/**
 * @brief get boolean value from json object
 * @param json json object
 * @param field key
 * @return value if exists, false otherwise
 */
aptima_UTILS_API bool aptima_json_object_get_boolean(aptima_json_t *json,
                                               const char *field);

aptima_UTILS_API aptima_json_t *aptima_json_object_peek_array(aptima_json_t *json,
                                                     const char *field);

aptima_UTILS_API aptima_json_t *aptima_json_object_peek_array_forcibly(
    aptima_json_t *json, const char *field);

aptima_UTILS_API aptima_json_t *aptima_json_object_peek_object(aptima_json_t *json,
                                                      const char *field);

aptima_UTILS_API aptima_json_t *aptima_json_object_peek_object_forcibly(
    aptima_json_t *json, const char *field);

aptima_UTILS_API void aptima_json_object_set_new(aptima_json_t *json, const char *field,
                                           aptima_json_t *value);

aptima_UTILS_API void aptima_json_array_append_new(aptima_json_t *json,
                                             aptima_json_t *value);

/**
 * @brief check if json object contains a field
 * @param json json object
 * @param field key
 * @return non-NULL if exists, NULL otherwise
 */
aptima_UTILS_API aptima_json_t *aptima_json_object_peek(aptima_json_t *json,
                                               const char *field);

/**
 * @brief Get value of field from json object in string format, if the type
 *        of field is not string, dumps the value in string format.
 * @param json The json object.
 * @param field The field name.
 * @param must_free have to free after use.
 * @return The json string of field value if exists, or NULL.
 */
aptima_UTILS_API const char *aptima_json_to_string(aptima_json_t *json,
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
aptima_UTILS_API aptima_json_t *aptima_json_from_string(const char *msg,
                                               aptima_error_t *err);

/**
 * @brief Destroy a json object
 *
 * @param json The json object
 */
aptima_UTILS_API void aptima_json_destroy(aptima_json_t *json);

aptima_UTILS_API bool aptima_json_is_object(aptima_json_t *json);

aptima_UTILS_API bool aptima_json_is_array(aptima_json_t *json);

aptima_UTILS_API bool aptima_json_is_string(aptima_json_t *json);

aptima_UTILS_API bool aptima_json_is_integer(aptima_json_t *json);

aptima_UTILS_API bool aptima_json_is_boolean(aptima_json_t *json);

aptima_UTILS_API bool aptima_json_is_real(aptima_json_t *json);

aptima_UTILS_API bool aptima_json_is_true(aptima_json_t *json);

aptima_UTILS_API bool aptima_json_is_null(aptima_json_t *json);

aptima_UTILS_API const char *aptima_json_peek_string_value(aptima_json_t *json);

aptima_UTILS_API int64_t aptima_json_get_integer_value(aptima_json_t *json);

aptima_UTILS_API bool aptima_json_get_boolean_value(aptima_json_t *json);

aptima_UTILS_API double aptima_json_get_real_value(aptima_json_t *json);

aptima_UTILS_API double aptima_json_get_number_value(aptima_json_t *json);

aptima_UTILS_API aptima_json_t *aptima_json_create_object(void);

aptima_UTILS_API aptima_json_t *aptima_json_create_array(void);

aptima_UTILS_API aptima_json_t *aptima_json_create_string(const char *str);

aptima_UTILS_API aptima_json_t *aptima_json_create_integer(int64_t value);

aptima_UTILS_API aptima_json_t *aptima_json_create_real(double value);

aptima_UTILS_API aptima_json_t *aptima_json_create_boolean(bool value);

aptima_UTILS_API aptima_json_t *aptima_json_create_true(void);

aptima_UTILS_API aptima_json_t *aptima_json_create_false(void);

aptima_UTILS_API aptima_json_t *aptima_json_create_null(void);

aptima_UTILS_API size_t aptima_json_array_get_size(aptima_json_t *json);

aptima_UTILS_API aptima_json_t *aptima_json_array_peek_item(aptima_json_t *json,
                                                   size_t index);

aptima_UTILS_API void aptima_json_object_update_missing(aptima_json_t *json,
                                                  aptima_json_t *other);

aptima_UTILS_API const char *aptima_json_object_iter_key(void *iter);

aptima_UTILS_API aptima_json_t *aptima_json_object_iter_value(void *iter);

aptima_UTILS_API void *aptima_json_object_iter(aptima_json_t *json);

aptima_UTILS_API void *aptima_json_object_iter_at(aptima_json_t *json, const char *key);

aptima_UTILS_API void *aptima_json_object_key_to_iter(const char *key);

aptima_UTILS_API void *aptima_json_object_iter_next(aptima_json_t *json, void *iter);

aptima_UTILS_API aptima_json_t *aptima_json_incref(aptima_json_t *json);
