//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

typedef enum axis_CMD_BASE_FIELD {
  axis_CMD_BASE_FIELD_MSGHDR,

  axis_CMD_BASE_FIELD_CMD_ID,
  axis_CMD_BASE_FIELD_SEQ_ID,

  axis_CMD_BASE_FIELD_ORIGINAL_CONNECTION,

  axis_CMD_BASE_FIELD_RESPONSE_HANDLER,
  axis_CMD_BASE_FIELD_RESPONSE_HANDLER_DATA,

  axis_CMD_BASE_FIELD_LAST,
} axis_CMD_BASE_FIELD;
