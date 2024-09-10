//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_utils/ten_config.h"

typedef enum TEN_HTTP_METHOD {
  TEN_HTTP_METHOD_INVALID,

  TEN_HTTP_METHOD_GET,
  TEN_HTTP_METHOD_POST,
  TEN_HTTP_METHOD_PUT,
  TEN_HTTP_METHOD_PATCH,
  TEN_HTTP_METHOD_DELETE,
  TEN_HTTP_METHOD_HEAD,
  TEN_HTTP_METHOD_OPTIONS,

  TEN_HTTP_METHOD_FIRST = TEN_HTTP_METHOD_GET,
  TEN_HTTP_METHOD_LAST = TEN_HTTP_METHOD_OPTIONS,
} TEN_HTTP_METHOD;
