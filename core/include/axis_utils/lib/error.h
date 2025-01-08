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

#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/string.h"

typedef int64_t axis_errno_t;

#define axis_ERROR_SIGNATURE 0xCA49E5F63FC43623U

// 0 is a special TEN errno value, representing success/ok.
#define axis_ERRNO_OK 0

typedef struct axis_error_t {
  axis_signature_t signature;

  axis_errno_t err_no;
  axis_string_t err_msg;
} axis_error_t;

axis_UTILS_API bool axis_error_check_integrity(axis_error_t *self);

axis_UTILS_API void axis_error_init(axis_error_t *self);

axis_UTILS_API void axis_error_deinit(axis_error_t *self);

axis_UTILS_API axis_error_t *axis_error_create(void);

axis_UTILS_API void axis_error_copy(axis_error_t *self, axis_error_t *other);

// Set error info, return true if set success, false otherwise.
axis_UTILS_API bool axis_error_set(axis_error_t *self, axis_errno_t err_no,
                                 const char *fmt, ...);

axis_UTILS_API bool axis_error_vset(axis_error_t *self, axis_errno_t err_no,
                                  const char *fmt, va_list ap);

axis_UTILS_API bool axis_error_prepend_errmsg(axis_error_t *self, const char *fmt,
                                            ...);

axis_UTILS_API bool axis_error_append_errmsg(axis_error_t *self, const char *fmt,
                                           ...);

// Get last errno in current context, return axis_ERRNO_OK if no error set
// before.
axis_UTILS_API axis_errno_t axis_error_errno(axis_error_t *self);

axis_UTILS_API const char *axis_error_errmsg(axis_error_t *self);

axis_UTILS_API void axis_error_reset(axis_error_t *self);

axis_UTILS_API void axis_error_destroy(axis_error_t *self);

axis_UTILS_API bool axis_error_is_success(axis_error_t *self);
