//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_utils/ten_config.h"

typedef enum TEN_TYPE {
  TEN_TYPE_INVALID,

  TEN_TYPE_NULL,

  TEN_TYPE_BOOL,

  TEN_TYPE_INT8,
  TEN_TYPE_INT16,
  TEN_TYPE_INT32,
  TEN_TYPE_INT64,

  TEN_TYPE_UINT8,
  TEN_TYPE_UINT16,
  TEN_TYPE_UINT32,
  TEN_TYPE_UINT64,

  TEN_TYPE_FLOAT32,
  TEN_TYPE_FLOAT64,

  TEN_TYPE_STRING,
  TEN_TYPE_BUF,

  TEN_TYPE_ARRAY,
  TEN_TYPE_OBJECT,

  TEN_TYPE_PTR,
} TEN_TYPE;
