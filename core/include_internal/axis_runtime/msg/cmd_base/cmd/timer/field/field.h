//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

typedef enum axis_CMD_TIMER_FIELD {
  axis_CMD_TIMER_FIELD_CMD_HDR,

  axis_CMD_TIMER_FIELD_TIMER_ID,
  axis_CMD_TIMER_FIELD_TIMEOUT_IN_US,
  axis_CMD_TIMER_FIELD_TIMES,

  axis_CMD_TIMER_FIELD_LAST,
} axis_CMD_TIMER_FIELD;
