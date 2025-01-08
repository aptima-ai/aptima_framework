//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

typedef enum axis_MSG_FIELD {
  // 'type' should be the first, because the handling method of some fields
  // would depend on the 'type'.
  axis_MSG_FIELD_TYPE,

  axis_MSG_FIELD_NAME,

  axis_MSG_FIELD_SRC,
  axis_MSG_FIELD_DEST,
  axis_MSG_FIELD_PROPERTIES,

  axis_MSG_FIELD_LAST,
} axis_MSG_FIELD;
