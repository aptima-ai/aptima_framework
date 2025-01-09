//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

typedef enum aptima_HTTP_METHOD {
  aptima_HTTP_METHOD_INVALID,

  aptima_HTTP_METHOD_GET,
  aptima_HTTP_METHOD_POST,
  aptima_HTTP_METHOD_PUT,
  aptima_HTTP_METHOD_PATCH,
  aptima_HTTP_METHOD_DELETE,
  aptima_HTTP_METHOD_HEAD,
  aptima_HTTP_METHOD_OPTIONS,

  aptima_HTTP_METHOD_FIRST = aptima_HTTP_METHOD_GET,
  aptima_HTTP_METHOD_LAST = aptima_HTTP_METHOD_OPTIONS,
} aptima_HTTP_METHOD;
