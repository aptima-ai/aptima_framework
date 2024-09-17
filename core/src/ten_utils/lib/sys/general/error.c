//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "ten_utils/lib/error.h"

#include <stdarg.h>

#include "ten_utils/macro/check.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/macro/memory.h"

bool ten_error_check_integrity(ten_error_t *self) {
  TEN_ASSERT(self, "Invalid argument");

  if (ten_signature_get(&self->signature) != TEN_ERROR_SIGNATURE) {
    return false;
  }
  return true;
}

void ten_error_init(ten_error_t *self) {
  TEN_ASSERT(self, "Invalid argument");

  ten_signature_set(&self->signature, TEN_ERROR_SIGNATURE);

  self->err_no = TEN_ERRNO_OK;
  ten_string_init(&self->err_msg);
}

void ten_error_deinit(ten_error_t *self) {
  TEN_ASSERT(self && ten_error_check_integrity(self), "Invalid argument");

  ten_string_deinit(&self->err_msg);
}

ten_error_t *ten_error_create(void) {
  ten_error_t *self = (ten_error_t *)TEN_MALLOC(sizeof(ten_error_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  ten_error_init(self);

  return self;
}

bool ten_error_prepend_errmsg(ten_error_t *self, const char *fmt, ...) {
  TEN_ASSERT(self && ten_error_check_integrity(self), "Invalid argument");

  va_list ap;
  va_start(ap, fmt);
  ten_string_prepend_from_va_list(&self->err_msg, fmt, ap);
  va_end(ap);

  return true;
}

bool ten_error_append_errmsg(ten_error_t *self, const char *fmt, ...) {
  TEN_ASSERT(self && ten_error_check_integrity(self), "Invalid argument");

  va_list ap;
  va_start(ap, fmt);
  ten_string_append_from_va_list(&self->err_msg, fmt, ap);
  va_end(ap);

  return true;
}

bool ten_error_set(ten_error_t *self, ten_errno_t err_no, const char *fmt,
                   ...) {
  TEN_ASSERT(self && ten_error_check_integrity(self) && fmt,
             "Invalid argument");

  va_list ap;
  va_start(ap, fmt);
  bool result = ten_error_vset(self, err_no, fmt, ap);
  va_end(ap);

  return result;
}

bool ten_error_vset(ten_error_t *self, ten_errno_t err_no, const char *fmt,
                    va_list ap) {
  TEN_ASSERT(self && ten_error_check_integrity(self) && fmt,
             "Invalid argument");

  self->err_no = err_no;
  ten_string_clear(&self->err_msg);
  ten_string_append_from_va_list(&self->err_msg, fmt, ap);

  return true;
}

ten_errno_t ten_error_errno(ten_error_t *self) {
  TEN_ASSERT(self && ten_error_check_integrity(self), "Invalid argument");
  return self->err_no;
}

const char *ten_error_errmsg(ten_error_t *self) {
  TEN_ASSERT(self && ten_error_check_integrity(self), "Invalid argument");

  return ten_string_get_raw_str(&self->err_msg);
}

void ten_error_reset(ten_error_t *self) {
  TEN_ASSERT(self && ten_error_check_integrity(self), "Invalid argument");

  self->err_no = TEN_ERRNO_OK;
  ten_string_clear(&self->err_msg);
}

void ten_error_destroy(ten_error_t *self) {
  TEN_ASSERT(self && ten_error_check_integrity(self), "Invalid argument");

  ten_error_deinit(self);
  TEN_FREE(self);
}

bool ten_error_is_success(ten_error_t *self) {
  TEN_ASSERT(self && ten_error_check_integrity(self), "Invalid argument");

  return self->err_no == TEN_ERRNO_OK;
}
