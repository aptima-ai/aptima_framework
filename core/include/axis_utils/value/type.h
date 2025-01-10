//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

typedef enum aptima_TYPE {
  aptima_TYPE_INVALID,

  aptima_TYPE_NULL,

  aptima_TYPE_BOOL,

  aptima_TYPE_INT8,
  aptima_TYPE_INT16,
  aptima_TYPE_INT32,
  aptima_TYPE_INT64,

  aptima_TYPE_UINT8,
  aptima_TYPE_UINT16,
  aptima_TYPE_UINT32,
  aptima_TYPE_UINT64,

  aptima_TYPE_FLOAT32,
  aptima_TYPE_FLOAT64,

  aptima_TYPE_STRING,
  aptima_TYPE_BUF,

  aptima_TYPE_ARRAY,
  aptima_TYPE_OBJECT,

  aptima_TYPE_PTR,
} aptima_TYPE;
