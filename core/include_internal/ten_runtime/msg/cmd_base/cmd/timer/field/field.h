//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

typedef enum TEN_CMD_TIMER_FIELD {
  TEN_CMD_TIMER_FIELD_CMDHDR,

  TEN_CMD_TIMER_FIELD_TIMER_ID,
  TEN_CMD_TIMER_FIELD_TIMEOUT_IN_US,
  TEN_CMD_TIMER_FIELD_TIMES,

  TEN_CMD_TIMER_FIELD_LAST,
} TEN_CMD_TIMER_FIELD;
