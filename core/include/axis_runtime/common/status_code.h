//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

typedef enum axis_STATUS_CODE {
  axis_STATUS_CODE_INVALID = -1,

  // 0 representing OK is a common convention.
  axis_STATUS_CODE_OK = 0,
  axis_STATUS_CODE_ERROR = 1,

  axis_STATUS_CODE_LAST = 2,
} axis_STATUS_CODE;
