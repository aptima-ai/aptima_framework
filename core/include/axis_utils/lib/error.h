//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include <stdbool.h>
#include <stdint.h>

#include "aptima_utils/lib/signature.h"
#include "aptima_utils/lib/string.h"

typedef int64_t aptima_errno_t;

#define aptima_ERROR_SIGNATURE 0xCA49E5F63FC43623U

// 0 is a special TEN errno value, representing success/ok.
#define aptima_ERRNO_OK 0

typedef struct aptima_error_t {
  aptima_signature_t signature;

  aptima_errno_t err_no;
  aptima_string_t err_msg;
} aptima_error_t;

aptima_UTILS_API bool aptima_error_check_integrity(aptima_error_t *self);

aptima_UTILS_API void aptima_error_init(aptima_error_t *self);

aptima_UTILS_API void aptima_error_deinit(aptima_error_t *self);

aptima_UTILS_API aptima_error_t *aptima_error_create(void);

aptima_UTILS_API void aptima_error_copy(aptima_error_t *self, aptima_error_t *other);

// Set error info, return true if set success, false otherwise.
aptima_UTILS_API bool aptima_error_set(aptima_error_t *self, aptima_errno_t err_no,
                                 const char *fmt, ...);

aptima_UTILS_API bool aptima_error_vset(aptima_error_t *self, aptima_errno_t err_no,
                                  const char *fmt, va_list ap);

aptima_UTILS_API bool aptima_error_prepend_errmsg(aptima_error_t *self, const char *fmt,
                                            ...);

aptima_UTILS_API bool aptima_error_append_errmsg(aptima_error_t *self, const char *fmt,
                                           ...);

// Get last errno in current context, return aptima_ERRNO_OK if no error set
// before.
aptima_UTILS_API aptima_errno_t aptima_error_errno(aptima_error_t *self);

aptima_UTILS_API const char *aptima_error_errmsg(aptima_error_t *self);

aptima_UTILS_API void aptima_error_reset(aptima_error_t *self);

aptima_UTILS_API void aptima_error_destroy(aptima_error_t *self);

aptima_UTILS_API bool aptima_error_is_success(aptima_error_t *self);
