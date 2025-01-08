//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

typedef enum axis_HTTP_METHOD {
  axis_HTTP_METHOD_INVALID,

  axis_HTTP_METHOD_GET,
  axis_HTTP_METHOD_POST,
  axis_HTTP_METHOD_PUT,
  axis_HTTP_METHOD_PATCH,
  axis_HTTP_METHOD_DELETE,
  axis_HTTP_METHOD_HEAD,
  axis_HTTP_METHOD_OPTIONS,

  axis_HTTP_METHOD_FIRST = axis_HTTP_METHOD_GET,
  axis_HTTP_METHOD_LAST = axis_HTTP_METHOD_OPTIONS,
} axis_HTTP_METHOD;
