//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

typedef enum axis_CMD_STATUS_FIELD {
  axis_CMD_STATUS_FIELD_CMD_BASE_HDR,

  axis_CMD_STATUS_FIELD_ORIGINAL_CMD_TYPE,
  axis_CMD_STATUS_FIELD_STATUS_CODE,
  axis_CMD_STATUS_FIELD_IS_FINAL,

  axis_CMD_STATUS_FIELD_LAST,
} axis_CMD_STATUS_FIELD;
