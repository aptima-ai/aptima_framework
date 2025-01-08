//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stdbool.h>
#include <stdint.h>

#include "axis_utils/container/list.h"
#include "axis_utils/lib/buf.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/value/type.h"
#include "axis_utils/value/value_get.h"     // IWYU pragma: keep
#include "axis_utils/value/value_is.h"      // IWYU pragma: keep
#include "axis_utils/value/value_json.h"    // IWYU pragma: keep
#include "axis_utils/value/value_kv.h"      // IWYU pragma: keep
#include "axis_utils/value/value_merge.h"   // IWYU pragma: keep
#include "axis_utils/value/value_object.h"  // IWYU pragma: keep
#include "axis_utils/value/value_string.h"  // IWYU pragma: keep

typedef enum axis_VALUE_ERROR {
  axis_VALUE_ERROR_UNSUPPORTED_TYPE_CONVERSION = 1,
} axis_VALUE_ERROR;

#define axis_value_object_foreach(value, iter) \
  axis_list_foreach (&((value)->content.object), iter)

#define axis_value_array_foreach(value, iter) \
  axis_list_foreach (&((value)->content.array), iter)

typedef struct axis_value_t axis_value_t;

typedef union axis_value_union_t {
  bool boolean;

  int8_t int8;
  int16_t int16;
  int32_t int32;
  int64_t int64;

  uint8_t uint8;
  uint16_t uint16;
  uint32_t uint32;
  uint64_t uint64;

  float float32;
  double float64;

  void *ptr;

  axis_list_t object;  // The element type is 'axis_value_kv_t*'
  axis_list_t array;   // The element type is 'axis_value_t*'
  axis_string_t string;
  axis_buf_t buf;
} axis_value_union_t;

typedef bool (*axis_value_construct_func_t)(axis_value_t *value,
                                           axis_error_t *err);

typedef bool (*axis_value_copy_func_t)(axis_value_t *dest, axis_value_t *src,
                                      axis_error_t *err);

typedef bool (*axis_value_destruct_func_t)(axis_value_t *value, axis_error_t *err);

typedef struct axis_value_t {
  axis_signature_t signature;

  /**
   * @brief The name of the value. Mainly for debug purpose.
   */
  axis_string_t *name;

  axis_TYPE type;
  axis_value_union_t content;

  axis_value_construct_func_t construct;
  axis_value_copy_func_t copy;
  axis_value_destruct_func_t destruct;
} axis_value_t;

axis_UTILS_API bool axis_value_check_integrity(axis_value_t *self);

axis_UTILS_API axis_value_t *axis_value_clone(axis_value_t *src);
axis_UTILS_API bool axis_value_copy(axis_value_t *src, axis_value_t *dest);

axis_UTILS_API bool axis_value_init_invalid(axis_value_t *self);
axis_UTILS_API bool axis_value_init_int8(axis_value_t *self, int8_t value);
axis_UTILS_API bool axis_value_init_int16(axis_value_t *self, int16_t value);
axis_UTILS_API bool axis_value_init_int32(axis_value_t *self, int32_t value);
axis_UTILS_API bool axis_value_init_int64(axis_value_t *self, int64_t value);
axis_UTILS_API bool axis_value_init_uint8(axis_value_t *self, uint8_t value);
axis_UTILS_API bool axis_value_init_uint16(axis_value_t *self, uint16_t value);
axis_UTILS_API bool axis_value_init_uint32(axis_value_t *self, uint32_t value);
axis_UTILS_API bool axis_value_init_uint64(axis_value_t *self, uint64_t value);
axis_UTILS_API bool axis_value_init_float32(axis_value_t *self, float value);
axis_UTILS_API bool axis_value_init_float64(axis_value_t *self, double value);
axis_UTILS_API bool axis_value_init_bool(axis_value_t *self, bool value);
axis_UTILS_API bool axis_value_init_null(axis_value_t *self);
axis_UTILS_API bool axis_value_init_string(axis_value_t *self);
axis_UTILS_API bool axis_value_init_string_with_size(axis_value_t *self,
                                                   const char *str, size_t len);
axis_UTILS_API bool axis_value_init_buf(axis_value_t *self, size_t size);

/**
 * @note Note that the ownership of @a value is moved to the value @a self.
 */
axis_UTILS_API bool axis_value_init_object_with_move(axis_value_t *self,
                                                   axis_list_t *value);

/**
 * @note Note that the ownership of @a value is moved to the value @a self.
 */
axis_UTILS_API bool axis_value_init_array_with_move(axis_value_t *self,
                                                  axis_list_t *value);

axis_UTILS_API axis_value_t *axis_value_create_invalid(void);
axis_UTILS_API axis_value_t *axis_value_create_int8(int8_t value);
axis_UTILS_API axis_value_t *axis_value_create_int16(int16_t value);
axis_UTILS_API axis_value_t *axis_value_create_int32(int32_t value);
axis_UTILS_API axis_value_t *axis_value_create_int64(int64_t value);
axis_UTILS_API axis_value_t *axis_value_create_uint8(uint8_t value);
axis_UTILS_API axis_value_t *axis_value_create_uint16(uint16_t value);
axis_UTILS_API axis_value_t *axis_value_create_uint32(uint32_t value);
axis_UTILS_API axis_value_t *axis_value_create_uint64(uint64_t value);
axis_UTILS_API axis_value_t *axis_value_create_float32(float value);
axis_UTILS_API axis_value_t *axis_value_create_float64(double value);
axis_UTILS_API axis_value_t *axis_value_create_bool(bool value);
axis_UTILS_API axis_value_t *axis_value_create_array_with_move(axis_list_t *value);
axis_UTILS_API axis_value_t *axis_value_create_object_with_move(axis_list_t *value);
axis_UTILS_API axis_value_t *axis_value_create_string_with_size(const char *str,
                                                             size_t len);
axis_UTILS_API axis_value_t *axis_value_create_string(const char *str);
axis_UTILS_API axis_value_t *axis_value_create_null(void);
axis_UTILS_API axis_value_t *axis_value_create_ptr(
    void *ptr, axis_value_construct_func_t construct, axis_value_copy_func_t copy,
    axis_value_destruct_func_t destruct);
axis_UTILS_API axis_value_t *axis_value_create_buf_with_move(axis_buf_t buf);

axis_UTILS_API void axis_value_deinit(axis_value_t *self);

axis_UTILS_API void axis_value_destroy(axis_value_t *self);

axis_UTILS_API void axis_value_reset_to_string_with_size(axis_value_t *self,
                                                       const char *str,
                                                       size_t len);

axis_UTILS_API void axis_value_reset_to_ptr(axis_value_t *self, void *ptr,
                                          axis_value_construct_func_t construct,
                                          axis_value_copy_func_t copy,
                                          axis_value_destruct_func_t destruct);

axis_UTILS_API void axis_value_set_name(axis_value_t *self, const char *fmt, ...);

axis_UTILS_API size_t axis_value_array_size(axis_value_t *self);

axis_UTILS_API bool axis_value_is_valid(axis_value_t *self);
