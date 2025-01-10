//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include <stdbool.h>
#include <stdint.h>

#include "aptima_utils/container/list.h"
#include "aptima_utils/lib/buf.h"
#include "aptima_utils/lib/error.h"
#include "aptima_utils/lib/signature.h"
#include "aptima_utils/lib/string.h"
#include "aptima_utils/value/type.h"
#include "aptima_utils/value/value_get.h"     // IWYU pragma: keep
#include "aptima_utils/value/value_is.h"      // IWYU pragma: keep
#include "aptima_utils/value/value_json.h"    // IWYU pragma: keep
#include "aptima_utils/value/value_kv.h"      // IWYU pragma: keep
#include "aptima_utils/value/value_merge.h"   // IWYU pragma: keep
#include "aptima_utils/value/value_object.h"  // IWYU pragma: keep
#include "aptima_utils/value/value_string.h"  // IWYU pragma: keep

typedef enum aptima_VALUE_ERROR {
  aptima_VALUE_ERROR_UNSUPPORTED_TYPE_CONVERSION = 1,
} aptima_VALUE_ERROR;

#define aptima_value_object_foreach(value, iter) \
  aptima_list_foreach (&((value)->content.object), iter)

#define aptima_value_array_foreach(value, iter) \
  aptima_list_foreach (&((value)->content.array), iter)

typedef struct aptima_value_t aptima_value_t;

typedef union aptima_value_union_t {
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

  aptima_list_t object;  // The element type is 'aptima_value_kv_t*'
  aptima_list_t array;   // The element type is 'aptima_value_t*'
  aptima_string_t string;
  aptima_buf_t buf;
} aptima_value_union_t;

typedef bool (*aptima_value_construct_func_t)(aptima_value_t *value,
                                           aptima_error_t *err);

typedef bool (*aptima_value_copy_func_t)(aptima_value_t *dest, aptima_value_t *src,
                                      aptima_error_t *err);

typedef bool (*aptima_value_destruct_func_t)(aptima_value_t *value, aptima_error_t *err);

typedef struct aptima_value_t {
  aptima_signature_t signature;

  /**
   * @brief The name of the value. Mainly for debug purpose.
   */
  aptima_string_t *name;

  aptima_TYPE type;
  aptima_value_union_t content;

  aptima_value_construct_func_t construct;
  aptima_value_copy_func_t copy;
  aptima_value_destruct_func_t destruct;
} aptima_value_t;

aptima_UTILS_API bool aptima_value_check_integrity(aptima_value_t *self);

aptima_UTILS_API aptima_value_t *aptima_value_clone(aptima_value_t *src);
aptima_UTILS_API bool aptima_value_copy(aptima_value_t *src, aptima_value_t *dest);

aptima_UTILS_API bool aptima_value_init_invalid(aptima_value_t *self);
aptima_UTILS_API bool aptima_value_init_int8(aptima_value_t *self, int8_t value);
aptima_UTILS_API bool aptima_value_init_int16(aptima_value_t *self, int16_t value);
aptima_UTILS_API bool aptima_value_init_int32(aptima_value_t *self, int32_t value);
aptima_UTILS_API bool aptima_value_init_int64(aptima_value_t *self, int64_t value);
aptima_UTILS_API bool aptima_value_init_uint8(aptima_value_t *self, uint8_t value);
aptima_UTILS_API bool aptima_value_init_uint16(aptima_value_t *self, uint16_t value);
aptima_UTILS_API bool aptima_value_init_uint32(aptima_value_t *self, uint32_t value);
aptima_UTILS_API bool aptima_value_init_uint64(aptima_value_t *self, uint64_t value);
aptima_UTILS_API bool aptima_value_init_float32(aptima_value_t *self, float value);
aptima_UTILS_API bool aptima_value_init_float64(aptima_value_t *self, double value);
aptima_UTILS_API bool aptima_value_init_bool(aptima_value_t *self, bool value);
aptima_UTILS_API bool aptima_value_init_null(aptima_value_t *self);
aptima_UTILS_API bool aptima_value_init_string(aptima_value_t *self);
aptima_UTILS_API bool aptima_value_init_string_with_size(aptima_value_t *self,
                                                   const char *str, size_t len);
aptima_UTILS_API bool aptima_value_init_buf(aptima_value_t *self, size_t size);

/**
 * @note Note that the ownership of @a value is moved to the value @a self.
 */
aptima_UTILS_API bool aptima_value_init_object_with_move(aptima_value_t *self,
                                                   aptima_list_t *value);

/**
 * @note Note that the ownership of @a value is moved to the value @a self.
 */
aptima_UTILS_API bool aptima_value_init_array_with_move(aptima_value_t *self,
                                                  aptima_list_t *value);

aptima_UTILS_API aptima_value_t *aptima_value_create_invalid(void);
aptima_UTILS_API aptima_value_t *aptima_value_create_int8(int8_t value);
aptima_UTILS_API aptima_value_t *aptima_value_create_int16(int16_t value);
aptima_UTILS_API aptima_value_t *aptima_value_create_int32(int32_t value);
aptima_UTILS_API aptima_value_t *aptima_value_create_int64(int64_t value);
aptima_UTILS_API aptima_value_t *aptima_value_create_uint8(uint8_t value);
aptima_UTILS_API aptima_value_t *aptima_value_create_uint16(uint16_t value);
aptima_UTILS_API aptima_value_t *aptima_value_create_uint32(uint32_t value);
aptima_UTILS_API aptima_value_t *aptima_value_create_uint64(uint64_t value);
aptima_UTILS_API aptima_value_t *aptima_value_create_float32(float value);
aptima_UTILS_API aptima_value_t *aptima_value_create_float64(double value);
aptima_UTILS_API aptima_value_t *aptima_value_create_bool(bool value);
aptima_UTILS_API aptima_value_t *aptima_value_create_array_with_move(aptima_list_t *value);
aptima_UTILS_API aptima_value_t *aptima_value_create_object_with_move(aptima_list_t *value);
aptima_UTILS_API aptima_value_t *aptima_value_create_string_with_size(const char *str,
                                                             size_t len);
aptima_UTILS_API aptima_value_t *aptima_value_create_string(const char *str);
aptima_UTILS_API aptima_value_t *aptima_value_create_null(void);
aptima_UTILS_API aptima_value_t *aptima_value_create_ptr(
    void *ptr, aptima_value_construct_func_t construct, aptima_value_copy_func_t copy,
    aptima_value_destruct_func_t destruct);
aptima_UTILS_API aptima_value_t *aptima_value_create_buf_with_move(aptima_buf_t buf);

aptima_UTILS_API void aptima_value_deinit(aptima_value_t *self);

aptima_UTILS_API void aptima_value_destroy(aptima_value_t *self);

aptima_UTILS_API void aptima_value_reset_to_string_with_size(aptima_value_t *self,
                                                       const char *str,
                                                       size_t len);

aptima_UTILS_API void aptima_value_reset_to_ptr(aptima_value_t *self, void *ptr,
                                          aptima_value_construct_func_t construct,
                                          aptima_value_copy_func_t copy,
                                          aptima_value_destruct_func_t destruct);

aptima_UTILS_API void aptima_value_set_name(aptima_value_t *self, const char *fmt, ...);

aptima_UTILS_API size_t aptima_value_array_size(aptima_value_t *self);

aptima_UTILS_API bool aptima_value_is_valid(aptima_value_t *self);
