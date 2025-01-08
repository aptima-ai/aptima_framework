//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

typedef enum axis_CMD_CONNECT_FIELD {
  axis_CMD_CONNECT_FIELD_CMD_HDR,

  axis_CMD_CONNECT_FIELD_LONG_RUNNING_MODE,
  axis_CMD_CONNECT_FIELD_EXTENSION_INFO,

  axis_CMD_CONNECT_FIELD_PREDEFINED_GRAPH,

  axis_CMD_CONNECT_FIELD_LAST,
} axis_CMD_CONNECT_FIELD;
