//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_runtime/aptima_config.h"

#include "aptima_utils/lib/error.h"

// Define various error numbers here.
//
// Note: To achieve the best compatibility, any new enum item, should be added
// to the end to avoid changing the value of previous enum items.
typedef enum aptima_ERRNO {
  // Generic errno.
  aptima_ERRNO_GENERIC = 1,

  // Invalid json.
  aptima_ERRNO_INVALID_JSON = 2,

  // Invalid argument.
  aptima_ERRNO_INVALID_ARGUMENT = 3,

  // Invalid type.
  aptima_ERRNO_INVALID_TYPE = 4,

  // Invalid graph.
  aptima_ERRNO_INVALID_GRAPH = 5,

  // The TEN world is closed.
  aptima_ERRNO_aptima_IS_CLOSED = 6,

  // The msg is not connected in the graph.
  aptima_ERRNO_MSG_NOT_CONNECTED = 7,
} aptima_ERRNO;

static_assert(
    sizeof(aptima_ERRNO) <= sizeof(aptima_errno_t),
    "The size of field aptima_ERRNO enum should be less or equal to the size "
    "of aptima_errno_t.");
