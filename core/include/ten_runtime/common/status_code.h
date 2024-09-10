//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

typedef enum TEN_STATUS_CODE {
  TEN_STATUS_CODE_INVALID = -1,

  // 0 representing OK is a common convention.
  TEN_STATUS_CODE_OK = 0,
  TEN_STATUS_CODE_ERROR = 1,

  TEN_STATUS_CODE_LAST = 2,
} TEN_STATUS_CODE;
