//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

typedef enum TEN_CMD_STATUS_FIELD {
  TEN_CMD_STATUS_FIELD_CMD_BASE_HDR,

  TEN_CMD_STATUS_FIELD_ORIGINAL_CMD_TYPE,
  TEN_CMD_STATUS_FIELD_STATUS_CODE,

  TEN_CMD_STATUS_FIELD_LAST,
} TEN_CMD_STATUS_FIELD;
