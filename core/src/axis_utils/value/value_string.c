//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/value/value_string.h"

#include <errno.h>
#include <float.h>
#include <inttypes.h>

#include "include_internal/axis_utils/value/constant_str.h"
#include "include_internal/axis_utils/value/value_convert.h"
#include "axis_runtime/common/errno.h"
#include "axis_utils/container/list.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/value/type.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_get.h"
#include "axis_utils/value/value_is.h"
#include "axis_utils/value/value_kv.h"

static bool axis_value_array_to_string(axis_value_t *self, axis_string_t *str,
                                      axis_error_t *err) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");
  axis_ASSERT(axis_value_is_array(self), "Invalid argument: %d",
             axis_value_get_type(self));
  axis_ASSERT(str, "Invalid argument.");

  axis_string_append_formatted(str, "%s", "[");

  axis_list_foreach (&self->content.array, iter) {
    axis_value_t *item = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(item && axis_value_check_integrity(item), "Invalid argument.");

    if (iter.index > 0) {
      axis_string_append_formatted(str, "%s", ",");
    }

    axis_string_t item_str;
    axis_string_init(&item_str);
    if (!axis_value_to_string(item, &item_str, err)) {
      axis_string_deinit(&item_str);
      return false;
    }

    axis_string_append_formatted(str, "%s", axis_string_get_raw_str(&item_str));
    axis_string_deinit(&item_str);
  }

  axis_string_append_formatted(str, "%s", "]");

  return true;
}

static bool axis_value_object_to_string(axis_value_t *self, axis_string_t *str,
                                       axis_error_t *err) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");
  axis_ASSERT(axis_value_is_object(self), "Invalid argument: %d",
             axis_value_get_type(self));
  axis_ASSERT(str, "Invalid argument.");

  axis_string_append_formatted(str, "%s", "{");

  axis_list_foreach (&self->content.array, iter) {
    axis_value_kv_t *item = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(item && axis_value_kv_check_integrity(item), "Invalid argument.");

    if (iter.index > 0) {
      axis_string_append_formatted(str, "%s", ",");
    }

    axis_string_t *item_str = axis_value_kv_to_string(item, err);
    if (!item_str) {
      return false;
    }

    axis_string_append_formatted(str, "%s", item_str);
    axis_string_destroy(item_str);
  }

  axis_string_append_formatted(str, "%s", "}");

  return true;
}

bool axis_value_to_string(axis_value_t *self, axis_string_t *str,
                         axis_error_t *err) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");
  axis_ASSERT(str, "Invalid argument.");

  switch (self->type) {
    case axis_TYPE_INVALID:
      axis_ASSERT(0, "Should not happen.");
      break;
    case axis_TYPE_INT8:
      axis_string_append_formatted(str, "%d", self->content.int8);
      break;
    case axis_TYPE_INT16:
      axis_string_append_formatted(str, "%d", self->content.int16);
      break;
    case axis_TYPE_INT32:
      axis_string_append_formatted(str, "%d", self->content.int32);
      break;
    case axis_TYPE_INT64:
      axis_string_append_formatted(str, "%ld", self->content.int64);
      break;
    case axis_TYPE_UINT8:
      axis_string_append_formatted(str, "%u", self->content.uint8);
      break;
    case axis_TYPE_UINT16:
      axis_string_append_formatted(str, "%u", self->content.uint16);
      break;
    case axis_TYPE_UINT32:
      axis_string_append_formatted(str, "%u", self->content.uint32);
      break;
    case axis_TYPE_UINT64:
      axis_string_append_formatted(str, "%lu", self->content.uint64);
      break;
    case axis_TYPE_FLOAT32:
      axis_string_append_formatted(str, "%f", self->content.float32);
      break;
    case axis_TYPE_FLOAT64:
      axis_string_append_formatted(str, "%f", self->content.float64);
      break;
    case axis_TYPE_NULL:
      break;
    case axis_TYPE_PTR:
      axis_string_append_formatted(str, "0x%" PRIXPTR, self->content.ptr);
      break;
    case axis_TYPE_BUF:
      axis_string_append_formatted(str, "0x%" PRIXPTR, self->content.buf.data);
      break;
    case axis_TYPE_BOOL:
      axis_string_append_formatted(
          str, "%s",
          axis_value_get_bool(self, err) ? axis_STR_TRUE : axis_STR_FALSE);
      break;
    case axis_TYPE_STRING:
      axis_string_append_formatted(
          str, "%s", axis_string_get_raw_str(&self->content.string));
      break;
    case axis_TYPE_ARRAY:
      if (!axis_value_array_to_string(self, str, err)) {
        return false;
      }
      break;
    case axis_TYPE_OBJECT:
      if (!axis_value_object_to_string(self, str, err)) {
        return false;
      }
      break;
    default:
      axis_ASSERT(0, "Need to implement more.");
      break;
  }

  return true;
}

static bool axis_value_adjust_from_int_type(axis_TYPE type, axis_value_t *result,
                                           axis_error_t *err) {
  axis_ASSERT(result && err, "Invalid argument.");

  bool success = false;

  switch (type) {
    case axis_TYPE_INT8:
      success = axis_value_convert_to_int8(result, err);
      break;
    case axis_TYPE_INT16:
      success = axis_value_convert_to_int16(result, err);
      break;
    case axis_TYPE_INT32:
      success = axis_value_convert_to_int32(result, err);
      break;
    case axis_TYPE_INT64:
      break;
    case axis_TYPE_UINT8:
      success = axis_value_convert_to_uint8(result, err);
      break;
    case axis_TYPE_UINT16:
      success = axis_value_convert_to_uint16(result, err);
      break;
    case axis_TYPE_UINT32:
      success = axis_value_convert_to_uint32(result, err);
      break;
    case axis_TYPE_UINT64:
      success = axis_value_convert_to_uint64(result, err);
      break;
    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }

  return success;
}

axis_value_t *axis_value_from_type_and_string(axis_TYPE type, const char *str,
                                            axis_error_t *err) {
  axis_ASSERT(type, "Invalid argument.");
  axis_ASSERT(str, "Invalid argument.");

  bool success = true;
  axis_value_t *result = NULL;

  switch (type) {
    case axis_TYPE_INT8:
    case axis_TYPE_INT16:
    case axis_TYPE_INT32:
    case axis_TYPE_INT64: {
      errno = 0;
      int64_t int64_val = strtol(str, NULL, 10);
      if (errno == ERANGE) {
        if (err) {
          axis_error_set(err, axis_ERRNO_GENERIC, "Integer value out of range");
        }
        return NULL;
      }

      result = axis_value_create_int64(int64_val);
      success = axis_value_adjust_from_int_type(type, result, err);
      break;
    }

    case axis_TYPE_UINT8:
    case axis_TYPE_UINT16:
    case axis_TYPE_UINT32:
    case axis_TYPE_UINT64: {
      errno = 0;
      uint64_t uint64_val = strtoul(str, NULL, 10);
      if (errno == ERANGE) {
        if (err) {
          axis_error_set(err, axis_ERRNO_GENERIC, "Integer value out of range");
        }
        return NULL;
      }

      result = axis_value_create_uint64(uint64_val);
      success = axis_value_adjust_from_int_type(type, result, err);
      break;
    }

    case axis_TYPE_STRING:
      result = axis_value_create_string(str);
      break;

    case axis_TYPE_BOOL:
      result = axis_value_create_bool(
          axis_c_string_is_equal(str, axis_STR_TRUE) ? true : false);
      break;

    case axis_TYPE_NULL:
      result = axis_value_create_null();
      break;

    case axis_TYPE_FLOAT32:
    case axis_TYPE_FLOAT64: {
      errno = 0;
      double double_val = strtod(str, NULL);
      if (errno == ERANGE) {
        if (err) {
          axis_error_set(err, axis_ERRNO_GENERIC,
                        "Floating point value out of range");
        }
        return NULL;
      }

      switch (type) {
        case axis_TYPE_FLOAT32:
          result = axis_value_create_float32(double_val < -FLT_MAX ||
                                                    double_val > FLT_MAX
                                                ? 0.0F
                                                : (float)double_val);
          break;
        case axis_TYPE_FLOAT64:
          result = axis_value_create_float64(
              double_val < -DBL_MAX || double_val > DBL_MAX ? 0.0 : double_val);
          break;
        default:
          axis_ASSERT(0, "Should not happen.");
          break;
      }
      break;
    }

    default:
      axis_ASSERT(0, "Need to implement more operators.");
      break;
  }

  if (!success) {
    if (result) {
      axis_value_destroy(result);
    }
    result = NULL;
  }

  return result;
}
