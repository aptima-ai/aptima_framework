//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "ten_utils/lib/error.h"

// Define various error numbers here.
//
// Note: To achieve the best compatibility, any new enum item, should be added
// to the end to avoid changing the value of previous enum items.
typedef enum TEN_ERRNO {
  // Generic errno.
  TEN_ERRNO_GENERIC = 1,

  // Invalid json.
  TEN_ERRNO_INVALID_JSON = 2,

  // Invalid argument.
  TEN_ERRNO_INVALID_ARGUMENT = 3,

  // Invalid type.
  TEN_ERRNO_INVALID_TYPE = 4,

  // Invalid graph.
  TEN_ERRNO_INVALID_GRAPH = 5,

  // The TEN world is closed.
  TEN_ERRNO_TEN_IS_CLOSED = 6,
} TEN_ERRNO;

static_assert(
    sizeof(TEN_ERRNO) <= sizeof(ten_errno_t),
    "The size of field TEN_ERRNO enum should be less or equal to the size "
    "of ten_errno_t.");
