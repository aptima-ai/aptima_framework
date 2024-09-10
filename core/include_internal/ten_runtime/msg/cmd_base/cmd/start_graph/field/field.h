//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

typedef enum TEN_CMD_CONNECT_FIELD {
  TEN_CMD_CONNECT_FIELD_CMDHDR,

  TEN_CMD_CONNECT_FIELD_LONG_RUNNING_MODE,
  TEN_CMD_CONNECT_FIELD_EXTENSION_INFO,

  TEN_CMD_CONNECT_FIELD_PREDEFINED_GRAPH,

  TEN_CMD_CONNECT_FIELD_LAST,
} TEN_CMD_CONNECT_FIELD;
