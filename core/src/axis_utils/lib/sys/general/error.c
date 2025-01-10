//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/error.h"

#include <stdarg.h>

#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/memory.h"

bool axis_error_check_integrity(axis_error_t *self) {
  axis_ASSERT(self, "Invalid argument");

  if (axis_signature_get(&self->signature) != axis_ERROR_SIGNATURE) {
    return false;
  }
  return true;
}

void axis_error_init(axis_error_t *self) {
  axis_ASSERT(self, "Invalid argument");

  axis_signature_set(&self->signature, axis_ERROR_SIGNATURE);

  self->err_no = axis_ERRNO_OK;
  axis_string_init(&self->err_msg);
}

void axis_error_deinit(axis_error_t *self) {
  axis_ASSERT(self && axis_error_check_integrity(self), "Invalid argument");

  axis_string_deinit(&self->err_msg);
}

axis_error_t *axis_error_create(void) {
  axis_error_t *self = (axis_error_t *)axis_MALLOC(sizeof(axis_error_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_error_init(self);

  return self;
}

void axis_error_copy(axis_error_t *self, axis_error_t *other) {
  axis_ASSERT(self && axis_error_check_integrity(self), "Invalid argument");
  axis_ASSERT(other && axis_error_check_integrity(other), "Invalid argument");

  self->err_no = other->err_no;
  axis_string_copy(&self->err_msg, &other->err_msg);
}

bool axis_error_prepend_errmsg(axis_error_t *self, const char *fmt, ...) {
  axis_ASSERT(self && axis_error_check_integrity(self), "Invalid argument");

  va_list ap;
  va_start(ap, fmt);
  axis_string_prepend_from_va_list(&self->err_msg, fmt, ap);
  va_end(ap);

  return true;
}

bool axis_error_append_errmsg(axis_error_t *self, const char *fmt, ...) {
  axis_ASSERT(self && axis_error_check_integrity(self), "Invalid argument");

  va_list ap;
  va_start(ap, fmt);
  axis_string_append_from_va_list(&self->err_msg, fmt, ap);
  va_end(ap);

  return true;
}

bool axis_error_set(axis_error_t *self, axis_errno_t err_no, const char *fmt,
                   ...) {
  axis_ASSERT(self && axis_error_check_integrity(self) && fmt,
             "Invalid argument");

  va_list ap;
  va_start(ap, fmt);
  bool result = axis_error_vset(self, err_no, fmt, ap);
  va_end(ap);

  return result;
}

bool axis_error_vset(axis_error_t *self, axis_errno_t err_no, const char *fmt,
                    va_list ap) {
  axis_ASSERT(self && axis_error_check_integrity(self) && fmt,
             "Invalid argument");

  self->err_no = err_no;
  axis_string_clear(&self->err_msg);
  axis_string_append_from_va_list(&self->err_msg, fmt, ap);

  return true;
}

axis_errno_t axis_error_errno(axis_error_t *self) {
  axis_ASSERT(self && axis_error_check_integrity(self), "Invalid argument");
  return self->err_no;
}

const char *axis_error_errmsg(axis_error_t *self) {
  axis_ASSERT(self && axis_error_check_integrity(self), "Invalid argument");

  return axis_string_get_raw_str(&self->err_msg);
}

void axis_error_reset(axis_error_t *self) {
  axis_ASSERT(self && axis_error_check_integrity(self), "Invalid argument");

  self->err_no = axis_ERRNO_OK;
  axis_string_clear(&self->err_msg);
}

void axis_error_destroy(axis_error_t *self) {
  axis_ASSERT(self && axis_error_check_integrity(self), "Invalid argument");

  axis_error_deinit(self);
  axis_FREE(self);
}

bool axis_error_is_success(axis_error_t *self) {
  axis_ASSERT(self && axis_error_check_integrity(self), "Invalid argument");

  return self->err_no == axis_ERRNO_OK;
}
