//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "axis_utils/lib/error.h"
#include "axis_utils/value/type.h"

typedef struct json_t json_t;
typedef json_t axis_json_t;

#define axis_json_array_foreach(array, index, value)                     \
  for ((index) = 0; (index) < axis_json_array_get_size(array) &&         \
                    ((value) = axis_json_array_peek_item(array, index)); \
       (index)++)

#define axis_json_object_foreach(object, key, value)                    \
  for ((key) = axis_json_object_iter_key(axis_json_object_iter(object)); \
       (key) && ((value) = axis_json_object_iter_value(                 \
                     axis_json_object_key_to_iter(key)));               \
       (key) = axis_json_object_iter_key(axis_json_object_iter_next(     \
           object, axis_json_object_key_to_iter(key))))

axis_UTILS_API bool axis_json_check_integrity(axis_json_t *json);

/**
 * @brief delete the item from json object specified by key.
 * @param json json object
 * @param field key name
 * @return true if key exists and delete successful, false otherwise
 */
axis_UTILS_API bool axis_json_object_del(axis_json_t *json, const char *field);

axis_UTILS_API axis_TYPE axis_json_get_type(axis_json_t *json);

/**
 * @brief get string value from json object
 * @param json json object
 * @param field key
 * @return value if exists, NULL otherwise
 */
axis_UTILS_API const char *axis_json_object_peek_string(axis_json_t *json,
                                                      const char *field);

/**
 * @brief get int value from json object
 * @param json json object
 * @param field key
 * @return value if exists, -1 otherwise
 */
axis_UTILS_API int64_t axis_json_object_get_integer(axis_json_t *json,
                                                  const char *field);

/**
 * @brief get floating-point value from json object
 * @param json json object
 * @param field key
 * @return value if exists, 0 otherwise
 */
axis_UTILS_API double axis_json_object_get_real(axis_json_t *json,
                                              const char *field);

/**
 * @brief get boolean value from json object
 * @param json json object
 * @param field key
 * @return value if exists, false otherwise
 */
axis_UTILS_API bool axis_json_object_get_boolean(axis_json_t *json,
                                               const char *field);

axis_UTILS_API axis_json_t *axis_json_object_peek_array(axis_json_t *json,
                                                     const char *field);

axis_UTILS_API axis_json_t *axis_json_object_peek_array_forcibly(
    axis_json_t *json, const char *field);

axis_UTILS_API axis_json_t *axis_json_object_peek_object(axis_json_t *json,
                                                      const char *field);

axis_UTILS_API axis_json_t *axis_json_object_peek_object_forcibly(
    axis_json_t *json, const char *field);

axis_UTILS_API void axis_json_object_set_new(axis_json_t *json, const char *field,
                                           axis_json_t *value);

axis_UTILS_API void axis_json_array_append_new(axis_json_t *json,
                                             axis_json_t *value);

/**
 * @brief check if json object contains a field
 * @param json json object
 * @param field key
 * @return non-NULL if exists, NULL otherwise
 */
axis_UTILS_API axis_json_t *axis_json_object_peek(axis_json_t *json,
                                               const char *field);

/**
 * @brief Get value of field from json object in string format, if the type
 *        of field is not string, dumps the value in string format.
 * @param json The json object.
 * @param field The field name.
 * @param must_free have to free after use.
 * @return The json string of field value if exists, or NULL.
 */
axis_UTILS_API const char *axis_json_to_string(axis_json_t *json,
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
axis_UTILS_API axis_json_t *axis_json_from_string(const char *msg,
                                               axis_error_t *err);

/**
 * @brief Destroy a json object
 *
 * @param json The json object
 */
axis_UTILS_API void axis_json_destroy(axis_json_t *json);

axis_UTILS_API bool axis_json_is_object(axis_json_t *json);

axis_UTILS_API bool axis_json_is_array(axis_json_t *json);

axis_UTILS_API bool axis_json_is_string(axis_json_t *json);

axis_UTILS_API bool axis_json_is_integer(axis_json_t *json);

axis_UTILS_API bool axis_json_is_boolean(axis_json_t *json);

axis_UTILS_API bool axis_json_is_real(axis_json_t *json);

axis_UTILS_API bool axis_json_is_true(axis_json_t *json);

axis_UTILS_API bool axis_json_is_null(axis_json_t *json);

axis_UTILS_API const char *axis_json_peek_string_value(axis_json_t *json);

axis_UTILS_API int64_t axis_json_get_integer_value(axis_json_t *json);

axis_UTILS_API bool axis_json_get_boolean_value(axis_json_t *json);

axis_UTILS_API double axis_json_get_real_value(axis_json_t *json);

axis_UTILS_API double axis_json_get_number_value(axis_json_t *json);

axis_UTILS_API axis_json_t *axis_json_create_object(void);

axis_UTILS_API axis_json_t *axis_json_create_array(void);

axis_UTILS_API axis_json_t *axis_json_create_string(const char *str);

axis_UTILS_API axis_json_t *axis_json_create_integer(int64_t value);

axis_UTILS_API axis_json_t *axis_json_create_real(double value);

axis_UTILS_API axis_json_t *axis_json_create_boolean(bool value);

axis_UTILS_API axis_json_t *axis_json_create_true(void);

axis_UTILS_API axis_json_t *axis_json_create_false(void);

axis_UTILS_API axis_json_t *axis_json_create_null(void);

axis_UTILS_API size_t axis_json_array_get_size(axis_json_t *json);

axis_UTILS_API axis_json_t *axis_json_array_peek_item(axis_json_t *json,
                                                   size_t index);

axis_UTILS_API void axis_json_object_update_missing(axis_json_t *json,
                                                  axis_json_t *other);

axis_UTILS_API const char *axis_json_object_iter_key(void *iter);

axis_UTILS_API axis_json_t *axis_json_object_iter_value(void *iter);

axis_UTILS_API void *axis_json_object_iter(axis_json_t *json);

axis_UTILS_API void *axis_json_object_iter_at(axis_json_t *json, const char *key);

axis_UTILS_API void *axis_json_object_key_to_iter(const char *key);

axis_UTILS_API void *axis_json_object_iter_next(axis_json_t *json, void *iter);

axis_UTILS_API axis_json_t *axis_json_incref(axis_json_t *json);
