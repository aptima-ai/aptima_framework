//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_runtime/aptima_config.h"

typedef enum aptima_STATUS_CODE {
  aptima_STATUS_CODE_INVALID = -1,

  // 0 representing OK is a common convention.
  aptima_STATUS_CODE_OK = 0,
  aptima_STATUS_CODE_ERROR = 1,

  aptima_STATUS_CODE_LAST = 2,
} aptima_STATUS_CODE;
