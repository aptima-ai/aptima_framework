//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_utils/lib/error.h"

// Define various error numbers here.
//
// Note: To achieve the best compatibility, any new enum item, should be added
// to the end to avoid changing the value of previous enum items.
typedef enum axis_ERRNO {
  // Generic errno.
  axis_ERRNO_GENERIC = 1,

  // Invalid json.
  axis_ERRNO_INVALID_JSON = 2,

  // Invalid argument.
  axis_ERRNO_INVALID_ARGUMENT = 3,

  // Invalid type.
  axis_ERRNO_INVALID_TYPE = 4,

  // Invalid graph.
  axis_ERRNO_INVALID_GRAPH = 5,

  // The TEN world is closed.
  axis_ERRNO_axis_IS_CLOSED = 6,

  // The msg is not connected in the graph.
  axis_ERRNO_MSG_NOT_CONNECTED = 7,
} axis_ERRNO;

static_assert(
    sizeof(axis_ERRNO) <= sizeof(axis_errno_t),
    "The size of field axis_ERRNO enum should be less or equal to the size "
    "of axis_errno_t.");
