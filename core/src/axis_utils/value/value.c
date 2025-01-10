//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/value/value.h"

#include <assert.h>
#include <float.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>

#include "include_internal/axis_utils/value/value.h"
#include "axis_utils/container/list.h"
#include "axis_utils/container/list_node.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/buf.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"
#include "axis_utils/macro/memory.h"
#include "axis_utils/value/type.h"
#include "axis_utils/value/value_is.h"
#include "axis_utils/value/value_kv.h"

bool axis_value_check_integrity(axis_value_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_VALUE_SIGNATURE) {
    return false;
  }
  return true;
}

static bool axis_value_copy_int8(axis_value_t *dest, axis_value_t *src,
                                axis_UNUSED axis_error_t *err) {
  axis_ASSERT(dest && src, "Invalid argument.");
  axis_ASSERT(src->type == axis_TYPE_INT8, "Invalid argument.");

  dest->content.int8 = src->content.int8;

  return true;
}

static void axis_value_init(axis_value_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  // Initialize all memory within `value` to 0, so that the type within it (such
  // as `axis_string_t`) recognizes it as being in an uninitialized state.
  memset(self, 0, sizeof(axis_value_t));

  axis_signature_set(&self->signature, (axis_signature_t)axis_VALUE_SIGNATURE);
  self->type = axis_TYPE_INVALID;
}

bool axis_value_init_int8(axis_value_t *self, int8_t value) {
  axis_ASSERT(self, "Invalid argument.");

  axis_value_init(self);

  self->type = axis_TYPE_INT8;
  self->content.int8 = value;

  self->construct = NULL;
  self->copy = axis_value_copy_int8;
  self->destruct = NULL;

  return true;
}

static bool axis_value_copy_int16(axis_value_t *dest, axis_value_t *src,
                                 axis_UNUSED axis_error_t *err) {
  axis_ASSERT(dest && src, "Invalid argument.");
  axis_ASSERT(src->type == axis_TYPE_INT16, "Invalid argument.");

  dest->content.int16 = src->content.int16;

  return true;
}

bool axis_value_init_int16(axis_value_t *self, int16_t value) {
  axis_ASSERT(self, "Invalid argument.");

  axis_value_init(self);

  self->type = axis_TYPE_INT16;
  self->content.int16 = value;

  self->construct = NULL;
  self->copy = axis_value_copy_int16;
  self->destruct = NULL;

  return true;
}

static bool axis_value_copy_int32(axis_value_t *dest, axis_value_t *src,
                                 axis_UNUSED axis_error_t *err) {
  axis_ASSERT(dest && src, "Invalid argument.");
  axis_ASSERT(src->type == axis_TYPE_INT32, "Invalid argument.");

  dest->content.int32 = src->content.int32;

  return true;
}

bool axis_value_init_int32(axis_value_t *self, int32_t value) {
  axis_ASSERT(self, "Invalid argument.");

  axis_value_init(self);

  self->type = axis_TYPE_INT32;
  self->content.int32 = value;

  self->construct = NULL;
  self->copy = axis_value_copy_int32;
  self->destruct = NULL;

  return true;
}

static bool axis_value_copy_int64(axis_value_t *dest, axis_value_t *src,
                                 axis_UNUSED axis_error_t *err) {
  axis_ASSERT(dest && src, "Invalid argument.");
  axis_ASSERT(src->type == axis_TYPE_INT64, "Invalid argument.");

  dest->content.int64 = src->content.int64;

  return true;
}

bool axis_value_init_int64(axis_value_t *self, int64_t value) {
  axis_ASSERT(self, "Invalid argument.");

  axis_value_init(self);

  self->type = axis_TYPE_INT64;
  self->content.int64 = value;

  self->construct = NULL;
  self->copy = axis_value_copy_int64;
  self->destruct = NULL;

  return true;
}

static bool axis_value_copy_uint8(axis_value_t *dest, axis_value_t *src,
                                 axis_UNUSED axis_error_t *err) {
  axis_ASSERT(dest && src, "Invalid argument.");
  axis_ASSERT(src->type == axis_TYPE_UINT8, "Invalid argument.");

  dest->content.uint8 = src->content.uint8;

  return true;
}

bool axis_value_init_uint8(axis_value_t *self, uint8_t value) {
  axis_ASSERT(self, "Invalid argument.");

  axis_value_init(self);

  self->type = axis_TYPE_UINT8;
  self->content.uint8 = value;

  self->construct = NULL;
  self->copy = axis_value_copy_uint8;
  self->destruct = NULL;

  return true;
}

static bool axis_value_copy_uint16(axis_value_t *dest, axis_value_t *src,
                                  axis_UNUSED axis_error_t *err) {
  axis_ASSERT(dest && src, "Invalid argument.");
  axis_ASSERT(src->type == axis_TYPE_UINT16, "Invalid argument.");

  dest->content.uint16 = src->content.uint16;

  return true;
}

bool axis_value_init_uint16(axis_value_t *self, uint16_t value) {
  axis_ASSERT(self, "Invalid argument.");

  axis_value_init(self);

  self->type = axis_TYPE_UINT16;
  self->content.uint16 = value;

  self->construct = NULL;
  self->copy = axis_value_copy_uint16;
  self->destruct = NULL;

  return true;
}

static bool axis_value_copy_uint32(axis_value_t *dest, axis_value_t *src,
                                  axis_UNUSED axis_error_t *err) {
  axis_ASSERT(dest && src, "Invalid argument.");
  axis_ASSERT(src->type == axis_TYPE_UINT32, "Invalid argument.");

  dest->content.uint32 = src->content.uint32;

  return true;
}

bool axis_value_init_uint32(axis_value_t *self, uint32_t value) {
  axis_ASSERT(self, "Invalid argument.");

  axis_value_init(self);

  self->type = axis_TYPE_UINT32;
  self->content.uint32 = value;

  self->construct = NULL;
  self->copy = axis_value_copy_uint32;
  self->destruct = NULL;

  return true;
}

static bool axis_value_copy_uint64(axis_value_t *dest, axis_value_t *src,
                                  axis_UNUSED axis_error_t *err) {
  axis_ASSERT(dest && src, "Invalid argument.");
  axis_ASSERT(src->type == axis_TYPE_UINT64, "Invalid argument.");

  dest->content.uint64 = src->content.uint64;

  return true;
}

bool axis_value_init_uint64(axis_value_t *self, uint64_t value) {
  axis_ASSERT(self, "Invalid argument.");

  axis_value_init(self);

  self->type = axis_TYPE_UINT64;
  self->content.uint64 = value;

  self->construct = NULL;
  self->copy = axis_value_copy_uint64;
  self->destruct = NULL;

  return true;
}

static bool axis_value_copy_float32(axis_value_t *dest, axis_value_t *src,
                                   axis_UNUSED axis_error_t *err) {
  axis_ASSERT(dest && src, "Invalid argument.");
  axis_ASSERT(src->type == axis_TYPE_FLOAT32, "Invalid argument.");

  dest->content.float32 = src->content.float32;

  return true;
}

bool axis_value_init_float32(axis_value_t *self, float value) {
  axis_ASSERT(self, "Invalid argument.");

  axis_value_init(self);

  self->type = axis_TYPE_FLOAT32;
  self->content.float32 = value;

  self->construct = NULL;
  self->copy = axis_value_copy_float32;
  self->destruct = NULL;

  return true;
}

static bool axis_value_copy_float64(axis_value_t *dest, axis_value_t *src,
                                   axis_UNUSED axis_error_t *err) {
  axis_ASSERT(dest && src, "Invalid argument.");
  axis_ASSERT(src->type == axis_TYPE_FLOAT64, "Invalid argument.");

  dest->content.float64 = src->content.float64;

  return true;
}

bool axis_value_init_float64(axis_value_t *self, double value) {
  axis_ASSERT(self, "Invalid argument.");

  axis_value_init(self);

  self->type = axis_TYPE_FLOAT64;
  self->content.float64 = value;

  self->construct = NULL;
  self->copy = axis_value_copy_float64;
  self->destruct = NULL;

  return true;
}

bool axis_value_init_null(axis_value_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_value_init(self);

  self->type = axis_TYPE_NULL;

  return true;
}

static bool axis_value_copy_bool(axis_value_t *dest, axis_value_t *src,
                                axis_UNUSED axis_error_t *err) {
  axis_ASSERT(dest && src, "Invalid argument.");
  axis_ASSERT(src->type == axis_TYPE_BOOL, "Invalid argument.");

  dest->content.boolean = src->content.boolean;

  return true;
}

bool axis_value_init_bool(axis_value_t *self, bool value) {
  axis_ASSERT(self, "Invalid argument.");

  axis_value_init(self);

  self->type = axis_TYPE_BOOL;
  self->content.boolean = value;

  self->construct = NULL;
  self->copy = axis_value_copy_bool;
  self->destruct = NULL;

  return true;
}

static bool axis_value_copy_construct_string(axis_value_t *dest, axis_value_t *src,
                                            axis_UNUSED axis_error_t *err) {
  axis_ASSERT(dest && src, "Invalid argument.");
  axis_ASSERT(axis_value_is_string(src), "Invalid argument.");

  axis_string_init_from_string(&dest->content.string, &src->content.string);

  return true;
}

static bool axis_value_destruct_string(axis_value_t *self,
                                      axis_UNUSED axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_value_is_string(self), "Invalid argument.");

  axis_string_deinit(&self->content.string);

  return true;
}

bool axis_value_init_string(axis_value_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_value_init(self);

  self->type = axis_TYPE_STRING;

  axis_string_init(&self->content.string);
  self->construct = NULL;
  self->copy = axis_value_copy_construct_string;
  self->destruct = axis_value_destruct_string;

  return true;
}

static bool axis_value_init_vastring(axis_value_t *self, const char *fmt,
                                    va_list ap) {
  axis_ASSERT(fmt, "Invalid argument.");

  axis_value_init_string(self);

  axis_string_append_from_va_list(&self->content.string, fmt, ap);

  return true;
}

bool axis_value_init_string_with_size(axis_value_t *self, const char *str,
                                     size_t len) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");
  axis_ASSERT(str, "Invalid argument.");

  axis_value_init_string(self);
  axis_string_set_formatted(&self->content.string, "%.*s", len, str);

  return true;
}

static bool axis_value_copy_array(axis_value_t *dest, axis_value_t *src,
                                 axis_UNUSED axis_error_t *err) {
  axis_ASSERT(dest && src, "Invalid argument.");
  axis_ASSERT(src->type == axis_TYPE_ARRAY, "Invalid argument.");

  axis_list_init(&dest->content.array);
  axis_list_foreach (&src->content.array, iter) {
    axis_value_t *item = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(item && axis_value_check_integrity(item), "Invalid argument.");

    axis_value_t *clone_item = axis_value_clone(item);
    axis_list_push_ptr_back(&dest->content.array, clone_item,
                           (axis_ptr_listnode_destroy_func_t)axis_value_destroy);
  }

  return true;
}

static bool axis_value_destruct_array(axis_value_t *self,
                                     axis_UNUSED axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(self->type == axis_TYPE_ARRAY, "Invalid argument.");

  axis_list_clear(&self->content.array);

  return true;
}

bool axis_value_init_array_with_move(axis_value_t *self, axis_list_t *value) {
  axis_ASSERT(self, "Invalid argument.");

  axis_value_init(self);

  self->type = axis_TYPE_ARRAY;
  axis_list_init(&self->content.array);
  if (value) {
    axis_list_swap(&self->content.array, value);
  }

  self->construct = NULL;
  self->copy = axis_value_copy_array;
  self->destruct = axis_value_destruct_array;

  return true;
}

static bool axis_value_copy_ptr_default(axis_value_t *dest, axis_value_t *src,
                                       axis_UNUSED axis_error_t *err) {
  axis_ASSERT(dest && src, "Invalid argument.");
  axis_ASSERT(src->type == axis_TYPE_PTR, "Invalid argument.");

  dest->content.ptr = src->content.ptr;

  return true;
}

static void axis_value_init_ptr(axis_value_t *self, void *ptr,
                               axis_value_construct_func_t construct,
                               axis_value_copy_func_t copy,
                               axis_value_destruct_func_t destruct) {
  axis_ASSERT(self, "Invalid argument.");

  axis_value_init(self);

  self->type = axis_TYPE_PTR;
  self->content.ptr = ptr;

  self->construct = construct;
  // If there is a customized copy handler, use it, otherwise, simply copy the
  // pointer directly.
  self->copy = copy ? copy : axis_value_copy_ptr_default;
  self->destruct = destruct;

  // If there is a customized construct handler, use it, otherwise, just set the
  // pointer value.
  if (self->construct) {
    self->construct(self, NULL);
  }
}

static bool axis_value_copy_buf(axis_value_t *dest, axis_value_t *src,
                               axis_UNUSED axis_error_t *err) {
  axis_ASSERT(dest && src, "Invalid argument.");
  axis_ASSERT(src->type == axis_TYPE_BUF, "Invalid argument.");

  axis_buf_t *src_buf = &src->content.buf;
  if (src_buf->owns_memory) {
    if (src_buf->size) {
      axis_buf_init_with_copying_data(&dest->content.buf, src_buf->data,
                                     src_buf->size);
    }
  } else {
    dest->content.buf = *src_buf;
  }

  return true;
}

static bool axis_value_destruct_buf(axis_value_t *self,
                                   axis_UNUSED axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(self->type == axis_TYPE_BUF, "Invalid argument.");

  axis_buf_t *buf = &self->content.buf;
  if (buf->owns_memory) {
    axis_buf_deinit(&self->content.buf);
  }

  return true;
}

bool axis_value_init_buf(axis_value_t *self, size_t size) {
  axis_ASSERT(self, "Invalid argument.");

  axis_value_init(self);

  self->type = axis_TYPE_BUF;
  axis_buf_init_with_owned_data(&self->content.buf, size);

  self->construct = NULL;
  self->copy = axis_value_copy_buf;
  self->destruct = axis_value_destruct_buf;

  return true;
}

static void axis_value_init_buf_with_move(axis_value_t *self, axis_buf_t buf) {
  axis_ASSERT(self, "Invalid argument.");

  axis_value_init(self);

  self->type = axis_TYPE_BUF;
  self->content.buf = buf;

  self->construct = NULL;
  self->copy = axis_value_copy_buf;
  self->destruct = axis_value_destruct_buf;
}

static bool axis_value_copy_object(axis_value_t *dest, axis_value_t *src,
                                  axis_UNUSED axis_error_t *err) {
  axis_ASSERT(dest && src, "Invalid argument.");
  axis_ASSERT(src->type == axis_TYPE_OBJECT, "Invalid argument.");

  axis_list_init(&dest->content.object);

  axis_list_foreach (&src->content.object, iter) {
    axis_value_kv_t *item = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(item && axis_value_kv_check_integrity(item), "Invalid argument.");

    axis_value_kv_t *clone_item = axis_value_kv_clone(item);
    axis_list_push_ptr_back(
        &dest->content.object, clone_item,
        (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);
  }

  return true;
}

static bool axis_value_destruct_object(axis_value_t *self,
                                      axis_UNUSED axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(self->type == axis_TYPE_OBJECT, "Invalid argument.");

  axis_list_clear(&self->content.object);

  return true;
}

bool axis_value_init_object_with_move(axis_value_t *self, axis_list_t *value) {
  axis_ASSERT(self, "Invalid argument.");

  axis_value_init(self);

  self->type = axis_TYPE_OBJECT;
  axis_list_init(&self->content.object);
  if (value) {
    axis_list_swap(&self->content.object, value);
  }

  self->construct = NULL;
  self->copy = axis_value_copy_object;
  self->destruct = axis_value_destruct_object;

  return true;
}

void axis_value_reset_to_string_with_size(axis_value_t *self, const char *str,
                                         size_t len) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");

  axis_value_deinit(self);
  axis_value_init_string_with_size(self, str, len);
}

void axis_value_reset_to_null(axis_value_t *self) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");

  axis_value_deinit(self);
  axis_value_init_null(self);
}

void axis_value_reset_to_ptr(axis_value_t *self, void *ptr,
                            axis_value_construct_func_t construct,
                            axis_value_copy_func_t copy,
                            axis_value_destruct_func_t destruct) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");

  axis_value_deinit(self);
  axis_value_init_ptr(self, ptr, construct, copy, destruct);
}

static axis_value_t *axis_value_create(void) {
  axis_value_t *self = (axis_value_t *)axis_MALLOC(sizeof(axis_value_t));
  axis_ASSERT(self, "Invalid argument.");

  axis_value_init(self);

  return self;
}

axis_value_t *axis_value_create_invalid(void) {
  axis_value_t *v = axis_value_create();
  axis_value_init_invalid(v);
  return v;
}

bool axis_value_init_invalid(axis_value_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_value_init(self);
  self->type = axis_TYPE_INVALID;

  return true;
}

axis_value_t *axis_value_create_int8(int8_t value) {
  axis_value_t *self = axis_value_create();
  axis_value_init_int8(self, value);
  return self;
}

axis_value_t *axis_value_create_int16(int16_t value) {
  axis_value_t *self = axis_value_create();
  axis_value_init_int16(self, value);
  return self;
}

axis_value_t *axis_value_create_int32(int32_t value) {
  axis_value_t *self = axis_value_create();
  axis_value_init_int32(self, value);
  return self;
}

axis_value_t *axis_value_create_int64(int64_t value) {
  axis_value_t *self = axis_value_create();
  bool rc = axis_value_init_int64(self, value);
  axis_ASSERT(rc, "Failed to initialize the value.");
  return self;
}

axis_value_t *axis_value_create_uint8(uint8_t value) {
  axis_value_t *self = axis_value_create();
  axis_value_init_uint8(self, value);
  return self;
}

axis_value_t *axis_value_create_uint16(uint16_t value) {
  axis_value_t *self = axis_value_create();
  axis_value_init_uint16(self, value);
  return self;
}

axis_value_t *axis_value_create_uint32(uint32_t value) {
  axis_value_t *self = axis_value_create();
  axis_value_init_uint32(self, value);
  return self;
}

axis_value_t *axis_value_create_uint64(uint64_t value) {
  axis_value_t *self = axis_value_create();
  axis_value_init_uint64(self, value);
  return self;
}

axis_value_t *axis_value_create_float32(float value) {
  axis_value_t *self = axis_value_create();
  axis_value_init_float32(self, value);
  return self;
}

axis_value_t *axis_value_create_float64(double value) {
  axis_value_t *self = axis_value_create();
  bool rc = axis_value_init_float64(self, value);
  axis_ASSERT(rc, "Failed to initialize the value.");
  return self;
}

axis_value_t *axis_value_create_bool(bool value) {
  axis_value_t *self = axis_value_create();
  axis_value_init_bool(self, value);
  return self;
}

axis_value_t *axis_value_create_null(void) {
  axis_value_t *self = axis_value_create();
  bool rc = axis_value_init_null(self);
  axis_ASSERT(rc, "Failed to initialize the value.");
  return self;
}

axis_value_t *axis_value_create_array_with_move(axis_list_t *value) {
  axis_value_t *self = axis_value_create();
  axis_value_init_array_with_move(self, value);
  return self;
}

axis_value_t *axis_value_create_object_with_move(axis_list_t *value) {
  axis_value_t *self = axis_value_create();
  axis_value_init_object_with_move(self, value);
  return self;
}

axis_value_t *axis_value_create_string_with_size(const char *str, size_t len) {
  axis_ASSERT(str, "Invalid argument.");

  axis_value_t *self = axis_value_create();
  bool rc = axis_value_init_string_with_size(self, str, len);
  axis_ASSERT(rc, "Failed to initialize the value.");
  return self;
}

axis_value_t *axis_value_create_string(const char *str) {
  axis_ASSERT(str, "Invalid argument.");
  return axis_value_create_string_with_size(str, strlen(str));
}

axis_value_t *axis_value_create_vastring(const char *fmt, va_list ap) {
  axis_ASSERT(fmt, "Invalid argument.");

  axis_value_t *self = axis_value_create();
  bool rc = axis_value_init_vastring(self, fmt, ap);
  axis_ASSERT(rc, "Failed to initialize the value.");
  return self;
}

axis_value_t *axis_value_create_vstring(const char *fmt, ...) {
  axis_ASSERT(fmt, "Invalid argument.");

  va_list ap;
  va_start(ap, fmt);
  axis_value_t *self = axis_value_create_vastring(fmt, ap);
  va_end(ap);

  return self;
}

void axis_value_set_name(axis_value_t *self, const char *fmt, ...) {
  axis_ASSERT(self && fmt, "Invalid argument.");

  if (self->name) {
    axis_string_destroy(self->name);
  }

  va_list ap;
  va_start(ap, fmt);
  self->name = axis_string_create_from_va_list(fmt, ap);
  va_end(ap);
}

axis_value_t *axis_value_create_ptr(void *ptr,
                                  axis_value_construct_func_t construct,
                                  axis_value_copy_func_t copy,
                                  axis_value_destruct_func_t destruct) {
  axis_value_t *self = axis_value_create();
  axis_value_init_ptr(self, ptr, construct, copy, destruct);
  return self;
}

axis_value_t *axis_value_create_buf_with_move(axis_buf_t buf) {
  axis_value_t *self = axis_value_create();
  axis_value_init_buf_with_move(self, buf);
  return self;
}

bool axis_value_copy(axis_value_t *src, axis_value_t *dest) {
  axis_ASSERT(src && axis_value_check_integrity(src), "Invalid argument.");
  axis_ASSERT(dest && axis_value_check_integrity(dest), "Invalid argument.");

  dest->type = src->type;

  axis_value_construct_func_t construct = src->construct;
  axis_value_destruct_func_t destruct = src->destruct;
  axis_value_copy_func_t copy = src->copy;

  if (src->copy) {
    src->copy(dest, src, NULL);
  }

  dest->construct = construct;
  dest->destruct = destruct;
  dest->copy = copy;

  return true;
}

axis_value_t *axis_value_clone(axis_value_t *src) {
  axis_ASSERT(src && axis_value_check_integrity(src), "Invalid argument.");

  axis_value_t *self = axis_value_create();

  bool rc = axis_value_copy(src, self);
  axis_ASSERT(rc, "Should not happen.");

  return self;
}

void axis_value_deinit(axis_value_t *self) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");

  if (self->name) {
    axis_string_destroy(self->name);
    self->name = NULL;
  }

  if (self->destruct) {
    self->destruct(self, NULL);
  }
}

void axis_value_destroy(axis_value_t *self) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");

  axis_value_deinit(self);
  axis_FREE(self);
}

size_t axis_value_array_size(axis_value_t *self) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");
  return axis_list_size(&self->content.array);
}

bool axis_value_is_valid(axis_value_t *self) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");

  switch (self->type) {
    case axis_TYPE_INVALID:
      return false;
    default:
      return true;
  }
}

bool axis_value_is_equal(axis_value_t *self, axis_value_t *target) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");
  axis_ASSERT(target && axis_value_check_integrity(target), "Invalid argument.");

  if (self->type != target->type) {
    return false;
  }
  switch (self->type) {
    case axis_TYPE_NULL:
      break;
    case axis_TYPE_BOOL:
      return self->content.boolean == target->content.boolean;
    case axis_TYPE_STRING:
      return axis_string_is_equal(&self->content.string,
                                 &target->content.string);
    case axis_TYPE_OBJECT:
      if (axis_list_size(&self->content.object) !=
          axis_list_size(&target->content.object)) {
        return false;
      }

      axis_list_foreach (&self->content.object, iter_self) {
        axis_value_kv_t *kv_self = axis_ptr_listnode_get(iter_self.node);
        axis_ASSERT(kv_self && axis_value_kv_check_integrity(kv_self),
                   "Invalid argument.");

        // Peek the corresponding value in the target object.
        axis_value_t *kv_target = axis_value_object_peek(
            target, axis_string_get_raw_str(&kv_self->key));
        if (!kv_target) {
          // Key does not exist in target
          return false;
        }

        // Recursively check equality of the values.
        if (!axis_value_is_equal(kv_self->value, kv_target)) {
          return false;
        }
      }
      break;
    case axis_TYPE_ARRAY:
      if (axis_list_size(&self->content.array) !=
          axis_list_size(&target->content.array)) {
        return false;
      }

      {
        axis_list_iterator_t iter_self = axis_list_begin(&self->content.array);
        axis_list_iterator_t iter_target =
            axis_list_begin(&target->content.array);

        while (!axis_list_iterator_is_end(iter_self) &&
               !axis_list_iterator_is_end(iter_target)) {
          axis_value_t *value_self =
              (axis_value_t *)axis_list_iterator_to_listnode(iter_self);
          axis_value_t *value_target =
              (axis_value_t *)axis_list_iterator_to_listnode(iter_target);

          if (!axis_value_is_equal(value_self, value_target)) {
            return false;
          }

          iter_self = axis_list_iterator_next(iter_self);
          iter_target = axis_list_iterator_next(iter_target);
        }
      }
      break;
    case axis_TYPE_INT8:
      return self->content.int8 == target->content.int8;
    case axis_TYPE_INT16:
      return self->content.int16 == target->content.int16;
    case axis_TYPE_INT32:
      return self->content.int32 == target->content.int32;
    case axis_TYPE_INT64:
      return self->content.int64 == target->content.int64;
    case axis_TYPE_UINT8:
      return self->content.uint8 == target->content.uint8;
    case axis_TYPE_UINT16:
      return self->content.uint16 == target->content.uint16;
    case axis_TYPE_UINT32:
      return self->content.uint32 == target->content.uint32;
    case axis_TYPE_UINT64:
      return self->content.uint64 == target->content.uint64;
    case axis_TYPE_FLOAT32:
      return self->content.float32 == target->content.float32;
    case axis_TYPE_FLOAT64:
      return self->content.float64 == target->content.float64;
    default:
      axis_ASSERT(0, "Invalid argument.");
      return false;
  }

  return true;
}
