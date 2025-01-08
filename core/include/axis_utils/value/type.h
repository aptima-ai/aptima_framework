//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

typedef enum axis_TYPE {
  axis_TYPE_INVALID,

  axis_TYPE_NULL,

  axis_TYPE_BOOL,

  axis_TYPE_INT8,
  axis_TYPE_INT16,
  axis_TYPE_INT32,
  axis_TYPE_INT64,

  axis_TYPE_UINT8,
  axis_TYPE_UINT16,
  axis_TYPE_UINT32,
  axis_TYPE_UINT64,

  axis_TYPE_FLOAT32,
  axis_TYPE_FLOAT64,

  axis_TYPE_STRING,
  axis_TYPE_BUF,

  axis_TYPE_ARRAY,
  axis_TYPE_OBJECT,

  axis_TYPE_PTR,
} axis_TYPE;
